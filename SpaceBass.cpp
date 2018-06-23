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
	kNumParams = 26 + 7 - 1,
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
};

typedef struct Params Params;
struct Params {
  const int type;
  const char *name;
  const int x;
  const int y;
  const int ds_id;
  const int def;
  const int min;
  const int max;
};

enum {
	PKnob,
	PKnob4,
	PKnob5,
	PSwitch2,
	PSwitch3,
	PModsrc7,
	PVoices4,
	PNumTypes
};
const Params parameterProperties[kNumParams] = {
	{ PKnob5,	"octave",	20, 58,		0,	0, -2, 2	},
	{ PKnob,	"vco.int",	20, 222,	1,	0, -63, 63 },
	{ PKnob,	"porta",	20, 140,	2,	0, 0, 127	},
	{ PKnob,	"pitch.in",	98, 537,	3,	0, -63, 63	},
	{ PKnob,	"pitch1.in",184, 537,	4,	0, -63, 63	},
	{ PKnob,	"pitch2.in",270, 537,	5,	0, -63, 63	},
	{ PKnob4,	"vco1",		126, 58,	6,	0, 0, 3		},
	{ PKnob4,	"vco2",		220, 58,	7,	0, 0, 3		},
	{ PKnob,	"vco2pitch",220, 140,	8,	0, -63, 63	},
	{ PKnob,	"balance",	126, 140,	9,	0, 0, 127	},
	{ PSwitch2,	"vco2sync",	232, 218,	10,	0, 0, 1		},
	{ PKnob,	"cutoff",	324, 58,	11,	127, 0, 127	},
	{ PKnob,	"peak",		324, 140,	12, 0, 0, 127	},
	{ PSwitch3,	"vcf.type",	338, 306,	13, 0, 0, 2		},
	{ PKnob,	"eg.int",	324, 222,	14,	0, -63, 63	},
	{ PKnob,	"cutoff.in",356, 537,	15, 0, -63, 63	},
	{ PSwitch2,	"vca.eg",	232, 306,	16, 0, 0, 1		},
	{ PKnob,	"vca.in",	442, 537,	17, 0, -63, 63	},
	{ PKnob,	"drive",	126, 304,	18, 0, 0, 127	},
	{ PKnob,	"level",	20, 304,	19, 0, 0, 100	},
	{ PKnob,	"attack",	426, 58,	20, 0, 0, 127	},
	{ PKnob,	"decay",	426, 140,	21, 127, 0, 127	},
	{ PKnob,	"sustain",	426, 222,	22, 127, 0, 127	},
	{ PKnob,	"release",	426, 304,	23, 0, 0, 127	},
	{ PKnob,	"mg.freq",	12, 537,	24, 0, 0, 127	},
	{ PSwitch2, "mg.bpm",	20, 458,	25, 0, 0, 1		},
	{ PModsrc7, "modsrc0",	112, 628,	26, 0, 0, 6		},
	{ PModsrc7, "modsrc1",	198, 628,	27, 0, 0, 6		},
	{ PModsrc7, "modsrc2",	284, 628,	28, 0, 0, 6		},
	{ PModsrc7, "modsrc3",	370, 628,	29, 0, 0, 6		},
	{ PModsrc7, "modsrc4",	458, 628,	30, 0, 0, 6		},
	{ PVoices4, "poly",		19, 712,	-1,	0, 0, 3		},
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
  //printf("hello world, params=%p\n", CreateParams);
  ds10_init(4);
  CreateParams();
  CreateGraphics();
  CreatePresets();
  
  mMIDIReceiver.noteOn.Connect(&voiceManager, &VoiceManager::onNoteOn);
  mMIDIReceiver.noteOff.Connect(&voiceManager, &VoiceManager::onNoteOff);
}

SpaceBass::~SpaceBass() {}

void SpaceBass::CreateParams() {
	char name[32];
	for (int i = 0; i < kNumParams; i++) {
		IParam *par = GetParam(i);
		const Params *param = &parameterProperties[i];
		sprintf(name, "param%d", i);

		switch (param->type) {
		default:
		case PKnob:
			par->InitDouble(name, param->def, param->min, param->max, 0.01);
			break;
		case PSwitch2:
		case PSwitch3:
		case PKnob4:
		case PKnob5:
		case PVoices4:
		case PModsrc7:
			par->InitEnum(name, param->def, param->max - param->min + 1);
			break;
		}
	}
	for (int i = 0; i < kNumParams; i++) {
		OnParamChange(i);
	}
}

