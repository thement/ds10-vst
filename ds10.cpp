#include "ds10.h"
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
#include "ds10-core.h"
}
const int kNumPrograms = 32;

enum EParams
{
	kNumParams = 26 + 7 - 1 + 2,
	kMetaParams = 31,
	mPolySwitch = 31,
	mOversampleSwitch,
	mDistractKnob,
};

typedef struct Property Property;
struct Property {
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
const Property parameterProperties[kNumParams] = {
	{ PKnob5,	"octave",	20, 58,		0,	0, -2, 2	},
	{ PKnob,	"vco.int",	20, 222,	1,	0, -63, 63 },
	{ PKnob,	"porta",	20, 140,	2,	0, 0, 127	},
	{ PKnob,	"pitch.in",	98, 537,	3,	0, -63, 63	},
	{ PKnob,	"pitch1.in",184, 537,	4,	0, -63, 63	},

	{ PKnob,	"pitch2.in",270, 537,	5,	0, -63, 63	},
	{ PKnob4,	"vco1",		126, 58,	6,	1, 0, 3		},
	{ PKnob4,	"vco2",		220, 58,	7,	1, 0, 3		},
	{ PKnob,	"vco2pitch",220, 140,	8,	0, -63, 63	},
	{ PKnob,	"balance",	126, 140,	9,	0, 0, 127	},

	{ PSwitch2,	"vco2sync",	232, 218,	10,	1, 0, 1		},
	{ PKnob,	"cutoff",	324, 58,	11,	127, 0, 127	},
	{ PKnob,	"peak",		324, 140,	12, 0, 0, 127	},
	{ PSwitch3,	"vcf.type",	338, 306,	13, 0, 0, 2		},
	{ PKnob,	"eg.int",	324, 222,	14,	0, -63, 63	},

	{ PKnob,	"cutoff.in",356, 537,	15, 0, -63, 63	},
	{ PSwitch2,	"vca.eg",	232, 306,	16, 0, 0, 1		},
	{ PKnob,	"vca.in",	442, 537,	17, 0, -63, 63	},
	{ PKnob,	"drive",	126, 304,	18, 0, 0, 127	},
	{ PKnob,	"level",	20, 304,	19, 100, 0, 127	},

	{ PKnob,	"attack",	426, 58,	20, 0, 0, 127	},
	{ PKnob,	"decay",	426, 140,	21, 127, 0, 127	},
	{ PKnob,	"sustain",	426, 222,	22, 127, 0, 127	},
	{ PKnob,	"release",	426, 304,	23, 0, 0, 127	},
	{ PKnob,	"mg.freq",	12, 537,	24, 50, 0, 127	},

	{ PSwitch2, "mg.bpm",	20, 458,	25, 1, 0, 1		},
	{ PModsrc7, "modsrc0",	112, 628,	26, 0, 0, 6		},
	{ PModsrc7, "modsrc1",	198, 628,	27, 0, 0, 6		},
	{ PModsrc7, "modsrc2",	284, 628,	28, 0, 0, 6		},
	{ PModsrc7, "modsrc3",	370, 628,	29, 0, 0, 6		},

	{ PModsrc7, "modsrc4",	458, 628,	30, 0, 0, 6		},
	{ PVoices4, "poly",		19, 712,	-1,	0, 0, 3		},
	{ PSwitch2,	"oversample",368, 700,	-1, 0, 0, 1		},
	{ PKnob,	"distract",	432, 696,	-1, 127, 0, 127	},
};

enum ELayout
{
  kWidth = GUI_WIDTH,
  kHeight = GUI_HEIGHT,
  kKeybX = 62,
  kKeybY = 425
};



ds10::ds10(IPlugInstanceInfo instanceInfo) : IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo), lastVirtualKeyboardNoteNumber(virtualKeyboardMinimumNoteNumber - 1) {
  TRACE;

  print2_init();
  ds10state = ds10_init();
  mOversample = 1;
  mExtraRatio = 1;
  mSampleRate = 44100;
  

  CreateParams();
  CreateGraphics();
  CreatePresets();
  
  mMIDIReceiver.noteOn.Connect(this, &ds10::onNoteOn);
  mMIDIReceiver.noteOff.Connect(this, &ds10::onNoteOff);
}

ds10::~ds10() {
	ds10_exit(ds10state);
}


