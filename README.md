# dyClamp
## A fast dynamic clamp sketch for the pyClamp interface


dyClamp is a reimplementation of [dynamic_clamp](https://github.com/nsdesai/dynamic_clamp) with a focus on a robust serial communication between the Teensy and its controlling host: The current implementation allows scientists to alter the Teensy's behavior at runtime - by transmitting updates for calibration parameters, conductance values or by triggering custom events. Furthermore, the low-latency design of [Teensyduino USB Serial Communication](https://www.pjrc.com/teensy/td_serial.html) makes contiuous flow of information from and to the host possible - with a minimal effect on the cycle times of the dynamic clamp system.

![Screenshot](https://github.com/christianrickert/dyClamp/blob/master/dyClamp.png)

## Serial communication

In short, information exchanged between the Teensy and its host via the serial connection follows a simple formal convention: A new string of information begins with a carriage return character `<cr>` and ends with a linefeed character `<lf>`, while values embedded in this frame are separated by tabulator characters `<tab>`.

Example:
```
<cr> value1 <lf>
<cr> value1 <tab> value2 <lf>
<cr> value1 <tab> value2 <tab> value3 <lf>
```

### Serial command input

The Teensy will interpret information strings ("commands") from the controlling host based on `value1` and `value2`. The first value (index) indicates the type of command, while the second value represents an updated value or a pre-defined command case.


- All positive command indices are indicating update commands for the array of calibration parameters.
```
// This command changes the first value (at index 0) of the array of calibration parameters to "1.234".
<cr> 1.0 <tab> 1.234 <lf>
```

- All negative command indices are indicating update commands for the array of coductance values.
```
// This command changes the third value (at index 2) of the array of coductance values to "5.678".
<cr> -3.0 <tab> 5.678 <lf>
```

- All command indices with a value of zero are generally interpreted as execution commands, with the second value indicating the subtype of the command.
```
/* This command does nothing by itself, except that it produces an echo to the host - see below.
   However, this command can be useful to "ping" the device when establishing a new connection. */
<cr> 0.0 <tab> 0.0 <lf>

/* Sends calibration parameters and conductance values to the host.
   These values can be interpreted by the host to confirm a successful transmission. */
<cr> 0.0 <tab> 1.0 <lf>

/* Toggles live reports of selected values on/off that are sent to the host continuously.
   The data stream can contain an arbitrary number of values for monitoring. */
<cr> 0.0 <tab> 2.0 <lf>
```
All command strings are echoed to the controlling host to indicate a successful transmission of the command.

### Serial command output

Besides the echo of a command string that is exclusively sent on-demand, the Teensy can provide a continuous stream of information to its host upon request.
```
/* Toggles live reports of selected values on/off that are sent to the host continuously.
   The data stream can contain an arbitrary number of values for monitoring. */
<cr> 0.0 <tab> 2.0 <lf>
```
The Teensy will start to report data values live that representing the current state of internal parameters.
The transmitted data can be useful to observe parameters during experiments or to debug new current models.
```
/* Continuous stream of information from the Teensy, reporting the current values of
   the membrane potential [mV], the injected current [pA], and the cycle time [µs]. */
<cr> -65.1 <tab> 1.1 <tab> 9.0 <lf>
<cr> -64.9 <tab> 1.3 <tab> 10.0 <lf>
<cr> -64.7 <tab> 1.4 <tab> 9.0 <lf>
<cr> -65.4 <tab> 1.0 <tab> 9.0 <lf>
...
```
In order to reduce information overhead and therefore transmission latency, the information in the report strings is encoded positionally - in contrast to the command strings that use indices for individual value updates.

Furthermore, the current implementation temporarily discontinues the generation of live reports when a new command string has arrived. Thus avoiding interpretation conflicts on the host side (i.e. a command string echo being interpreted as a live report with two values).

In addition to the serial communication tweaks mentioned above, dyClamp is dynamically throttling its generation of live reports  based on the availability of the serial output buffer. While this behaviour does not guarantee a prompt transmission to the host sytem, it makes sure that data generation is synchronous with data representation.