void SpaceBass::CreateGraphics() {
	IGraphics* pGraphics = MakeGraphics(this, kWidth, kHeight);
	pGraphics->AttachBackground(BG_ID, BG_FN);

#if 0
	IBitmap waveformBitmap = pGraphics->LoadIBitmap(WAVEFORM_ID, WAVEFORM_FN, 4);
	IBitmap filterModeBitmap = pGraphics->LoadIBitmap(FILTERMODE_ID, FILTERMODE_FN, 3);
#endif
	IBitmap bitmaps[PNumTypes];
	bitmaps[PKnob] = pGraphics->LoadIBitmap(KNOB_ID, KNOB_FN, 128);
	bitmaps[PKnob4] = pGraphics->LoadIBitmap(KNOB4_ID, KNOB4_FN, 4);
	bitmaps[PKnob5] = pGraphics->LoadIBitmap(KNOB5_ID, KNOB5_FN, 5);
	bitmaps[PSwitch2] = pGraphics->LoadIBitmap(SWITCH2_ID, SWITCH2_FN, 2);
	bitmaps[PSwitch3] = pGraphics->LoadIBitmap(SWITCH3_ID, SWITCH3_FN, 3);
	bitmaps[PVoices4] = pGraphics->LoadIBitmap(VOICES4_ID, VOICES4_FN, 4);
	bitmaps[PModsrc7] = pGraphics->LoadIBitmap(MODSRC7_ID, MODSRC7_FN, 7);

	for (int i = 0; i < kNumParams; i++) {
		IControl* control;
		const Params *param = &parameterProperties[i];

		switch (param->type) {
		case PKnob:
		case PKnob4:
		case PKnob5:
		default:
			control = new IKnobMultiControl(this, param->x, param->y, i, &bitmaps[param->type]);
			break;
		case PSwitch2:
		case PSwitch3:
		case PModsrc7:
		case PVoices4:
			control = new ISwitchControl(this, param->x, param->y, i, &bitmaps[param->type]);
			break;
		}
		pGraphics->AttachControl(control);
	}
	AttachGraphics(pGraphics);
}

#if 0
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
#ifdef KEYBOARD_ENABLED
  IBitmap whiteKeyImage = pGraphics->LoadIBitmap(WHITE_KEY_ID, WHITE_KEY_FN, 6);
  IBitmap blackKeyImage = pGraphics->LoadIBitmap(BLACK_KEY_ID, BLACK_KEY_FN);
  //                            C#     D#          F#      G#      A#
  int keyCoordinates[12] = { 0, 10, 17, 30, 35, 52, 61, 68, 79, 85, 97, 102 };
  mVirtualKeyboard = new IKeyboardControl(this, kKeybX, kKeybY, virtualKeyboardMinimumNoteNumber, /* octaves: */ 4, &whiteKeyImage, &blackKeyImage, keyCoordinates);
  pGraphics->AttachControl(mVirtualKeyboard);
#endif
  IBitmap waveformBitmap = pGraphics->LoadIBitmap(WAVEFORM_ID, WAVEFORM_FN, 4);
  IBitmap filterModeBitmap = pGraphics->LoadIBitmap(FILTERMODE_ID, FILTERMODE_FN, 3);
  IBitmap knobBitmap = pGraphics->LoadIBitmap(KNOB_ID, KNOB_FN, 128);
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
#endif

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
#ifdef KEYBOARD_ENABLED
  processVirtualKeyboard();
#endif
  for (int i = 0; i < nFrames; ++i) {
    mMIDIReceiver.advance();
    leftOutput[i] = rightOutput[i] = ds10_get_sample();
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
  printf("param %d: %lf\n", paramIdx, param->Value());
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
		ds10_knob_all(11, param->Value() * 128);
		return;
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
	  default: return;
    }
    voiceManager.changeAllVoices(changer);
  }
}

void SpaceBass::ProcessMidiMsg(IMidiMsg* pMsg) {
  mMIDIReceiver.onMessageReceived(pMsg);
#ifdef KEYBOARD_ENABLED
  mVirtualKeyboard->SetDirty();
#endif
}

#ifdef KEYBOARD_ENABLED
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
#endif