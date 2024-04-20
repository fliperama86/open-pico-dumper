from machine import Pin
from utime import sleep_us

address_pins = [Pin(i, Pin.OUT) for i in range(24)]

# Control pins
control_pins = {
  'rst_pin': Pin(24, Pin.OUT),
  'cs_pin': Pin(25, Pin.OUT),
  'wrl_pin': Pin(26, Pin.OUT),
  'wrh_pin': Pin(27, Pin.OUT),
  'oe_pin': Pin(28, Pin.OUT)
}

time_pin = Pin(29, Pin.OUT)
as_pin = Pin(30, Pin.OUT)
asel_pin = Pin(31, Pin.OUT)

def set_pico_address_pins_out():
  for pin in address_pins:
    pin.init(Pin.OUT)
    pin.low()

def set_pico_address_pins_in():
  for pin in address_pins:
    pin.init(Pin.IN)

def setup_cartridge():
  set_pico_address_pins_out()
  
  # Setting RST(PH0) CS(PH3) WRH(PH4) WRL(PH5) OE(PH6) HIGH
  for pin in control_pins.values():
    pin.high()
  
  # Setting TIME and AS HIGH
  time_pin.high()
  as_pin.high()
  
  # Setting ASEL HIGH
  asel_pin.high()
  sleep_us(1)

def get_cart_info():
  pass