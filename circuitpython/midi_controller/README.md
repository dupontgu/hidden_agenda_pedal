## Using the Hidden Agenda as a MIDI Controller
**IMPORTANT! This will overwrite the default firmware on your pedal! You will need to [reinstall](TODO) it to regain normal functionality.**

The MIDI controller firmware was implemented in [CircuitPython](https://www.circuitpython.org), which makes it super easy to install and customize!

## Installation
1. On your computer, download the latest CircuitPython 8 [UF2 file for the `Seeed Studio XIAO RP2040`](https://circuitpython.org/board/seeeduino_xiao_rp2040/) (That's the microcontroller board that powers this pedal!)
2. Download a recent version of the [Adafruit MIDI library (for Circuitpython 8)](https://github.com/adafruit/Adafruit_CircuitPython_MIDI/releases/download/1.4.16/adafruit-circuitpython-midi-8.x-mpy-1.4.16.zip).
3. Download the `code.py` file from this directory.
4. Unscrew and remove the bottem panel of your pedal.
5. Plug the pedal into your computer using the USB-C port.
6. Press and release the "Boot" button on the inside (bottom) of the pedal. You'll need to unscrew/remove the bottom panel.
7. Observe that the pedal is now mounted as a disk drive on your computer (It will likely be called `RPI-RP2`).
8. Drag/drop the UF2 file you downloaded in step 1 onto the new disk drive. The pedal should reboot and remount as a drive called `CIRCUITPY`.
9. Drag/drop the entire `adafruit_midi` directory from the `lib` directory of the Adafruit MIDI library (step 2) to the `lib` directory on `CIRCUITPY`.
10. Drag/drop the `code.py` file you downloaded in step 3 onto the `CIRCUITPY` drive.
11. As soon as the file is copied over, the pedal should show up as a USB MIDI controller!

## Usage
When the MIDI code from this directory is used, the pedal will send/receive MIDI CC messages:

**`Toggle Switch`**
- Will send value 127 to MIDI CC 110 if the switch is moved to the up position, will send value 0 when it is moved elsewhere.
- Will send value 127 to MIDI CC 111 if the switch is moved to the down position, will send value 0 when it is moved elsewhere.

**`Footswitch`**
- Will send value 127 to MIDI CC 112 when it is pushed down, will send value 0 when it is released. (It is a momentary switch!)

**`Knob`**
- Will send values 0-127 to MIDI CC 113 as it is turned from left to right.

**`LED`**
- By default, the LED will show the status of the 3 parameters above. The red, green, and blue components of the color will represent the states of the toggle/foot switches, and the knob will control the brightness.
- You can override the LED color by sending MIDI CC messages _to_ the pedal from your MIDI host:
  - MIDI CC 110 will change the red component of the LED color.
  - MIDI CC 111 will change the green component of the LED color.
  - MIDI CC 112 will change the blue component of the LED color.
