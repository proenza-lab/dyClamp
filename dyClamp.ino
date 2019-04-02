/*
dyClamp (dynamic clamp sketch for the pyClamp interface)
Copyright (C) 2019 Christian Rickert <mail@crickert.de>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
MA 02110-1301, USA.

dyClamp was partially developed at Laboratory of Catherine Proenza,
Department of Physiology & Biophysics, University of Colorado,
Anschutz Medical Campus.

This project is based on the original implementation for a dynamic clamp system
by Niraj S. Desai, Richard Gray, and Daniel Johnston. For details, please visit
their website: https://dynamicclamp.com.

Version 1.0
*/


// imports

#include <math.h>               // enable floating point unit functions


// variables

bool report = false;            // report values live
int adcs = 0;                   // raw value from analog in [1]
int count = 0;                  // cycle counter for benchmarking
int dacs = 0;                   // raw value from analog out [1]
float pamps = 0.0;              // current output [pA]
float msecs = 0.0;              // time past since last cycle (dt) [ms]
float mvolts = 0.0;             // membrane potential [mV]

/* extend this array to add calibration parameters,
   use decrementing negative indices to address elements */
float calibras[] =  {50.0,      // Amplifier input gain (AMP_i) [mV/mV]
                     400.0,     // Amplifier output gain (AMP_o) [pA/V]
                     5.5,       // ADC input slope (ADC_m) [mV/1]
                     -11500.0,  // ADC input intercept (ADC_n) [mV]
                     750.0,     // DAC output slope (DAC_m) [1/pA]
                     2000.0,    // DAC output intercept (DAC_n) [0-4095]
                     0.0};      // Voltage offset (VLT_d) [mV]

/* extend this array to add conductance parameters,
   use incrementing postive indices to address elements */
float conducts[] =  {0.0,       // G_Shunt [nS]
                     0.0,       // G_H [nS]
                     0.0,       // G_Na [nS]
                     0.0,       // OU1_m [nS]
                     0.0,       // OU1_D [nS^2/ms]
                     0.0,       // OU2_m [nS]
                     0.0,       // OU2_D [nS^2/ms]
                     0.0};      // G_EPSC [nS]

/* extend this array to add values for live reports */
float values[] =    {0.0,       // mvolts [mV]
                     0.0,       // pamps [pA]
                     0.0};      // msecs [µs]

elapsedMicros stepTime = 0;     // individual cycle time [µs]
elapsedMillis cmdTime = 0;      // time since alst command check [ms]
elapsedMillis ttlTime = 0;      // time since last TTL trigger [ms]

/* serial command strings are identified by their specific format:

    <\r> idx <\t> val <\n>
    
    index:  int
            {-inf, ..., -1} = calibras[+inf-1, ..., +0]
            {0}             = execute a command
            {+1, ..., +inf} = conducts[ 0, ..., inf-1]
    value:  float
            the value written into the specified array and position or
            the command to be executed from a switch...case structure
    
    Examples:

    <\r> -2 <\t> 2000.0 <\n>    changes the second parameter (Amp_o) to 2000.0
    <\r>  0 <\t>    2.0 <\n>    toggles live reports on or off
    <\r>  1 <\t>   10.0 <\n>    changes the first conductance (G_Shunt) to 10.0 */

/* custom struct for serial communication */
typedef struct serialCmd {
    int len = 0;
    int idx = 0;
    int sep = 0;
    float val = 0.0;
    String str = "";
} command;
command cmd;                    // serial command string struct

// constants
const int adcPin = 0;           // analog in pin identifier
const int cmdIntvl = 20;        // command interval [ms]
const int dacPin = A21;         // analog out pin identifier
const int ttlPin = 2;           // trigger pin identifier
const int lenCals = sizeof(calibras)/sizeof(calibras[0]);
const int lenCons = sizeof(conducts)/sizeof(conducts[0]);
const int lenVals = sizeof(values)/sizeof(values[0]);


// functions

/* clear serial COM port buffers */
void clearBuffers() {
    Serial.flush();                     // clear write buffer
    while (Serial.available() > 0)      // clear read buffer
        Serial.read();
}

/* interprets a command string */
void interpretString(command cmd) {
    cmd.sep = cmd.str.indexOf('\t');
    if (cmd.str.startsWith('\r') && cmd.sep != -1) {  // check format
        cmd.len = cmd.str.length();
        cmd.idx = cmd.str.substring(1,cmd.sep).toInt();
        if (cmd.idx < 0) {          // negative index, update parameters
            cmd.idx = -1 * cmd.idx - 1;
            cmd.val = cmd.str.substring(cmd.sep + 1, cmd.len).toFloat();
            setParameter(cmd.idx, cmd.val);
        } else if (cmd.idx > 0) {   // positive index, update conductances
            cmd.idx = cmd.idx - 1;
            cmd.val = cmd.str.substring(cmd.sep + 1, cmd.len).toFloat();
            setConductance(cmd.idx, cmd.val);
        } else {                    // zero index, execute command instead
            cmd.val = cmd.str.substring(cmd.sep + 1, cmd.len).toInt();
            runCommand(cmd.val);
        }
    }
}

