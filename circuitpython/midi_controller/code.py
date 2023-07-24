import board
import digitalio
from analogio import AnalogIn
import time
import neopixel
import microcontroller
import adafruit_midi
from adafruit_midi.control_change import ControlChange
import usb_midi

# Should work with HA boards with v >=cir 0.2

pixel = neopixel.NeoPixel(board.D3, 1, brightness=0.0, auto_write=False)

tgl0 = digitalio.DigitalInOut(board.D8)
tgl0.pull = digitalio.Pull.UP
tgl0_v = tgl0.value

tgl1 = digitalio.DigitalInOut(board.D7)
tgl1.pull = digitalio.Pull.UP
tgl1_v = tgl1.value

boot_btn = digitalio.DigitalInOut(board.D6)
boot_btn.pull = digitalio.Pull.UP

foot_sw = digitalio.DigitalInOut(board.A2)
foot_sw.pull = digitalio.Pull.UP
foot_sw_v = foot_sw.value

analog_in = AnalogIn(board.A0)
pot_v_9bit = analog_in.value >> 7
pot_v_7bit = pot_v_9bit >> 2
color_override = None

midi = adafruit_midi.MIDI(midi_in=usb_midi.ports[0], midi_out=usb_midi.ports[1],
                          out_channel=0)

while True:
    event = midi.receive()
    while isinstance(event, ControlChange):
        color_copy = color_override or [0,0,0]
        should_update = True
        if event.control == 110:
            color_copy[0] = event.value * 2
        elif event.control == 111:
            color_copy[1] = event.value * 2
        elif event.control == 112:
            color_copy[2] = event.value * 2
        else:
            should_update = False
        if should_update:
            print("UPDATED COLOR:",event, color_copy)
            color_override = color_copy
        event = midi.receive()
    if (not boot_btn.value):
        microcontroller.on_next_reset(microcontroller.RunMode.UF2)
        microcontroller.reset()
    if (tgl0.value != tgl0_v):
        tgl0_v = not tgl0_v
        midi.send(ControlChange(110, 0 if tgl0_v else 127))
        print("TGL0 switched:", tgl0_v)
    if (tgl1.value != tgl1_v):
        tgl1_v = not tgl1_v
        midi.send(ControlChange(111, 0 if tgl1_v else 127))
        print("TGL1 switched:", tgl1_v)
    if (foot_sw.value != foot_sw_v):
        foot_sw_v = not foot_sw_v
        midi.send(ControlChange(112, 0 if foot_sw_v else 127))
        print("FOOT_SW switched:", foot_sw_v)
    
    # take a few readings and average them for a tiny bit of filtering
    # analog_in.value is an unsigned, 16 bit value
    pot_readings = []
    pot_sample_count = 15
    for _ in range(pot_sample_count):
        pot_readings.append(analog_in.value >> 7)
        time.sleep(0.001)
    current_pot_v_9bit = round(sum(pot_readings) / pot_sample_count)
    # it tends to flicker between adjacent values, 
    # so safe to move forward once the value changes by a magnitude >= 3
    if (abs(current_pot_v_9bit - pot_v_9bit) >= 3):
        pot_v_9bit = current_pot_v_9bit
        current_pot_v_7bit = current_pot_v_9bit >> 2
        # avoid sending the same value twice
        if (current_pot_v_7bit != pot_v_7bit):
            pot_v_7bit = current_pot_v_7bit
            midi.send(ControlChange(113, pot_v_7bit))
            print("POT adjusted:", pot_v_7bit)

    r = 0 if tgl0_v else 255
    g = 0 if tgl1_v else 255
    b = 255 if foot_sw_v else 0
    pixel[0] = color_override or (r,g,b)

    pixel.brightness = 1.0 if color_override is not None else analog_in.value / 65536
    pixel.show()