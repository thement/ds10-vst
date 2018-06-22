//
//  Oscillator.h
//  SpaceBass
//
//  Created by Martin on 08.04.14.
//
//

#ifndef __SpaceBass__Oscillator__
#define __SpaceBass__Oscillator__

#include <math.h>

class Oscillator {
public:
    enum OscillatorMode {
        OSCILLATOR_MODE_SINE = 0,
        OSCILLATOR_MODE_SAW,
        OSCILLATOR_MODE_SQUARE,
        OSCILLATOR_MODE_TRIANGLE,
        kNumOscillatorModes
    };
    void setMode(OscillatorMode mode);
    void setFrequency(double frequency);
    void setSampleRate(double sampleRate);
    void generate(double* buffer, int nFrames);
    inline void setMuted(bool muted) { isMuted = muted; }
    virtual double nextSample();
    Oscillator() :
    mOscillatorMode(OSCILLATOR_MODE_SINE),
    mPI(2*acos(0.0)),
    twoPI(2 * mPI),
    isMuted(true),
    mFrequency(440.0),
    mPitchMod(0.0),
    mPhase(0.0) { updateIncrement(); };
    
    void reset() { mPhase = 0.0; }
    void setPitchMod(double amount);
protected:
    double naiveWaveformForMode(OscillatorMode mode);
    OscillatorMode mOscillatorMode;
    const double mPI;
    const double twoPI;
    bool isMuted;
    double mFrequency;
    double mPhase;
    static double mSampleRate;
    double mPhaseIncrement;
    void updateIncrement();
    
    double mPitchMod;
};

#endif /* defined(__SpaceBass__Oscillator__) */
