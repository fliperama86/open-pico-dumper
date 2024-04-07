from machine import Pin, SPI, UART
import utime

# IO used in the cart dumper
PIN_CART_DETECT = Pin(0, Pin.IN)
PIN_WRITE_ENABLE_LOW = Pin(2, Pin.OUT)
PIN_WRITE_ENABLE_HIGH = Pin(3, Pin.OUT)
PIN_CART_OUTPUT_ENABLE = Pin(4, Pin.OUT)
PIN_CART_CHIP_ENABLE = Pin(5, Pin.OUT)

# SHIFT REGISTERS PIN
PIN_SHIFT_REGISTERS_OE = Pin(6, Pin.OUT)
PIN_SHIFT_REGISTERS_RCK = Pin(7, Pin.OUT)
PIN_SHIFT_REGISTERS_SCK = Pin(8, Pin.OUT)
PIN_SHIFT_REGISTERS_DATA = Pin(9, Pin.OUT)

# 18.432 Mhz: 1nop = 54 ns
def NOP():
    utime.sleep_us(1)

# global const
ROM_HEADER_START = 0x80
ROM_HEADER_END = 0x100
ROM_MAX = 0x0FFFFF

# address and data bus
address = bytearray(4)
command = 0x00
data = bytearray(2)
size = bytearray(4)
version = "GENDUMPER v1.0"

# The 16 bit databus is made of two 8 bit bus on the RP2040: Port A and Port C. This sets the whole port as inputs
def set_data_bus_input():
    for i in range(16):
        Pin(i, Pin.IN)

# The 16 bit databus is made of two 8 bit bus on the RP2040: Port A and Port C. This sets the whole port as outputs
def set_data_bus_output():
    for i in range(16):
        Pin(i, Pin.OUT)

# roughly a 200ns delay
def wait200ns():
    NOP()
    NOP()
    NOP()
    NOP()

# SPI and UART setup
spi = SPI(0)
uart = UART(1, 460800)

while True:
    if uart.any() > 0:
        command = uart.read(1)

        if command == b'v':  # challenge for auto detection
            size = len(version).to_bytes(4, 'little')
            uart.write(size)  # number of bytes to be sent
            uart.write(version)

        elif command == b'i':  # cart info
            size = (256).to_bytes(4, 'little')
            data = (0).to_bytes(2, 'little')

            PIN_SHIFT_REGISTERS_OE.value(0)

            uart.write(size)  # number of bytes to be sent

            for i in range(ROM_HEADER_START, ROM_HEADER_END):
                PIN_SHIFT_REGISTERS_RCK.value(0)
                spi.write(address[2:0:-1])
                PIN_SHIFT_REGISTERS_RCK.value(1)

                # 200 ns wait
                wait200ns()

                # read data
                data[0] = Pin(0).value()
                data[1] = Pin(1).value()

                uart.write(data[::-1])

            PIN_SHIFT_REGISTERS_OE.value(1)

        elif command == b'd':  # dump cart
            to = bytearray(4)
            params = bytearray(8)
            address = (0).to_bytes(4, 'little')
            to = (0).to_bytes(4, 'little')
            data = (0).to_bytes(2, 'little')

            # read from address and to address
            uart.readinto(params)
            address = params[0:4]
            to = params[4:8]

            PIN_SHIFT_REGISTERS_OE.value(0)

            size = ((int.from_bytes(to, 'little') - int.from_bytes(address, 'little')) * 2).to_bytes(4, 'little')  # number of bytes sent is twice the to-from address because we send 16 bit for each address
            uart.write(size)  # number of bytes to be sent

            for i in range(int.from_bytes(address, 'little'), int.from_bytes(to, 'little') + 1):
                PIN_SHIFT_REGISTERS_RCK.value(0)
                spi.write(address[2:0:-1])
                PIN_SHIFT_REGISTERS_RCK.value(1)

                # wait ~150 ns for the ROM to show data. 2 NOPs is not reliable and sometimes generates wrong data
                NOP()
                NOP()
                NOP()

                # read data
                data[0] = Pin(0).value()
                data[1] = Pin(1).value()

                # push it over the UART
                uart.write(data[::-1])

            PIN_SHIFT_REGISTERS_OE.value(1)

        elif command == b'S':  # dump save game if any
            pass