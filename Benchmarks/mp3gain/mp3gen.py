from pydub.generators import Sine
import os

# Output directory for MP3 files
output_dir = "mp3_fuzz_inputs"
os.makedirs(output_dir, exist_ok=True)

# Generate a few MP3 files with varying properties
def generate_mp3_files():
    frequencies = [50, 100, 150, 200, 250, 300, 440, 550, 660]  # Example frequencies in Hz
    durations = [1000, 1000, 1000, 1000, 1000, 1000, 1000, 2000, 3000]  # Durations in milliseconds

    for i, (freq, duration) in enumerate(zip(frequencies, durations)):
        file_name = f"{output_dir}/sample_{i+1}.mp3"
        print(f"Generating {file_name} with frequency {freq}Hz and duration {duration}ms")

        # Generate a sine wave audio segment
        sine_wave = Sine(freq).to_audio_segment(duration=duration)

        # Export as MP3
        sine_wave.export(file_name, format="mp3")

    print(f"MP3 files have been generated in {output_dir}")

# Run the function
generate_mp3_files()
