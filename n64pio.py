#type: ignore
from machine import Pin
import rp2
from rp2 import StateMachine, PIO
from utime import sleep_us, time_ns

# Constants
NUM_BYTES = 512  # Number of bytes to read
CONTROL_PIN = 16  # GPIO pin number for the control pin
SM_FREQ = 125_000_000  # State machine frequency

@rp2.asm_pio(out_shiftdir=PIO.SHIFT_LEFT, out_init=PIO.OUT_LOW, set_init=PIO.OUT_LOW)
def set_address_pio():
  # First part: output the high 16 bits (via thresh config on SM)
  pull(block)           # Pull the next 32-bit word from FIFO
  mov(x, osr)           # Move data from OSR to scratch X register
  
  # Toggle the control pin to indicate high bits are set
  set(pindirs, 0b1111)  # Make control pin an output
  set(pins, 0b1111)     # Set control pin high
  nop() [31]

  # Second part: output the low 16 bits (via thresh config on SM)
  mov(osr, x)           # Move the low 16 bits into the OSR

# PIO program to read data from the pins
@rp2.asm_pio(set_init=PIO.OUT_HIGH)
def read_word_pio():
  # Set the control pin low to request data
  set(pins, 0)
  # Delay for setup time, adjust delay as needed (number represents cycles)
  nop() [9]
  # Read 16 bits of data
  in_(pins, 16)
  # Set the control pin high to advance to the next data set
  set(pins, 1)
  # Push the read data to RX FIFO
  push(noblock)

# Configure PIO and state machine
read_state_machine = StateMachine(0, read_word_pio, freq=SM_FREQ, in_base=Pin(0), set_base=Pin(CONTROL_PIN), push_thresh=16)
address_state_machine = StateMachine(1, set_address_pio, freq=2000000, out_base=Pin(0), set_base=Pin(CONTROL_PIN))


# Function to read 512 bytes of data
def read_512():
  read_state_machine.active(1)
  buffer = bytearray(NUM_BYTES)
  for i in range(512 // 2):  # Each read operation fetches 2 bytes (16 bits)
    # Wait until there is data in the FIFO
    while read_state_machine.rx_fifo() == 0:
      sleep_us(1)
      print("Waiting for data...")
    # Retrieve data from FIFO
    data = read_state_machine.get()
    # Store the high and low bytes in the buffer
    buffer[2*i] = data & 0xFF
    buffer[2*i + 1] = (data >> 8) & 0xFF
  read_state_machine.active(0)
  return buffer

def print_hex(data):
  for i in range(0, len(data), 16):
    line = data[i:i+16]
    hex_line = ' '.join(f'{byte:02X}' for byte in line)
    print(hex_line)

led_pin = Pin(25, Pin.OUT)

def read_cart():
  base_address = 0x10000000
  cart_size = 12 * 1024 * 1024 # 12 MB
  cart_size = 512
  for i in range(base_address, base_address + cart_size, 512):
    address_state_machine.active(1)
    address_state_machine.put(base_address + i)
    address_state_machine.active(0)
    
    if(i & 0x3FFF) == 0:
      led_pin.high()
    else:
      led_pin.low()
    
    print_hex(read_512())

def main():
  start_time = time_ns()
  read_cart()
  end_time = time_ns()

  ms_time = (end_time - start_time) / 1000000
  s_time = ms_time / 1000
  print(f"Runtime {s_time:.2f}s ({ms_time:.0f}ms)")

main()