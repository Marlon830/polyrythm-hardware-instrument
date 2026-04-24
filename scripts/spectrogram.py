#!/usr/bin/env python3
"""
Live spectrogram viewer.
Lit des valeurs flottantes sur stdin (même format que scripts/wave_viewer.py)
Exemple :
  ./TinyFirmware | ./scripts/spectrogram.py --rate 8000
"""
import sys
import re
import threading
import collections
import signal
import time
import argparse

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation

LINE_RE = re.compile(r':\s*([+-]?\d*\.?\d+(?:[eE][+-]?\d+)?)')

def _choose_style():
    for s in ('seaborn-dark', 'seaborn', 'dark_background', 'classic'):
        try:
            plt.style.use(s)
            return
        except OSError:
            continue

def stdin_reader(sample_buffer, stop_event):
    while not stop_event.is_set():
        line = sys.stdin.readline()
        if line == '':
            break
        m = LINE_RE.search(line)
        if m:
            try:
                v = float(m.group(1))
                sample_buffer.append(v)
            except Exception:
                pass
    stop_event.set()

def start_plot(sample_buffer, stop_event, rate=8000, win=256, hop=128, cols=100, interval_ms=50):
    _choose_style()
    n_fft = int(1 << (win - 1).bit_length()) if (win & (win - 1)) != 0 else win
    freq_bins = n_fft // 2 + 1

    fig, ax = plt.subplots()
    ax.set_title('Live Spectrogram (close window to quit)')
    ax.set_xlabel('Time (columns)')
    ax.set_ylabel('Frequency (Hz)')

    # spectrogram matrix: rows = freq_bins, cols = time steps
    spec = np.full((freq_bins, cols), -120.0, dtype=np.float32)  # dB
    im = ax.imshow(spec, origin='lower', aspect='auto', cmap='magma',
                   vmin=-80, vmax=0, extent=[0, cols, 0, rate/2])
    fig.colorbar(im, ax=ax, label='Magnitude (dB)')

    window = np.hanning(win) if win > 1 else np.ones(win)
    processing_buffer = collections.deque(maxlen=win)

    def update(frame):
        # consume incoming samples into processing_buffer
        while sample_buffer and len(processing_buffer) < win:
            processing_buffer.append(sample_buffer.popleft())

        if len(processing_buffer) >= win:
            # build windowed frame (if processing_buffer shorter than n_fft, pad)
            frame_data = np.array(processing_buffer)
            if len(frame_data) < n_fft:
                x = np.zeros(n_fft, dtype=float)
                x[:len(frame_data)] = frame_data
            else:
                x = frame_data[:n_fft]
            if len(window) != len(x):
                w = np.hanning(len(x))
            else:
                w = window
            fft = np.fft.rfft(x * w, n=n_fft)
            mag = 20.0 * np.log10(np.abs(fft) + 1e-8)
            # shift spectrogram left and append new column
            spec[:, :-1] = spec[:, 1:]
            spec[:, -1] = mag[:freq_bins]
            im.set_data(spec)
            # drop hop samples to advance (if hop==0 avoid infinite loop)
            for _ in range(max(1, hop)):
                if processing_buffer:
                    processing_buffer.popleft()

        if stop_event.is_set() and not sample_buffer and len(processing_buffer) < win:
            plt.close(fig)
            return (im,)
        return (im,)

    ani = animation.FuncAnimation(fig, update, interval=interval_ms, blit=True)
    plt.show()
    stop_event.set()

def main():
    parser = argparse.ArgumentParser(description='Live spectrogram viewer (stdin floats).')
    parser.add_argument('--rate', type=int, default=44100, help='sample rate in Hz')
    parser.add_argument('--win', type=int, default=256, help='analysis window size (samples)')
    parser.add_argument('--hop', type=int, default=128, help='hop size (samples)')
    parser.add_argument('--cols', type=int, default=100, help='number of time columns shown')
    parser.add_argument('--interval', type=int, default=50, help='plot update interval in ms')
    args = parser.parse_args()

    sample_buffer = collections.deque()
    stop_event = threading.Event()

    signal.signal(signal.SIGINT, lambda s, f: stop_event.set())

    reader = threading.Thread(target=stdin_reader, args=(sample_buffer, stop_event), daemon=True)
    reader.start()

    start_plot(sample_buffer, stop_event,
               rate=args.rate, win=args.win, hop=args.hop, cols=args.cols, interval_ms=args.interval)

    stop_event.set()
    reader.join(timeout=1.0)

if __name__ == '__main__':
    main()