import sys
import re
import numpy as np
import matplotlib.pyplot as plt
from scipy import signal as sig

def parse_pcm_data(input_text):
    """Parse PCM data from the format 'index : value'"""
    pattern = r'(\d+)\s*:\s*(-?\d+\.?\d*(?:e[+-]?\d+)?)'
    matches = re.findall(pattern, input_text)
    
    indices = []
    values = []
    for idx, val in matches:
        indices.append(int(idx))
        values.append(float(val))
    
    return np.array(indices), np.array(values)

def parse_adsr_states(input_text, num_samples):
    """Parse ADSR state transitions from logs"""
    # Find ADSR state markers in the log
    adsr_pattern = r'ADSR\s*:\s*(ATTACK|DECAY|SUSTAIN|RELEASE|OFF)'
    matches = list(re.finditer(adsr_pattern, input_text))
    
    states = []
    colors = {
        'ATTACK': 'green',
        'DECAY': 'orange', 
        'SUSTAIN': 'blue',
        'RELEASE': 'purple',
        'OFF': 'gray'
    }
    
    # Estimate sample positions based on order of appearance
    # This is approximate - ideally you'd log the sample index too
    for i, match in enumerate(matches):
        state_name = match.group(1)
        # Estimate position proportionally through the buffer
        estimated_pos = int((i + 1) * num_samples / (len(matches) + 1)) if matches else 0
        states.append({
            'state': state_name,
            'position': estimated_pos,
            'color': colors.get(state_name, 'black')
        })
    
    return states

def detect_anomalies(samples, threshold_std=3.0):
    """Detect sudden jumps or discontinuities"""
    diff = np.diff(samples)
    mean_diff = np.mean(np.abs(diff))
    std_diff = np.std(diff)
    
    anomalies = np.where(np.abs(diff) > mean_diff + threshold_std * std_diff)[0]
    return anomalies, diff

def print_click_details(samples, anomalies, diff):
    """Print detailed information about each detected click"""
    if len(anomalies) == 0:
        print("\n✓ No clicks/pops detected")
        return
    
    print("\n" + "="*70)
    print("CLICK/POP DETAILS")
    print("="*70)
    
    for i, idx in enumerate(anomalies):
        print(f"\n--- Click #{i+1} at sample {idx} ---")
        print(f"  Jump size (delta): {diff[idx]:.8f}")
        print(f"  Before: sample[{idx}] = {samples[idx]:.8f}")
        print(f"  After:  sample[{idx+1}] = {samples[idx+1]:.8f}")
        
        # Context: show 3 samples before and after
        start = max(0, idx - 3)
        end = min(len(samples), idx + 5)
        print(f"  Context:")
        for j in range(start, end):
            marker = " >>>" if j == idx or j == idx + 1 else "    "
            print(f"  {marker} [{j}]: {samples[j]:.8f}")
    
    print("\n" + "="*70)
    print("SUMMARY")
    print("="*70)
    print(f"Total clicks detected: {len(anomalies)}")
    print(f"Largest jump: {np.max(np.abs(diff[anomalies])):.8f} at sample {anomalies[np.argmax(np.abs(diff[anomalies]))]}")
    print(f"Average jump size: {np.mean(np.abs(diff[anomalies])):.8f}")
    print(f"Normal avg delta: {np.mean(np.abs(diff)):.8f}")
    print(f"Click indices: {list(anomalies)}")

