from machine import Pin
from time import sleep

led_pin = Pin('LED', Pin.OUT)

pico_pins_map = [28, 27, 26, 22, 21, 20, 19, 18, 2, 3, 4, 5, 6, 7, 8, 9]
address_pins = [Pin(i, Pin.OUT) for i in pico_pins_map]

write_pin = Pin(10, Pin.OUT)
read_pin = Pin(11, Pin.OUT)
ale_low_pin = Pin(17, Pin.OUT)
ale_high_pin = Pin(16, Pin.OUT)

# while True:
led_pin.toggle()
for pin in [write_pin, read_pin, ale_low_pin, ale_high_pin]:
    pin.value(1)
    # sleep(1)