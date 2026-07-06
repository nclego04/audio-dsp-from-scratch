# Audio DSP From Scratch

A day-by-day journey learning digital audio programming in C++ — writing WAV files, synthesizing waveforms, and exploring signals with no audio libraries.

## Contents

- **Day 1 – Simple WAV** — Writing a raw WAV file from scratch.
- **Day 2 – Generate Square and Sawtooth** — Synthesizing basic waveforms.
- **Day 3 – Sweep** — Frequency sweeps across waveforms.
- **Day 4 – Describe** — Notes on aliasing and spectrograms.

## Building

Each day is self-contained. Compile the C++ source with any C++ compiler, e.g.:

```sh
g++ "Day 1 - Simple WAV/simple_wav.cpp" -o simple_wav
./simple_wav
```

Generated `.wav`, `.exe`, and `.out` files are not tracked in git (see `.gitignore`) — they're produced by running the programs.
