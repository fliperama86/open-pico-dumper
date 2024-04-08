from machine import Pin
from utime import sleep_us
import os

cart_size = 12
rom_base_address = 0x10000000
file_path = "dump.n64"
led_pin = Pin(25, Pin.OUT)

pico_pins_map = [28, 27, 26, 22, 21, 20, 19, 18, 9, 8, 7, 6, 5, 4, 3, 2]
address_pins = [Pin(i, Pin.OUT) for i in pico_pins_map]

write_pin = Pin(10, Pin.OUT)
read_pin = Pin(11, Pin.OUT)

ale_low_pin = Pin(17, Pin.OUT)
ale_high_pin = Pin(16, Pin.OUT)

reset_pin = Pin(15, Pin.OUT)

def setup_cart():
  # Set Address Pins to Output and set them low
  set_pico_address_pins_out()
  
  # Set Control Pins to Output RESET(PH0) WR(PH5) RD(PH6) aleL(PC0) aleH(PC1)
  for pin in [reset_pin, write_pin, read_pin, ale_low_pin, ale_high_pin]:
    pin.init(Pin.OUT)

  # Pull RESET(PH0) low until we are ready
  reset_pin.low()
  
  # Output a high signal on WR(PH5) RD(PH6), pins are active low therefore
  # everything is disabled now
  write_pin.high()
  read_pin.high()
  
  # Pull aleL(PC0) low and aleH(PC1) high
  ale_low_pin.low()
  ale_high_pin.high()
  
  # Wait until all is stable
  sleep_us(1)
  
  # Pull RESET(PH0) high to start eeprom
  reset_pin.high()

def set_pico_address_pins_out():
  for pin in address_pins:
    pin.init(Pin.OUT)
    pin.low()

def set_pico_address_pins_in():
  for pin in address_pins:
    pin.init(Pin.IN)

def set_address(address):
  # Set address pins to output
  set_pico_address_pins_out()

  # Split address into two words
  address_low = address & 0xFFFF
  address_high = address >> 16

  # Switch WR(PH5) RD(PH6) ale_L(PC0) ale_H(PC1) to high (since the pins are active low)
  write_pin.high()
  read_pin.high()
  ale_low_pin.high()
  ale_high_pin.high()
  
  write_word(address_high)

  ale_high_pin.low()

  write_word(address_low)
  
  ale_low_pin.low()
  
  set_pico_address_pins_in()

def read_word_from_address_pins():
  word = 0
  for i in range(16):
    bit = address_pins[i].value()
    word |= bit << i
  return word

def write_word(word):
  for i in range(16):
    bit = (word >> i) & 1
    address_pins[i].value(bit)

def read_word():
  read_pin.low()
  word = read_word_from_address_pins()
  read_pin.high()
  return word

def print_hex(data):
  for i in range(0, len(data), 16):
    line = data[i:i+16]
    hex_line = ' '.join(f'{byte:02X}' for byte in line)
    print(hex_line)

def get_cart_id():
  set_address(rom_base_address)
  buffer = bytearray(64)
  for i in range(0, 64, 2):
    word = read_word()
    low_byte = word & 0xFF
    high_byte = word >> 8
    buffer[i] = high_byte
    buffer[i + 1] = low_byte
  return buffer

max_file_size = 1024 * 1024
def read_cart():
  buffer_size = 100 * 1024
  os.remove(file_path)
  with open(file_path, 'wb') as file:
    write_buffer = bytearray(buffer_size)
    offset = 0
    progress = 0

    # Read the data in 512 byte chunks
    for rom_address in range(rom_base_address, rom_base_address + (cart_size * 1024 * 1024), 512):
      # Set the address for the next 512 bytes
      set_address(rom_address)

      for bufferIndex in range(0, 512, 2):
        word = read_word()
        write_buffer[bufferIndex + offset] = word >> 8
        write_buffer[bufferIndex + offset + 1] = word & 0xFF
      
      offset += 512
      
      if (offset >= buffer_size):
        file.write(write_buffer)
        offset = 0
      
      # Report progress
      if (rom_address & 0x3FFF) == 0:
        led_pin.high()
        print(f'Progress: {progress:.0f}%', end='\r')
        progress += (0x3FFF / max_file_size) * 100
      else:
        led_pin.low()
    
      if (file.tell() >= max_file_size):
        print('')
        print("Done!                                    ")
        break

def main():
  setup_cart()
  cart_id = get_cart_id()
  cart_name = cart_id[32:42].decode('utf-8')
  print('Cart Name:', cart_name)
  print('Dumping cart...')
  setup_cart()
  read_cart()
  
main()

