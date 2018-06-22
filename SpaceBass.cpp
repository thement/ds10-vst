#include "SpaceBass.h"
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmain"
#include "IPlug_include_in_plug_src.h"
#pragma clang diagnostic pop
#include "IControl.h"
#include "IKeyboardControl.h"
#include "resource.h"

#include <math.h>
#include <algorithm>

#include <functional>
extern "C" {
#include "ds10.h"
}
const int kNumPrograms = 5;

const double parameterStep = 0.001;

enum EParams
{
  // Oscillator Section:
  mOsc1Waveform = 0,
  mOsc1PitchMod,
  mOsc2Waveform,
  mOsc2PitchMod,
  mOscMix,
  // Filter Section:
  mFilterMode,
  mFilterCutoff,
  mFilterResonance,
  mFilterLfoAmount,
  mFilterEnvAmount,
  // LFO:
  mLFOWaveform,
  mLFOFrequency,
  // Volume Envelope:
  mVolumeEnvAttack,
  mVolumeEnvDecay,
  mVolumeEnvSustain,
  mVolumeEnvRelease,
  // Filter Envelope:
  mFilterEnvAttack,
  mFilterEnvDecay,
  mFilterEnvSustain,
  mFilterEnvRelease,
  kNumParams
};

typedef struct {
  const char* name;
  const int x;
  const int y;
  const double defaultVal;
  const double minVal;
  const double maxVal;
} parameterProperties_struct;

const parameterProperties_struct parameterProperties[kNumParams] = {
  {"Osc 1 Waveform", 30, 75},
  {"Osc 1 Pitch Mod", 69, 61, 0.0, 0.0, 1.0},
  {"Osc 2 Waveform", 203, 75},
  {"Osc 2 Pitch Mod", 242, 61, 0.0, 0.0, 1.0},
  {"Osc Mix", 130, 61, 0.5, 0.0, 1.0},
  {"Filter Mode", 30, 188},
  {"Filter Cutoff", 69, 174, 0.99, 0.0, 0.99},
  {"Filter Resonance", 124, 174, 0.0, 0.0, 1.0},
  {"Filter LFO Amount", 179, 174, 0.0, 0.0, 1.0},
  {"Filter Envelope Amount", 234, 174, 0.0, -1.0, 1.0},
  {"LFO Waveform", 30, 298},
  {"LFO Frequency", 69, 284, 6.0, 0.01, 30.0},
  {"Volume Env Attack", 323, 61, 0.01, 0.01, 10.0},
  {"Volume Env Decay", 378, 61, 0.5, 0.01, 15.0},
  {"Volume Env Sustain", 433, 61, 0.1, 0.001, 1.0},
  {"Volume Env Release", 488, 61, 1.0, 0.01, 15.0},
  {"Filter Env Attack", 323, 174, 0.01, 0.01, 10.0},
  {"Filter Env Decay", 378, 174, 0.5, 0.01, 15.0},
  {"Filter Env Sustain", 433, 174, 0.1, 0.001, 1.0},
  {"Filter Env Release", 488, 174, 1.0, 0.01, 15.0}
};

enum ELayout
{
  kWidth = GUI_WIDTH,
  kHeight = GUI_HEIGHT,
  kKeybX = 62,
  kKeybY = 425
};



SpaceBass::SpaceBass(IPlugInstanceInfo instanceInfo) : IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo), lastVirtualKeyboardNoteNumber(virtualKeyboardMinimumNoteNumber - 1) {
  TRACE;
  printf("hello world\n");
  CreateParams();
  CreateGraphics();
  CreatePresets();
  ds10_init(0);
  
  mMIDIReceiver.noteOn.Connect(&voiceManager, &VoiceManager::onNoteOn);
  mMIDIReceiver.noteOff.Connect(&voiceManager, &VoiceManager::onNoteOff);
}

SpaceBass::~SpaceBass() {}

