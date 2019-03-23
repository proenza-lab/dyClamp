/*
Copyright 2017. Niraj S. Desai, Richard Gray, and Daniel Johnston.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

// A fast, transient Na+ conductance using the Hodgkin-Huxley formalism.
// For speed, the parameters alphaM, betaM, alphaH, and betaH are pre-calculated
// and put in lookup tables stored as global variables.

// Declare the lookup table variables
float alphaM[1501] = {0.0};                          // Pre-calculate activation and inactivation parameters 
float betaM[1501] = {0.0};                           // for sodium currents: Vm from -100 mV to +50 mV in 
float alphaH[1501] = {0.0};                          // steps of 0.1 mV
float betaH[1501] = {0.0};

// Generate the lookup tables
void GenerateSodiumLUT() {
  float v;
  for (int x=0; x<1501; x++) {
    v = (float)x/10 - 100.0;                        // We use membrane potentials between -100 mV and +50 mV
    if (x==600) {                                   // The if-else statement makes sure alphaM stays finite at v = -40 mV.
      alphaM[x] = 1.0;
    } else {
      alphaM[x] = 0.1 * (-(v+40.0)) / ( expf( -(v+40.0)/10 ) - 1 );
    }
    betaM[x] = 4.0 * expf(-(v+65)/18);
    alphaH[x] = 0.07 * expf(-(v+65)/20);
    betaH[x] = 1 / ( expf(-(v+35)/10 ) + 1); 
  }
}

// At every time step, calculate the sodium current in the Hodgkin-Huxley manner
float Sodium(float v) {
  static float mNaVar = 0.0;    // activation gate
  static float hNaVar = 1.0;    // inactivation gate
  float v10 = v*10.0;
  int vIdx = (int)v10 + 1000;
  vIdx = constrain(vIdx,0,1500);
  mNaVar = mNaVar + msecs * ( alphaM[vIdx]*(1-mNaVar) - betaM[vIdx]*mNaVar );
  if (mNaVar < 0.0) mNaVar = 0.0;
  hNaVar = hNaVar + msecs * ( alphaH[vIdx]*(1-hNaVar) - betaH[vIdx]*hNaVar );
  if (hNaVar < 0.0) hNaVar = 0.0;
  float current1 = -conducts[2] * mNaVar * mNaVar * mNaVar * hNaVar * (v - 50);  // ENa = +50 mV
  return current1;
}

