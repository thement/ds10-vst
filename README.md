# DS-10 emulator

A reimagination of Korg DS-10 synthesizer as a VST.

## Build

There's a pre-built VST2 dll for x64 [right in this repository](ds10-vst2-x64.dll).

I have no mac so no .au builds yet.

## Changes from original DS-10

- Added polyphony - up to 4 voices polyphony. When in mono-mode, the priority is "last played".

- Added resampling mode: when the "resample" switch is turned on, the played note is always C-3, but the "replay speed" is changed so it plays at correct pitch.
This mimics the "drum track" mode, which sampled one note and then replayed the sample at various pitches.

  For extra grittiness, turn off the "oversample switch" (which does 4x oversampling). Note that oversampling is done no matter the "resampling" switch in order to convert from Nintendo DS audio frequency (32768 Hz) to host frequency. Turning off oversampling lowers audio quality.
  
  **Warning**: replaying at higher pitches than C3 with resampling on causes high CPU usage (for example, at C5 the synth has to run at 4x the speed).

  **Note**: polyphony doesn't work well in "resample mode". There's extra knob that just pitches down the replayed synth, which works well with polyphony.

- no effects (yet)

- no drum track (but see resample)

- no hanging wires - instead you can click on window bellow the knobs to select modulation source

## DS-10 internals

[There's a log](http://ibawizard.net/~thement/ds10rev.html) of my work on DS-10 reverse engineering.

## Screenshot

![alt text](screenshot.png "Screenshot")