void SpaceBass::CreateParams() {
  for (int i = 0; i < kNumParams; i++) {
    IParam* param = GetParam(i);
    const parameterProperties_struct& properties = parameterProperties[i];
    switch (i) {
        // Enum Parameters:
      case mOsc1Waveform:
      case mOsc2Waveform:
        param->InitEnum(properties.name,
                        Oscillator::OSCILLATOR_MODE_SAW,
                        Oscillator::kNumOscillatorModes);
        // For VST3:
        param->SetDisplayText(0, properties.name);
        break;
      case mLFOWaveform:
        param->InitEnum(properties.name,
                        Oscillator::OSCILLATOR_MODE_TRIANGLE,
                        Oscillator::kNumOscillatorModes);
        // For VST3:
        param->SetDisplayText(0, properties.name);
        break;
      case mFilterMode:
        param->InitEnum(properties.name,
                        Filter::FILTER_MODE_LOWPASS,
                        Filter::kNumFilterModes);
        break;
        // Double Parameters:
      default:
        param->InitDouble(properties.name,
                          properties.defaultVal,
                          properties.minVal,
                          properties.maxVal,
                          parameterStep);
        break;
    }
  }
  GetParam(mFilterCutoff)->SetShape(2);
  GetParam(mVolumeEnvAttack)->SetShape(3);
  GetParam(mFilterEnvAttack)->SetShape(3);
  GetParam(mVolumeEnvDecay)->SetShape(3);
  GetParam(mFilterEnvDecay)->SetShape(3);
  GetParam(mVolumeEnvSustain)->SetShape(2);
  GetParam(mFilterEnvSustain)->SetShape(2);
  GetParam(mVolumeEnvRelease)->SetShape(3);
  GetParam(mFilterEnvRelease)->SetShape(3);
  for (int i = 0; i < kNumParams; i++) {
    OnParamChange(i);
  }
}

void SpaceBass::CreateGraphics() {
  IGraphics* pGraphics = MakeGraphics(this, kWidth, kHeight);
  pGraphics->AttachBackground(BG_ID, BG_FN);
  IBitmap whiteKeyImage = pGraphics->LoadIBitmap(WHITE_KEY_ID, WHITE_KEY_FN, 6);
  IBitmap blackKeyImage = pGraphics->LoadIBitmap(BLACK_KEY_ID, BLACK_KEY_FN);
  //                            C#     D#          F#      G#      A#
  int keyCoordinates[12] = { 0, 10, 17, 30, 35, 52, 61, 68, 79, 85, 97, 102 };
  mVirtualKeyboard = new IKeyboardControl(this, kKeybX, kKeybY, virtualKeyboardMinimumNoteNumber, /* octaves: */ 4, &whiteKeyImage, &blackKeyImage, keyCoordinates);
  pGraphics->AttachControl(mVirtualKeyboard);
  IBitmap waveformBitmap = pGraphics->LoadIBitmap(WAVEFORM_ID, WAVEFORM_FN, 4);
  IBitmap filterModeBitmap = pGraphics->LoadIBitmap(FILTERMODE_ID, FILTERMODE_FN, 3);
  IBitmap knobBitmap = pGraphics->LoadIBitmap(KNOB_ID, KNOB_FN, 64);
  for (int i = 0; i < kNumParams; i++) {
    const parameterProperties_struct& properties = parameterProperties[i];
    IControl* control;
    IBitmap* graphic;
    switch (i) {
        // Switches:
      case mOsc1Waveform:
      case mOsc2Waveform:
      case mLFOWaveform:
        graphic = &waveformBitmap;
        control = new ISwitchControl(this, properties.x, properties.y, i, graphic);
        break;
      case mFilterMode:
        graphic = &filterModeBitmap;
        control = new ISwitchControl(this, properties.x, properties.y, i, graphic);
        break;
        // Knobs:
      default:
        graphic = &knobBitmap;
        control = new IKnobMultiControl(this, properties.x, properties.y, i, graphic);
        break;
    }
    pGraphics->AttachControl(control);
  }
  AttachGraphics(pGraphics);
}

void SpaceBass::CreatePresets() {
}

void SpaceBass::ProcessDoubleReplacing(
                                       double** inputs,
                                       double** outputs,
                                       int nFrames)
{
  // Mutex is already locked for us.
  
  double *leftOutput = outputs[0];
  double *rightOutput = outputs[1];
  processVirtualKeyboard();
  for (int i = 0; i < nFrames; ++i) {
    mMIDIReceiver.advance();
    leftOutput[i] = rightOutput[i] = voiceManager.nextSample();
  }
  
  mMIDIReceiver.Flush(nFrames);
}

void SpaceBass::Reset()
{
  TRACE;
  IMutexLock lock(this);
  double sampleRate = GetSampleRate();
  voiceManager.setSampleRate(sampleRate);
}