def analyze_pcm(samples, sample_rate=44100, raw_data=""):
    """Analyze PCM buffer for imperfections"""
    fig, axes = plt.subplots(4, 1, figsize=(14, 12))
    
    # Parse ADSR states from raw data
    adsr_states = parse_adsr_states(raw_data, len(samples))
    
    # 1. Waveform
    axes[0].plot(samples, linewidth=0.8)
    axes[0].set_title('Waveform')
    axes[0].set_xlabel('Sample Index')
    axes[0].set_ylabel('Amplitude')
    axes[0].grid(True, alpha=0.3)
    
    # Mark ADSR state transitions
    for state in adsr_states:
        pos = state['position']
        if pos < len(samples):
            axes[0].axvline(x=pos, color=state['color'], linestyle='--', alpha=0.7, linewidth=2)
            axes[0].annotate(state['state'], xy=(pos, samples[pos] if pos < len(samples) else 0), 
                           xytext=(5, 10), textcoords='offset points',
                           fontsize=9, fontweight='bold', color=state['color'])
    
    # Add legend for ADSR states
    from matplotlib.lines import Line2D
    legend_elements = [
        Line2D([0], [0], color='green', linestyle='--', label='ATTACK'),
        Line2D([0], [0], color='orange', linestyle='--', label='DECAY'),
        Line2D([0], [0], color='blue', linestyle='--', label='SUSTAIN'),
        Line2D([0], [0], color='purple', linestyle='--', label='RELEASE'),
        Line2D([0], [0], color='gray', linestyle='--', label='OFF'),
    ]
    axes[0].legend(handles=legend_elements, loc='upper right')
    
    # Mark anomalies (clicks)
    anomalies, diff = detect_anomalies(samples)
    if len(anomalies) > 0:
        axes[0].scatter(anomalies, samples[anomalies], color='red', s=50, zorder=5, 
                       marker='x', linewidths=2, label=f'Clicks ({len(anomalies)})')
    
    # 2. First derivative (rate of change)
    axes[1].plot(diff, linewidth=0.8, color='orange')
    axes[1].set_title('First Derivative (Rate of Change) - Spikes indicate clicks/pops')
    axes[1].set_xlabel('Sample Index')
    axes[1].set_ylabel('Delta')
    axes[1].grid(True, alpha=0.3)
    
    # Mark ADSR transitions on derivative too
    for state in adsr_states:
        pos = state['position']
        if pos < len(diff):
            axes[1].axvline(x=pos, color=state['color'], linestyle='--', alpha=0.5, linewidth=1)
    
    if len(anomalies) > 0:
        axes[1].scatter(anomalies, diff[anomalies], color='red', s=50, zorder=5, marker='x', linewidths=2)
    
    # 3. Spectrum (FFT)
    fft = np.fft.fft(samples)
    freqs = np.fft.fftfreq(len(samples), 1/sample_rate)
    magnitude = np.abs(fft)[:len(fft)//2]
    freqs = freqs[:len(freqs)//2]
    
    axes[2].plot(freqs, magnitude, linewidth=0.8, color='green')
    axes[2].set_title('Frequency Spectrum')
    axes[2].set_xlabel('Frequency (Hz)')
    axes[2].set_ylabel('Magnitude')
    axes[2].grid(True, alpha=0.3)
    axes[2].set_xlim(0, sample_rate/2)
    
    # 4. Spectrogram
    if len(samples) > 64:
        nperseg = min(256, len(samples)//4)
        f, t, Sxx = sig.spectrogram(samples, fs=sample_rate, nperseg=nperseg)
        axes[3].pcolormesh(t, f, 10*np.log10(Sxx + 1e-10), shading='gouraud')
        axes[3].set_title('Spectrogram')
        axes[3].set_xlabel('Time (s)')
        axes[3].set_ylabel('Frequency (Hz)')
    
    plt.tight_layout()
    plt.savefig('pcm_analysis.png', dpi=150)
    plt.show()
    
    # Print ADSR state transitions
    if adsr_states:
        print("\n" + "="*70)
        print("ADSR STATE TRANSITIONS")
        print("="*70)
        for i, state in enumerate(adsr_states):
            print(f"  #{i+1}: {state['state']} at ~sample {state['position']}")
    
    # Print detailed click information
    print_click_details(samples, anomalies, diff)
    
    # Print statistics
    print("\n=== PCM Analysis Report ===")
    print(f"Samples: {len(samples)}")
    print(f"Min: {np.min(samples):.6f}, Max: {np.max(samples):.6f}")
    print(f"Mean: {np.mean(samples):.6f}, Std: {np.std(samples):.6f}")
    print(f"Peak-to-Peak: {np.max(samples) - np.min(samples):.6f}")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        with open(sys.argv[1], 'r') as f:
            data = f.read()
    else:
        print("Reading from stdin (paste data, then Ctrl+D)...")
        data = sys.stdin.read()
    
    indices, samples = parse_pcm_data(data)
    
    if len(samples) == 0:
        print("Error: No PCM data found")
        sys.exit(1)
    
    print(f"Parsed {len(samples)} samples")
    analyze_pcm(samples, raw_data=data)