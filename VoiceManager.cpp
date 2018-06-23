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

}

void VoiceManager::onNoteOff(int noteNumber, int velocity) {

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

VoiceManager::VoiceManager()
{
	for (int i = 0; i < NumberOfVoices; i++) {
		voices[i].setVoiceNo(i);
	}
}

