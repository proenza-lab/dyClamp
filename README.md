# dyClamp
## A fast dynamic clamp sketch for the pyClamp interface


dyClamp is a reimplementation of [dynamic_clamp](https://github.com/nsdesai/dynamic_clamp) with a focus on a robust serial communication between the Teensy and its controlling host: The current implementation allows scientists to alter the Teensy's behavior at runtime - by transmitting updates for calibration parameters, conductance values or by triggering custom events. Furthermore, the low-latency design of [Teensyduino USB Serial Communication](https://www.pjrc.com/teensy/td_serial.html) makes contiuous flow of information from and to the host possible - with a negligible effect on dynamic clamp cycle times.

![Screenshot](https://github.com/christianrickert/dyClamp/blob/master/dyClamp.png)

## Serial command structure

In short, information exchanged between the Teensy and its host via the serial connection follows a simple formal convention. A new string of information begins with a carriage return character `<cr>` and ends with a linefeed character `<lf>`, while values embedded in this frame are separated by tabulator characters `<tab>`:

`<cr> value1 <tab> value2 <lf>`
`<cr> value1 <tab> value2 <tab> value3 <lf>`

...
