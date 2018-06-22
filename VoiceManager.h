//
//  VoiceManager.h
//  SpaceBass
//
//  Created by Martin on 08.04.14.
//
//

#ifndef __SpaceBass__VoiceManager__
#define __SpaceBass__VoiceManager__

#include "Voice.h"
#include <functional>
// #include <functional> if that doesn't work

class VoiceManager {
public:
    void onNoteOn(int noteNumber, int velocity);
    void onNoteOff(int noteNumber, int velocity);
    double nextSample();
    
    void setSampleRate(double sampleRate) {
        EnvelopeGenerator::setSampleRate(sampleRate);
        for (int i = 0; i < NumberOfVoices; i++) {
            Voice& voice = voices[i];
            voice.mOscillatorOne.setSampleRate(sampleRate);
            voice.mOscillatorTwo.setSampleRate(sampleRate);
        }
        mLFO.setSampleRate(sampleRate);
    }
    
    inline void setLFOMode(Oscillator::OscillatorMode mode) { mLFO.setMode(mode); };
    inline void setLFOFrequency(double frequency) { mLFO.setFrequency(frequency); };
    
    typedef std::tr1::function<void (Voice&)> VoiceChangerFunction;
    inline void changeAllVoices(VoiceChangerFunction changer) {
        for (int i = 0; i < NumberOfVoices; i++) {
            changer(voices[i]);
        }
    }
    // Functions to change a single voice:
    static void setVolumeEnvelopeStageValue(Voice& voice, EnvelopeGenerator::EnvelopeStage stage, double value) {
        voice.mVolumeEnvelope.setStageValue(stage, value);
    }
    static void setFilterEnvelopeStageValue(Voice& voice, EnvelopeGenerator::EnvelopeStage stage, double value) {
        voice.mFilterEnvelope.setStageValue(stage, value);
    }
    static void setOscillatorMode(Voice& voice, int oscillatorNumber, Oscillator::OscillatorMode mode) {
        switch (oscillatorNumber) {
            case 1:
                voice.mOscillatorOne.setMode(mode);
                break;
            case 2:
                voice.mOscillatorTwo.setMode(mode);
                break;
        }
    }
    static void setOscillatorPitchMod(Voice& voice, int oscillatorNumber, double amount) {
        switch (oscillatorNumber) {
            case 1:
                voice.setOscillatorOnePitchAmount(amount);
                break;
            case 2:
                voice.setOscillatorTwoPitchAmount(amount);
                break;
        }
    }
    static void setOscillatorMix(Voice& voice, double value) {
        voice.setOscillatorMix(value);
    }
    static void setFilterCutoff(Voice& voice, double cutoff) {
        voice.mFilter.setCutoff(cutoff);
    }
    static void setFilterResonance(Voice& voice, double resonance) {
        voice.mFilter.setResonance(resonance);
    }
    static void setFilterMode(Voice& voice, Filter::FilterMode mode) {
        voice.mFilter.setFilterMode(mode);
    }
    static void setFilterEnvAmount(Voice& voice, double amount) {
        voice.setFilterEnvelopeAmount(amount);
    }
    static void setFilterLFOAmount(Voice& voice, double amount) {
        voice.setFilterLFOAmount(amount);
    }
private:
    static const int NumberOfVoices = 64;
    Voice voices[NumberOfVoices];
    Oscillator mLFO;
    Voice* findFreeVoice();
};

#endif /* defined(__SpaceBass__VoiceManager__) */
