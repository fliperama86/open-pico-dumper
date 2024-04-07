from machine import Pin
from utime import sleep, sleep_ms, sleep_us

led_pin = Pin('LED', Pin.OUT)

pico_pins_map = [28, 27, 26, 22, 21, 20, 19, 18, 2, 3, 4, 5, 6, 7, 8, 9]
address_pins = [Pin(i, Pin.OUT) for i in pico_pins_map]

write_pin = Pin(10, Pin.OUT)
read_pin = Pin(11, Pin.OUT)
ale_low_pin = Pin(17, Pin.OUT)
ale_high_pin = Pin(16, Pin.OUT)

bit_array = [0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]

# while True:
led_pin.toggle()
for pin in [write_pin, read_pin, ale_low_pin, ale_high_pin]:
    pin.low()
    pin.high()
    pin.init(Pin.IN)

sleep_ms(1)
for pin in [write_pin, read_pin, ale_low_pin, ale_high_pin]:
    print(pin.value(), end='')
    