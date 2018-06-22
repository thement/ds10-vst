//
//  VoiceManager.cpp
//  SpaceBass
//
//  Created by Martin on 08.04.14.
//
//

#include "VoiceManager.h"
extern "C" {
#include "ds10.h"
};

Voice* VoiceManager::findFreeVoice() {
    Voice* freeVoice = NULL;
    for (int i = 0; i < NumberOfVoices; i++) {
        if (!voices[i].isActive) {
            freeVoice = &(voices[i]);
            break;
        }
    }
    return freeVoice;
}

void VoiceManager::onNoteOn(int noteNumber, int velocity) {
	ds10_noteon(noteNumber, 0x7f);
	return;
    Voice* voice = findFreeVoice();
    if (!voice) {
        return;
    }
    voice->reset();
    voice->setNoteNumber(noteNumber);
    voice->mVelocity = velocity;
    voice->isActive = true;
    voice->mVolumeEnvelope.enterStage(EnvelopeGenerator::ENVELOPE_STAGE_ATTACK);
    voice->mFilterEnvelope.enterStage(EnvelopeGenerator::ENVELOPE_STAGE_ATTACK);
}

void VoiceManager::onNoteOff(int noteNumber, int velocity) {
	ds10_noteoff();
	return;
    // Find the voice(s) with the given noteNumber:
    for (int i = 0; i < NumberOfVoices; i++) {
        Voice& voice = voices[i];
        if (voice.isActive && voice.mNoteNumber == noteNumber) {
            voice.mVolumeEnvelope.enterStage(EnvelopeGenerator::ENVELOPE_STAGE_RELEASE);
            voice.mFilterEnvelope.enterStage(EnvelopeGenerator::ENVELOPE_STAGE_RELEASE);
        }
    }
}

double VoiceManager::nextSample() {
    double output = 0.0;
    double lfoValue = mLFO.nextSample();
    for (int i = 0; i < NumberOfVoices; i++) {
        Voice& voice = voices[i];
        voice.setLFOValue(lfoValue);
        output += voice.nextSample();
    }
    return output;
}