void SpaceBass::OnParamChange(int paramIdx)
{
  IMutexLock lock(this);
  IParam* param = GetParam(paramIdx);
  if(paramIdx == mLFOWaveform) {
    voiceManager.setLFOMode(static_cast<Oscillator::OscillatorMode>(param->Int()));
  } else if(paramIdx == mLFOFrequency) {
    voiceManager.setLFOFrequency(param->Value());
  } else {
    using std::tr1::placeholders::_1;
    using std::tr1::bind;
    VoiceManager::VoiceChangerFunction changer;
    switch(paramIdx) {
      case mOsc1Waveform:
        changer = bind(&VoiceManager::setOscillatorMode,
                       _1,
                       1,
                       static_cast<Oscillator::OscillatorMode>(param->Int()));
        break;
      case mOsc1PitchMod:
        changer = bind(&VoiceManager::setOscillatorPitchMod, _1, 1, param->Value());
        break;
      case mOsc2Waveform:
        changer = bind(&VoiceManager::setOscillatorMode, _1, 2, static_cast<Oscillator::OscillatorMode>(param->Int()));
        break;
      case mOsc2PitchMod:
        changer = bind(&VoiceManager::setOscillatorPitchMod, _1, 2, param->Value());
        break;
      case mOscMix:
        changer = bind(&VoiceManager::setOscillatorMix, _1, param->Value());
        break;
        // Filter Section:
      case mFilterMode:
        changer = bind(&VoiceManager::setFilterMode, _1, static_cast<Filter::FilterMode>(param->Int()));
        break;
      case mFilterCutoff:
        changer = bind(&VoiceManager::setFilterCutoff, _1, param->Value());
        break;
      case mFilterResonance:
        changer = bind(&VoiceManager::setFilterResonance, _1, param->Value());
        break;
      case mFilterLfoAmount:
        changer = bind(&VoiceManager::setFilterLFOAmount, _1, param->Value());
        break;
      case mFilterEnvAmount:
        changer = bind(&VoiceManager::setFilterEnvAmount, _1, param->Value());
        break;
        // Volume Envelope:
      case mVolumeEnvAttack:
        changer = bind(&VoiceManager::setVolumeEnvelopeStageValue, _1, EnvelopeGenerator::ENVELOPE_STAGE_ATTACK, param->Value());
        break;
      case mVolumeEnvDecay:
        changer = bind(&VoiceManager::setVolumeEnvelopeStageValue, _1, EnvelopeGenerator::ENVELOPE_STAGE_DECAY, param->Value());
        break;
      case mVolumeEnvSustain:
        changer = bind(&VoiceManager::setVolumeEnvelopeStageValue, _1, EnvelopeGenerator::ENVELOPE_STAGE_SUSTAIN, param->Value());
        break;
      case mVolumeEnvRelease:
        changer = bind(&VoiceManager::setVolumeEnvelopeStageValue, _1, EnvelopeGenerator::ENVELOPE_STAGE_RELEASE, param->Value());
        break;
        // Filter Envelope:
      case mFilterEnvAttack:
        changer = bind(&VoiceManager::setFilterEnvelopeStageValue, _1, EnvelopeGenerator::ENVELOPE_STAGE_ATTACK, param->Value());
        break;
      case mFilterEnvDecay:
        changer = bind(&VoiceManager::setFilterEnvelopeStageValue, _1, EnvelopeGenerator::ENVELOPE_STAGE_DECAY, param->Value());
        break;
      case mFilterEnvSustain:
        changer = bind(&VoiceManager::setFilterEnvelopeStageValue, _1, EnvelopeGenerator::ENVELOPE_STAGE_SUSTAIN, param->Value());
        break;
      case mFilterEnvRelease:
        changer = bind(&VoiceManager::setFilterEnvelopeStageValue, _1, EnvelopeGenerator::ENVELOPE_STAGE_RELEASE, param->Value());
        break;
    }
    voiceManager.changeAllVoices(changer);
  }
}

void SpaceBass::ProcessMidiMsg(IMidiMsg* pMsg) {
  mMIDIReceiver.onMessageReceived(pMsg);
  mVirtualKeyboard->SetDirty();
}

void SpaceBass::processVirtualKeyboard() {
  IKeyboardControl* virtualKeyboard = (IKeyboardControl*) mVirtualKeyboard;
  int virtualKeyboardNoteNumber = virtualKeyboard->GetKey() + virtualKeyboardMinimumNoteNumber;
  
  if(lastVirtualKeyboardNoteNumber >= virtualKeyboardMinimumNoteNumber && virtualKeyboardNoteNumber != lastVirtualKeyboardNoteNumber) {
    // The note number has changed from a valid key to something else (valid key or nothing). Release the valid key:
    IMidiMsg midiMessage;
    midiMessage.MakeNoteOffMsg(lastVirtualKeyboardNoteNumber, 0);
    mMIDIReceiver.onMessageReceived(&midiMessage);
  }
  
  if (virtualKeyboardNoteNumber >= virtualKeyboardMinimumNoteNumber && virtualKeyboardNoteNumber != lastVirtualKeyboardNoteNumber) {
    // A valid key is pressed that wasn't pressed the previous call. Send a "note on" message to the MIDI receiver:
    IMidiMsg midiMessage;
    midiMessage.MakeNoteOnMsg(virtualKeyboardNoteNumber, virtualKeyboard->GetVelocity(), 0);
    mMIDIReceiver.onMessageReceived(&midiMessage);
  }
  
  lastVirtualKeyboardNoteNumber = virtualKeyboardNoteNumber;
}