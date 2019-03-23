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

// Each EPSC is triggered by a LOW --> HIGH transition at epscTriggerPin.
// The two-stage kinetic scheme is similar to how NMDA currents are handled 
// in Walcott, Higgins, and Desai, J. Neurosci. (2011).

volatile float xEPSC = 0.0;                // a pure number, EPSC intermediate gating variable

// Increment xEPSC by 1 every time a trigger arrives at epscTriggerPin
void UpdateEpscTrain() {
  xEPSC += 1;
}

// Calculate the net EPSC current at every time step
float EPSC(float v) {
  const float tauX = 1.0;                       // msec, rise time
  const float tauS = 10.0;                      // msec, decay time
  const float alphaS = 1.0;                     // number/msec, saturation level
  static float s = 0.0;           
  xEPSC = xEPSC + msecs * (-xEPSC/tauX);           
  s = s + msecs * (-s/tauS + alphaS*xEPSC*(1-s));  // forward Euler method
  float current = -conducts[7] * s * (v-0);           // reversal potential 0 mV
  return current;
}

