#!/usr/bin/env python3
"""
Live GUI waveform viewer.
Usage:
  chmod +x scripts/live_wave_gui.py
  ./TinyFirmware | ./scripts/live_wave_gui.py
"""
import sys
import re
import threading
import collections
import signal
import time

import matplotlib.pyplot as plt
import matplotlib.animation as animation

def _choose_style():
    for s in ('seaborn-dark', 'seaborn', 'dark_background', 'classic'):
        try:
            plt.style.use(s)
            return
        except OSError:
            continue
    # no preferred style found; use default
    return

LINE_RE = re.compile(r':\s*([+-]?\d*\.?\d+(?:[eE][+-]?\d+)?)')

def stdin_reader(buffer, stop_event):
    while not stop_event.is_set():
        line = sys.stdin.readline()
        if line == '':
            # EOF on pipe
            break
        m = LINE_RE.search(line)
        if m:
            try:
                v = float(m.group(1))
                buffer.append(v)
            except Exception:
                pass
    stop_event.set()

def start_plot(buffer, stop_event, interval_ms=33):
    _choose_style()
    fig, ax = plt.subplots()
    line, = ax.plot([], [], lw=1)
    ax.set_title('Live Waveform (close window to quit)')
    ax.set_xlabel('Samples')
    ax.set_ylabel('Value')
    ax.grid(True)

    def update(frame):
        if stop_event.is_set() and len(buffer) == 0:
            plt.close(fig)
            return line,
        data = list(buffer)
        if not data:
            line.set_data([], [])
            return line,
        x = list(range(len(data)))
        line.set_data(x, data)
        ax.set_xlim(0, max(1, len(data)))
        vmin = min(data)
        vmax = max(data)
        if vmin == vmax:
            margin = 0.5 if vmin == 0 else abs(vmin) * 0.1 + 0.1
            ax.set_ylim(vmin - margin, vmax + margin)
        else:
            margin = (vmax - vmin) * 0.1
            ax.set_ylim(vmin - margin, vmax + margin)
        return line,

    ani = animation.FuncAnimation(fig, update, interval=interval_ms, blit=True)
    plt.show()
    stop_event.set()

def main():
    max_samples = 2000
    buffer = collections.deque(maxlen=max_samples)
    stop_event = threading.Event()

    # make sure SIGINT closes GUI cleanly
    signal.signal(signal.SIGINT, lambda s, f: stop_event.set())

    reader = threading.Thread(target=stdin_reader, args=(buffer, stop_event), daemon=True)
    reader.start()

    start_plot(buffer, stop_event, interval_ms=33)

    # wait for reader to finish
    stop_event.set()
    reader.join(timeout=1.0)

if __name__ == '__main__':
    main()