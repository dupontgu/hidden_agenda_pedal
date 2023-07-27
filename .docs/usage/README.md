# Hidden Agenda Usage

## The Serial Console
In addition to acting as a USB keyboard/house, the Hidden Agenda can act as a USB serial device. You can connect to the serial console using a terminal application of your choosing. While connected, the pedal will emit log messages and can receive commands to change certain settings. 

If you don't know how to talk to a USB serial device, I highly recommend Adafruit's tutorials for [Windows](https://learn.adafruit.com/welcome-to-circuitpython/advanced-serial-console-on-windows), [macOS](https://learn.adafruit.com/welcome-to-circuitpython/advanced-serial-console-on-mac-and-linux), and [Linux](https://learn.adafruit.com/welcome-to-circuitpython/advanced-serial-console-on-linux). Ensure that your pedal is plugged into your computer using the USB-C port before attempting to connect!


All supported commands are documented below. Each command is sent to the device by first typing `cmd:`, then the name of the setting you are trying to change, then the value(s) you'd like to set, then hitting enter. The items should be separated by colons.

### `boot`
* Resets the device into USB bootloader mode so you can update/change the firmware.
* (No parameters)
* example: `cmd:boot`

### `reset`
* Sets all following settings to their default hardcoded values
* (No parameters)
* example: `cmd:reset`

### `brightness`
* Sets the maximum brightness of the onboard LED, as a percentage
* parameter: an integer between 1-100 (inclusive) representing the brightness percentage
* defaults to 70%
* example: `cmd:brightness:80` (sets the maximum brightness to 80%)

### `flash`
* Sets whether or not the onboard LED should blink/flash/change brightness while in use, or remain static.
* parameter: either `on` or `off`
* defaults to `on`
* example: `cmd:flash:off` (sets the maximum brightness to 80%)

### `set_color`
* Sets the LED color for a given FX slot.
* parameter 1: index of the FX slot you want to change the color of (1-4)
* parameter 2: 24-bit [hex RGB color](https://htmlcolorcodes.com/)
* default values are: `{0xFF4000, 0x4000FF, 0x00FF40, 0xAA0070}`
* example: `cmd:set_color:1:FF0000` (sets the the color for the first FX slot to red)

### `raw_hid`
* Sets whether or not the pedal should log all _incoming_ HID messages from connected keyboards/mice (as hex strings).
* parameter: either `on` or `off`
* defaults to `off`
* example: `cmd:raw_hid:on` (turns on HID logging - all incoming HID messages will be printed to the serial console)

