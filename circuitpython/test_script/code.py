import board
import digitalio
from analogio import AnalogIn
import time
import busio
import neopixel
import adafruit_bus_device
import microcontroller

# Should work with HA boards with v >=cir 0.2

# --------- I2C EEPROM Test ------------

def eeprom_test():
    i2c_temp = busio.I2C(board.SCL, board.SDA)  
    i2cdev = adafruit_bus_device.i2c_device.I2CDevice(i2c_temp, 0x50)

    i2c_pages = 3
    i2c_page_len = 8
    read_storage = []
    read_buffer = bytearray(8)

    # write data...
    with i2cdev as i2c:
        for i in range(i2c_pages):
            start = i * i2c_page_len
            data = list([start + j for j in range(i2c_page_len)])
            i2c.write(bytes([start] + data))
            time.sleep(0.005)

    # read it back...
    with i2cdev as i2c:
        for i in range(i2c_pages):
            start = i * i2c_page_len
            i2c.write_then_readinto(bytes([start]), read_buffer)
            time.sleep(0.005)
            read_storage = read_storage + list(read_buffer)

    # confirm that read data == written data
    if list(range(i2c_pages * i2c_page_len)) != read_storage:
        print("ERROR!!! Initial i2c writes failed.")
        return False

    read_storage.clear()

    # write DIFFERENT data...
    with i2cdev as i2c:
        for i in range(i2c_pages):
            start = i * i2c_page_len
            data = list([0xFF for j in range(i2c_page_len)])
            i2c.write(bytes([start] + data))
            time.sleep(0.005)

    # read it back...
    with i2cdev as i2c:
        for i in range(i2c_pages):
            start = i * i2c_page_len
            i2c.write_then_readinto(bytes([start]), read_buffer)
            time.sleep(0.005)
            read_storage = read_storage + list(read_buffer)

    # confirm that read data == written data
    # (we do this twice to ensure that data doesn't stick around between tests)
    if list([0xFF] * (i2c_page_len * i2c_pages)) != read_storage:
        print("ERROR!!! Secondary i2c writes failed.")
        return False

    return True

# --------- Entry Point ----------------

pixel = neopixel.NeoPixel(board.D3, 1, brightness=0.3, auto_write=False)

if not eeprom_test():
    print("EEPROM TEST FAILED!")
    while True:
        pixel[0] = (200, 200, 200)
        pixel.show()
        time.sleep(0.2)
        pixel[0] = (0, 0, 0)
        pixel.show()
        time.sleep(0.2)

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

foot_hold_count = 0

def reset_to_bootloader():
    microcontroller.on_next_reset(microcontroller.RunMode.UF2)
    microcontroller.reset()

while True:
    if (not boot_btn.value):
        reset_to_bootloader()
    if (tgl0.value != tgl0_v):
        tgl0_v = tgl0.value
        print("TGL0 switched:", tgl0_v)
    if (tgl1.value != tgl1_v):
        tgl1_v = tgl1.value
        print("TGL1 switched:", tgl1_v)
    if (foot_sw.value != foot_sw_v):
        foot_sw_v = foot_sw.value
        print("FOOT_SW switched:", foot_sw_v)
    foot_hold_count = foot_hold_count + 1 if foot_sw_v else 0
    if (foot_hold_count > 60):
        reset_to_bootloader()
    r = 0 if tgl0_v else 255
    g = 0 if tgl1_v else 255
    b = 255 if foot_sw_v else 0
    pixel[0] = (r,g,b)
    brightness = analog_in.value / 65536
    # easy way to visually tell if our pot max/min are good enough
    if (brightness >= 0.99):
        brightness = 0
    elif (brightness <= 0.01):
        brightness = 1.0
    pixel.brightness = brightness
    pixel.show()
    time.sleep(0.1)