/* receives a string from the serial COM port */
void readString() {
    cmd.str = Serial.readStringUntil('\n') + '\n';  // avoid timeout
}

/* runs predefined commands */
void runCommand(int exec) {
    static bool troper = false;  // remember previous live report state
    troper = report;
    if (report)     // temporarily turn off live reports
        report = false;
    switch (exec) {
        case 0:     // echo test, nothing to do
            break;
        case 1:     // send calibration parameters and conductance values
            writeString(newArray(2));
            break;
        case 2:     // toggle live reports on or off
            report = invertBoolean(troper);
            break;
        default:    // every other (undefined) command
            break;
    }
    if (exec != 2)  // return to previous live report state
        report = troper;
}


/* updates conductance values */
void setConductance(int idx, float val) {
    if (idx < lenCons)
        conducts[idx] = val;
}

/* updates parameter values */
void setParameter(int idx, float val) {
    if (idx < lenCals)
        calibras[idx] = val;
}

/* sends a string to the serial COM port */
void writeString(String str) {
    Serial.print(str);
}

/* returns an inverted boolean value */
bool invertBoolean(bool boo) {
    if (boo) {
        boo = false;
    } else {
        boo = true;
    }
    return boo;
}

/* serial transmission strings are identified by their specific order:
    
    <\r> vals[0] <\t> vals[1] (<\t> vals[2] ... <\t> vals[inf] ) <\n>
    
    values: float[]
    
    Examples:

    <\r> 1.23 <\t> 1337 <\n>    reports two values, which are identified
                                by their order in the transmission string */

/* assembles calibration parameters and conductance values for transmission */
String newArray(int len) {
    String valstr = "";  // initialize before concatenation
    valstr = "";
    for (int i = lenCals-1; i >= 0; i--) {
        int j = -1 * (i + 1);
        values[0] = j;
        values[1] = calibras[i];
        valstr += newString(values, len);
    }
    for (int k = 0; k < lenCons; k++) {
        int l = k + 1;
        values[0] = l;
        values[1] = conducts[k];
        valstr += newString(values, len);
    }
    return valstr;
}

/*creates a new command/report string from an array of values */
String newString(float vals[], int len) {
    static const String cr = '\r';
    static const String tb = '\t';
    static const String lf = '\n';
    static String str;
    str = cr;
    for (int i = 0; i < len; i++ ) {
        if (i > 0)
            str += tb;
        str += vals[i];
    }
    str += lf;
    return str;
}


/* setup, run once */
void setup(){

    // configure serial COM port
    Serial.begin(115200);       // [b/s]
    Serial.setTimeout(1);       // [ms]
    clearBuffers();

    // configure Teensy pins
    analogReadResolution(12);   // (0-4095)
    analogWriteResolution(12);  // (0-4095)
    pinMode(ttlPin, INPUT);     // set to high-impedance state

    // pre-calculate lookup tables
    GenerateGaussianNumbers();  // Gaussian number pool for use by the OU processes
    GenerateSodiumLUT();        // sodium activation/inactivation
    GenerateHcnLUT();           // HCN activation

 }


/* loop, run continuously */
 void loop() {

    // read membrane potential (slow, ~8.0µs)
    adcs = float(analogRead(adcPin));
    mvolts = calibras[2] / calibras[0] * adcs + calibras[3] / calibras[0] + calibras[6];

    // calculate cycle interval
    msecs = 0.001 * float(stepTime);
    stepTime = 0;

    // reset current
    pamps = 0.0;

    // add Shunting current (fast, < 0.5µs)
    if (conducts[0] > 0) {
        pamps += Shunting(mvolts);
    }

    // add HCN current (fast, < 0.5µs)
    pamps += HCN(mvolts, conducts[1]);  // reset current after initial run
    
    // add Sodium current (fast, < 0.5µs)
    if (conducts[2] > 0) {
        pamps += Sodium(mvolts);
    }
    
    // add OrnsteinUhlenbeck current (medium, ~1.5µs)
    if (conducts[3] > 0 || conducts[5] > 0) {
        pamps += OrnsteinUhlenbeck(mvolts);
    }
    
    // add EPSC current (fast, ~0.5µs)
    if (conducts[7] > 0) {
        if ((digitalReadFast(ttlPin) == HIGH) && (ttlTime > 2)) {  // check for TTL signal
            UpdateEpscTrain();
            ttlTime = 0;
        }
        pamps += EPSC(mvolts);
    }

    // write calculated current (fast, ~0.5µs)
    dacs = int(calibras[4] / calibras[1] * pamps + calibras[5]);
    dacs = constrain(dacs, 0, 4095);    // keep constrain function simple
    analogWrite(dacPin, dacs);

    // update commands & report values
    if (cmdTime > cmdIntvl) {
        if (Serial.available() > 4) {   // read buffer not empty
            readString();
            writeString(cmd.str);
            interpretString(cmd);
        }
        if (report && Serial.availableForWrite() > 63) {  // write buffer empty
            values[0] = mvolts;
            values[1] = pamps;
            values[2] = 1000.0 * msecs;
            writeString(newString(values, lenVals));
        }
        cmdTime = 0;
    }

}
