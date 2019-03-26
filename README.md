# dyClamp
## A fast dynamic clamp sketch for the pyClamp interface


dyClamp is a reimplementation of [dynamic_clamp](https://github.com/nsdesai/dynamic_clamp) with a focus on a robust serial communication between the Teensy and its controlling host: The current implementation allows scientists to alter the Teensy's behavior at runtime - by transmitting updates for calibration parameters, conductance values or by triggering custom events. Furthermore, the low-latency design of [Teensyduino USB Serial Communication](https://www.pjrc.com/teensy/td_serial.html) makes contiuous flow of information from and to the host possible - with a negligible effect on dynamic clamp cycle times.

![Screenshot](https://github.com/christianrickert/dyClamp/blob/master/dyClamp.png)

## Serial communication

In short, information exchanged between the Teensy and its host via the serial connection follows a simple formal convention. A new string of information begins with a carriage return character `<cr>` and ends with a linefeed character `<lf>`, while values embedded in this frame are separated by tabulator characters `<tab>`:

Example:
```
<cr> value1 <lf>
<cr> value1 <tab> value2 <lf>
<cr> value1 <tab> value2 <tab> value3 <lf>
```

### Serial command input

The Teensy will interpret information strings ("commands") from the controlling host based on `value1` and `value2`. The first value (index) indicates the type of command, while the second value represents an updated value or a pre-defined command case.


- All positive command indices are indicating update commands of the array of calibration parameters.
```
// This command changes the first value (at index 0) of the array of calibration parameters to "1.234".
<cr> 1.0 <tab> 1.234 <lf>
```

- All negative command indices are indicating update commands of the array of coductance values.
```
// This command changes the third value (at index 2) of the array of coductance values to "5.678".
<cr> -3.0 <tab> 5.678 <lf>
```

- All command indices with a value of zero are interpreted as execution commands, with the second value indicating the exact type of command.
```
// This command does nothing itself, except that it produces an echo to the host - like all commands. However, this command can be used to "ping" the device when establishing a new connection. It is the first command in a switch...case structure of pre-defined commands.
<cr> 0.0 <tab> 0.0 <lf>
```

All command strings are echoed to the controlling host to indicate a successful transmission of the command.

### Serial command output

Besideds the just mentioned echo of a command string, the Teensy can provide a continuous flow of information to its host.

Example:
```
// This command toggles the live report of an array of pre-defined values on or off. It is the third type of command (counting starts at zero) in a switch...case structure of pre-defined commands.
<cr> 0.0 <tab> 2.0 <lf>
```
All command indices with value zero are interpreted as command switches, with the second value indicating the type of command.
