//
//  PolyBLEPOscillator.h
//  SpaceBass
//
//  Created by Martin on 08.04.14.
//
//

#ifndef __SpaceBass__PolyBLEPOscillator__
#define __SpaceBass__PolyBLEPOscillator__

#include "Oscillator.h"

class PolyBLEPOscillator: public Oscillator {
public:
    PolyBLEPOscillator() : lastOutput(0.0) { updateIncrement(); };
    double nextSample();
private:
    double poly_blep(double t);
    double lastOutput;
};

#endif /* defined(__SpaceBass__PolyBLEPOscillator__) */