void ds10::onNoteOn(int noteNumber, int velocity) {
	ds10_noteon(ds10state, noteNumber, velocity);
}

void ds10::onNoteOff(int noteNumber, int velocity) {
	ds10_noteoff(ds10state, noteNumber);
}

void ds10::CreateParams() {
	for (int i = 0; i < kNumParams; i++) {
		IParam *param = GetParam(i);
		const Property *prop = &parameterProperties[i];

		switch (prop->type) {
		default:
		case PKnob:
			param->InitDouble(prop->name, prop->def, prop->min, prop->max, 0.01);
			break;
		case PSwitch2:
		case PSwitch3:
		case PKnob4:
		case PKnob5:
		case PVoices4:
		case PModsrc7:
			param->InitEnum(prop->name, prop->def - prop->min, prop->max - prop->min + 1);
			param->SetDisplayText(0, prop->name);
			break;
		}
	}
	for (int i = 0; i < kNumParams; i++) {
		OnParamChange(i);
	}
}

void ds10::CreateGraphics() {
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
		const Property *param = &parameterProperties[i];

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

void ds10::CreatePresets() {
	MakePreset("Init", 2, 0.0, 14.0, 4.0, 0.0, 0.0, 2, 1, 32.0, 60.0, 1, 57.0, 51.0, 0, 14.0, 0.0, 0, 0.0, 32.0, 105.0, 0.0, 66.0, 127.0, 74.0, 77.0, 1, 1, 0, 0, 0, 0, 0, 0, 127.0);
	MakePreset("XLEAD_", 2, 0.0, 14.0, 4.0, 0.0, 0.0, 2, 1, 32.0, 60.0, 1, 57.0, 51.0, 0, 14.0, 0.0, 0, 0.0, 32.0, 105.0, 0.0, 66.0, 127.0, 74.0, 77.0, 1, 1, 0, 0, 0, 0, 0, 0, 127.0);
	MakePreset("BASS01", 1, 0.0, 0.0, 0.0, 0.0, 0.0, 2, 1, 20.0, 41.0, 0, 15.0, 81.0, 0, 25.0, 0.0, 0, 0.0, 110.0, 105.0, 35.0, 71.0, 0.0, 64.0, 19.0, 1, 0, 0, 0, 0, 0, 0, 0, 127.0);
	MakePreset("WOUWOU", 1, 0.0, 0.0, 0.0, 0.0, 0.0, 1, 1, 20.0, 41.0, 0, 30.0, 93.0, 0, 25.0, 14.0, 0, 0.0, 41.0, 105.0, 38.0, 66.0, 127.0, 64.0, 19.0, 1, 0, 0, 0, 1, 0, 0, 0, 127.0);
	MakePreset("ABASS_", 1, 3.0, 0.0, 4.0, 0.0, 14.0, 2, 1, 25.0, 0.0, 1, 36.0, 33.0, 0, 11.0, 4.0, 0, 7.0, 38.0, 105.0, 0.0, 80.0, 0.0, 91.0, 48.0, 1, 1, 0, 0, 0, 0, 0, 0, 127.0);
	MakePreset("THROAT", 2, 0.0, 0.0, 0.0, 0.0, -6.0, 1, 1, 39.0, 61.0, 0, 26.0, 93.0, 0, 10.0, 6.0, 0, 0.0, 41.0, 105.0, 46.0, 72.0, 19.0, 77.0, 19.0, 1, 0, 0, 2, 1, 0, 0, 0, 127.0);
	MakePreset("SUBOSC", 1, 0.0, 0.0, 0.0, 0.0, 0.0, 2, 1, 32.0, 67.0, 1, 37.0, 71.0, 0, 13.0, 2.0, 0, -63.0, 47.0, 105.0, 0.0, 66.0, 127.0, 88.0, 77.0, 1, 0, 0, 0, 0, 0, 0, 0, 127.0);
	MakePreset("RESO__", 2, 0.0, 0.0, 7.0, 0.0, 0.0, 3, 3, 63.0, 0.0, 1, 21.0, 127.0, 2, 0.0, -4.0, 0, 0.0, 40.0, 105.0, 0.0, 62.0, 127.0, 79.0, 0.0, 1, 0, 0, 0, 0, 0, 0, 0, 127.0);
	MakePreset("JUNKTT", 2, 0.0, 0.0, 3.0, 0.0, 0.0, 1, 2, -32.0, 32.0, 1, 27.0, 79.0, 0, 19.0, 9.0, 1, 0.0, 3.0, 105.0, 70.0, 66.0, 127.0, 0.0, 45.0, 1, 1, 0, 0, 0, 0, 0, 0, 127.0);
	MakePreset("NFBASS", 1, 0.0, 0.0, 0.0, 0.0, 0.0, 1, 1, 20.0, 41.0, 0, 25.0, 93.0, 0, 25.0, 14.0, 0, 0.0, 41.0, 105.0, 38.0, 66.0, 127.0, 64.0, 19.0, 1, 0, 0, 0, 1, 0, 0, 0, 127.0);
	MakePreset("RESBAR", 2, 0.0, 0.0, 0.0, 0.0, 0.0, 1, 1, 3.0, 0.0, 1, 88.0, 86.0, 2, -23.0, 0.0, 0, 0.0, 0.0, 105.0, 0.0, 66.0, 127.0, 75.0, 19.0, 1, 0, 0, 0, 0, 0, 0, 0, 127.0);
	MakePreset("SQP___", 2, 0.0, 0.0, 0.0, 0.0, 0.0, 1, 1, 20.0, 0.0, 0, 29.0, 93.0, 0, 25.0, 6.0, 0, 0.0, 41.0, 105.0, 38.0, 66.0, 127.0, 64.0, 19.0, 1, 0, 0, 0, 1, 0, 0, 0, 127.0);
	MakePreset("BOC___", 2, 3.0, 0.0, 4.0, 0.0, 14.0, 2, 1, 25.0, 82.0, 1, 62.0, 0.0, 0, 11.0, 4.0, 0, 7.0, 38.0, 105.0, 0.0, 80.0, 0.0, 91.0, 48.0, 1, 1, 0, 0, 0, 0, 0, 0, 127.0);
	MakePreset("FATLED", 1, 2.0, 14.0, 5.0, 0.0, 0.0, 1, 1, 45.0, 58.0, 1, 79.0, 38.0, 0, 19.0, 5.0, 0, 0.0, 55.0, 105.0, 0.0, 45.0, 72.0, 99.0, 72.0, 1, 1, 0, 0, 0, 0, 0, 0, 127.0);
	MakePreset("FLYPOL", 2, 0.0, 15.0, 0.0, 0.0, 0.0, 1, 1, 3.0, 0.0, 1, 40.0, 35.0, 0, 28.0, 0.0, 0, 0.0, 28.0, 94.0, 0.0, 66.0, 127.0, 72.0, 19.0, 1, 0, 0, 0, 0, 0, 0, 0, 127.0);
	MakePreset("303AKO", 1, 1.0, 22.0, 37.0, 0.0, 0.0, 1, 1, 37.0, 0.0, 1, 12.0, 98.0, 0, 30.0, 11.0, 0, 0.0, 33.0, 105.0, 0.0, 96.0, 0.0, 85.0, 90.0, 0, 3, 0, 0, 0, 0, 0, 0, 127.0);
	MakePreset("THRMIN", 2, 1.0, 29.0, 5.0, 5.0, 0.0, 0, 1, 28.0, 0.0, 1, 127.0, 0.0, 0, 0.0, 0.0, 0, 0.0, 0.0, 105.0, 49.0, 66.0, 127.0, 69.0, 86.0, 1, 0, 1, 0, 0, 0, 0, 0, 127.0);
	MakePreset("DCHRD1", 3, 5.0, 0.0, 24.0, 6.0, 0.0, 1, 1, 16.0, 62.0, 1, 83.0, 74.0, 0, -41.0, 12.0, 0, 0.0, 0.0, 79.0, 0.0, 64.0, 0.0, 64.0, 93.0, 1, 3, 0, 0, 1, 0, 0, 0, 127.0);
	MakePreset("DCHRD2", 4, 0.0, 0.0, 25.0, 0.0, 0.0, 1, 1, 19.0, 49.0, 1, 41.0, 0.0, 0, 37.0, 3.0, 0, 0.0, 0.0, 105.0, 0.0, 72.0, 0.0, 0.0, 86.0, 1, 3, 0, 0, 1, 0, 0, 0, 127.0);
	MakePreset("DCHRD3", 2, 3.0, 0.0, 44.0, 0.0, 14.0, 2, 1, 40.0, 46.0, 1, 69.0, 87.0, 0, 20.0, 4.0, 0, 7.0, 38.0, 105.0, 0.0, 78.0, 0.0, 91.0, 95.0, 1, 3, 0, 0, 0, 0, 0, 0, 127.0);
	MakePreset("SCIFIZ", 2, 0.0, 0.0, 63.0, 0.0, 0.0, 2, 1, 17.0, 0.0, 1, 29.0, 98.0, 2, 35.0, 0.0, 0, 0.0, 0.0, 62.0, 49.0, 65.0, 0.0, 21.0, 49.0, 1, 5, 0, 0, 0, 0, 0, 0, 127.0);
	MakePreset("SCIFI1", 3, 0.0, 0.0, 21.0, 0.0, 0.0, 0, 1, 3.0, 0.0, 0, 42.0, 78.0, 0, 32.0, 0.0, 0, 0.0, 0.0, 105.0, 0.0, 94.0, 0.0, 0.0, 84.0, 1, 1, 0, 0, 0, 0, 0, 0, 127.0);
	MakePreset("SCIFI3", 1, 1.0, 0.0, 10.0, 0.0, 17.0, 2, 2, -1.0, 58.0, 1, 31.0, 103.0, 0, 39.0, 9.0, 1, 63.0, 0.0, 127.0, 0.0, 44.0, 0.0, 21.0, 34.0, 1, 6, 0, 4, 1, 0, 0, 0, 127.0);
	MakePreset("DOBREJ", 1, 0.0, 0.0, 0.0, 0.0, 0.0, 2, 1, 37.0, 53.0, 1, 16.0, 82.0, 1, 0.0, 23.0, 0, 0.0, 0.0, 105.0, 0.0, 66.0, 127.0, 64.0, 19.0, 1, 0, 0, 0, 1, 0, 0, 0, 127.0);
	MakePreset("TMP___", 2, 0.0, 20.0, 0.0, 0.0, 0.0, 1, 2, 37.0, 0.0, 1, 31.0, 72.0, 0, 27.0, 0.0, 0, 0.0, 28.0, 105.0, 34.0, 66.0, 127.0, 67.0, 19.0, 1, 0, 0, 0, 0, 0, 0, 0, 127.0);
}

void ds10::ProcessDoubleReplacing(
                                       double** inputs,
                                       double** outputs,
                                       int nFrames)
{
  // Mutex is already locked for us.
  
  double *leftOutput = outputs[0];
  double *rightOutput = outputs[1];

  for (int i = 0; i < nFrames; ++i) {
    mMIDIReceiver.advance();
    leftOutput[i] = rightOutput[i] = ds10_get_sample0(ds10state);
  }
  
  mMIDIReceiver.Flush(nFrames);
}

void ds10::SetSampleRate(void)
{
	ds10_set_resampler(ds10state, mOversample, mSampleRate, mExtraRatio);
}

void ds10::Reset()
{
  TRACE;
  IMutexLock lock(this);
  mSampleRate = GetSampleRate();

  SetSampleRate();
}

void ds10::OnParamChange(int paramIdx)
{
	
  IMutexLock lock(this);
  IParam *param = GetParam(paramIdx);
  const Property *prop = &parameterProperties[paramIdx];
  int val = param->Value();

  if (paramIdx < kMetaParams) {
	  /* mg.bpm has off in lower position */
	  if (prop->ds_id == 25 || prop->ds_id == 10)
		  val = !val;
	  printf("set %d to %d\n", prop->ds_id, val);
	  ds10_knob_all(ds10state, prop->ds_id, val);
  } else switch (paramIdx) {
  case mPolySwitch:
	  ds10_set_polyphony(ds10state, val + 1);
	  break;
  case mOversampleSwitch:
	  mOversample = !param->Int();
	  SetSampleRate();
	  break;
  case mDistractKnob:
	  mExtraRatio = pow(2, -(127 - param->Value()) / 127 * 4);
	  SetSampleRate();
	  break;
  }
}

void ds10::ProcessMidiMsg(IMidiMsg* pMsg) {
  mMIDIReceiver.onMessageReceived(pMsg);
}
