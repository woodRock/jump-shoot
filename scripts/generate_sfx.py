import wave
import struct
import math
import os

def generate_wave(filename, duration, freq_func):
    sample_rate = 44100
    n_samples = int(sample_rate * duration)
    
    with wave.open(filename, 'w') as f:
        f.setnchannels(1)
        f.setsampwidth(2)
        f.setframerate(sample_rate)
        
        for i in range(n_samples):
            t = i / sample_rate
            freq = freq_func(t)
            value = int(32767.0 * math.sin(2.0 * math.pi * freq * t) * (1.0 - i/n_samples))
            f.writeframesraw(struct.pack('<h', value))

def ensure_dir(directory):
    if not os.path.exists(directory):
        os.makedirs(directory)

def main():
    ensure_dir("assets")
    
    # Shoot: high to low frequency
    generate_wave("assets/shoot.wav", 0.2, lambda t: 800 - 400 * t / 0.2)
    
    # Hit: low frequency thud
    generate_wave("assets/hit.wav", 0.15, lambda t: 200 - 100 * t / 0.15)
    
    # Jump: low to high
    generate_wave("assets/jump.wav", 0.1, lambda t: 300 + 300 * t / 0.1)
    
    # Grapple: high pitch zip
    generate_wave("assets/grapple.wav", 0.3, lambda t: 1200 - 200 * t / 0.3)

    print("Generated SFX in assets/")

if __name__ == "__main__":
    main()
