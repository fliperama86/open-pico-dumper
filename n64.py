from machine import Pin
import utime

rom_base_address = 0x10000000
cart_size = 12
file_path = "dump.n64"
led_pin = Pin(25, Pin.OUT)

pico_pins_map = [28, 27, 26, 22, 21, 20, 19, 18, 2, 3, 4, 5, 6, 7, 8, 9]
address_pins = [Pin(i, Pin.OUT) for i in pico_pins_map]

write_pin = Pin(10, Pin.OUT)
read_pin = Pin(11, Pin.OUT)

ale_low_pin = Pin(17, Pin.OUT)
ale_high_pin = Pin(18, Pin.OUT)

def set_address_pins_out():
  for pin in address_pins:
    pin.init(Pin.OUT)
    pin.low()

def set_address_pins_in():
  for pin in address_pins:
    pin.init(Pin.IN, Pin.PULL_DOWN)

def set_address(address):
  set_address_pins_out()

  address_low_out = address & 0xFFFF
  address_high_out = address >> 16

  write_pin.high()
  read_pin.high()
  ale_low_pin.high()
    
  utime.sleep_us(1)
  ale_high_pin.high()
    
  for i in range(0, 16, 1):
    bit = (address_high_out >> i) & 1
    address_pins[15 - i].value(bit)

  utime.sleep_us(1)
  
  ale_high_pin.low()

  for i in range(0, 16, 1):
    bit = (address_low_out >> i) & 1
    address_pins[15 - i].value(bit)
  
  utime.sleep_us(1)
  ale_low_pin.low()
  
  utime.sleep_us(1)
  
  set_address_pins_in()

def read_word():
  read_pin.low()
  utime.sleep_us(1)
  
  word = 0
  
  for i, pin in enumerate(address_pins):
    if pin.value():
      word |= 1 << i
      
  ale_high_pin.high()
  
  utime.sleep_us(1)
  
  return word

def read_cart():
  with open(file_path, "wb") as file:
    write_buffer = bytearray(512)

    # Read the data in 512 byte chunks
    for rom_address in range(rom_base_address, rom_base_address + (cart_size * 1024 * 1024), 512):
      # # Blink led
      if (rom_address & 0x3FFF) == 0:
        led_pin.high()
      else:
        led_pin.low()

      # Set the address for the next 512 bytes
      set_address(rom_address)

      for bufferIndex in range(0, 512, 2):
        word = read_word()
        write_buffer[bufferIndex] = word >> 8
        write_buffer[bufferIndex + 1] = word & 0xFF

      file.write(write_buffer)
      
      print("write_buffer:")
      print_hex(write_buffer)
      
      # TODO REMOVE THIS LIMIT AFTER IMPLEMTING UART
      if file.tell() >= 1024 * 1024 or 1 == 1:
        break

def print_hex(data):
  for i in range(0, len(data), 16):
    line = data[i:i+16]
    hex_line = ' '.join(f'{byte:02X}' for byte in line)
    print(hex_line)
    
read_cart()