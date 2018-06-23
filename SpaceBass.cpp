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

enum EParams
{
	kNumParams = 26 + 7 - 1,
	kMetaParams = 31,
	mPolySwitch = 31,
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
	{ PSwitch2,	"vco2sync",	232, 218,	10,	0, 0, 1		},
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
	{ PKnob,	"mg.freq",	12, 537,	24, 0, 0, 127	},
	{ PSwitch2, "mg.bpm",	20, 458,	25, 1, 0, 1		},
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
		IParam *param = GetParam(i);
		const Property *prop = &parameterProperties[i];
		sprintf(name, "prop%d", i);

		switch (prop->type) {
		default:
		case PKnob:
			param->InitDouble(name, prop->def, prop->min, prop->max, 0.01);
			break;
		case PSwitch2:
		case PSwitch3:
		case PKnob4:
		case PKnob5:
		case PVoices4:
		case PModsrc7:
			param->InitEnum(name, prop->def - prop->min, prop->max - prop->min + 1);
			param->SetDisplayText(0, prop->name);
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
  IParam *param = GetParam(paramIdx);
  const Property *prop = &parameterProperties[paramIdx];

  //printf("param %d: %lf\n", paramIdx, param->Value());
  //param->Int(); param->Int();
  if (paramIdx < kMetaParams) {
	  int val = param->Value();
	  /* mg.bpm has off in lower position */
	  if (prop->ds_id == 25)
		  val = !val;
	  ds10_knob_all(prop->ds_id, val);
  } else switch (paramIdx) {
  }
}

void SpaceBass::ProcessMidiMsg(IMidiMsg* pMsg) {
  mMIDIReceiver.onMessageReceived(pMsg);
}