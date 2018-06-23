#ifndef __SPACEBASS__
#define __SPACEBASS__

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wextra-tokens"
#include "IPlug_include_in_plug_hdr.h"
#pragma clang diagnostic pop

#include "MIDIReceiver.h"

extern "C" {
#include "ds10-core.h"
};

class SpaceBass : public IPlug
{
public:
  SpaceBass(IPlugInstanceInfo instanceInfo);
  ~SpaceBass();

  void SetSampleRate();

  void Reset();
  void OnParamChange(int paramIdx);
  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);
  // to receive MIDI messages:
  void ProcessMidiMsg(IMidiMsg* pMsg);
  
  // Needed for the GUI keyboard:
  // Should return non-zero if one or more keys are playing.
  inline int GetNumKeys() const { return mMIDIReceiver.getNumKeys(); };
  // Should return true if the specified key is playing.
  inline bool GetKeyStatus(int key) const { return mMIDIReceiver.getKeyStatus(key); };
  static const int virtualKeyboardMinimumNoteNumber = 48;
  int lastVirtualKeyboardNoteNumber;
  void onNoteOn(int noteNumber, int velocity);
  void onNoteOff(int noteNumber, int velocity);
private:
  Ds10State *ds10state;
  double mFrequency;
  void CreatePresets();
  MIDIReceiver mMIDIReceiver;
  int mOversample;
  double mSampleRate;
  double mExtraRatio;
  IControl* mVirtualKeyboard;
  void processVirtualKeyboard();

  
  double filterEnvelopeAmount;
  
  double lfoFilterModAmount;
  
  void CreateParams();
  void CreateGraphics();
};

#endif
