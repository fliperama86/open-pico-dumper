#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/spi.h"
#include <fstream>
#include <vector>

#define ROM_BASE_ADDRESS 0x10000000
#define CART_SIZE 12
#define FILE_PATH "dump.n64"
#define LED_PIN 25

std::vector<uint> pico_pins_map = {28, 27, 26, 22, 21, 20, 19, 18, 2, 3, 4, 5, 6, 7, 8, 9};
std::vector<uint> address_pins;

uint write_pin = 10;
uint read_pin = 11;
uint ale_low_pin = 17;
uint ale_high_pin = 18;

void set_address_pins_out()
{
  for (auto pin : address_pins)
  {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_OUT);
    gpio_put(pin, 0);
  }
}

void set_address_pins_in()
{
  for (auto pin : address_pins)
  {
    gpio_init(pin);
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_down(pin);
  }
}

void set_address(uint address)
{
  set_address_pins_out();

  uint address_low_out = address & 0xFFFF;
  uint address_high_out = address >> 16;

  gpio_put(write_pin, 1);
  gpio_put(read_pin, 1);
  gpio_put(ale_low_pin, 1);

  sleep_us(1);
  gpio_put(ale_high_pin, 1);

  for (int i = 0; i < 16; i++)
  {
    uint bit = (address_high_out >> i) & 1;
    gpio_put(address_pins[15 - i], bit);
  }

  sleep_us(1);

  gpio_put(ale_high_pin, 0);

  for (int i = 0; i < 16; i++)
  {
    uint bit = (address_low_out >> i) & 1;
    gpio_put(address_pins[15 - i], bit);
  }

  sleep_us(1);
  gpio_put(ale_low_pin, 0);

  sleep_us(1);

  set_address_pins_in();
}

uint read_word()
{
  gpio_put(read_pin, 0);
  sleep_us(1);

  uint word = 0;

  for (int i = 0; i < address_pins.size(); i++)
  {
    word = (word << i) | gpio_get(address_pins[i]);
  }

  gpio_put(ale_high_pin, 1);

  sleep_us(1);

  return word;
}

void read_cart()
{
  std::ofstream file(FILE_PATH, std::ios::binary);
  std::vector<uint8_t> write_buffer(512);

  // Read the data in 512 byte chunks
  for (uint rom_address = ROM_BASE_ADDRESS; rom_address < ROM_BASE_ADDRESS + (CART_SIZE * 1024 * 1024); rom_address += 512)
  {
    // Blink led
    if ((rom_address & 0x3FFF) == 0)
    {
      gpio_put(LED_PIN, 1);
    }
    else
    {
      gpio_put(LED_PIN, 0);
    }

    // Set the address for the next 512 bytes
    set_address(rom_address);

    for (int bufferIndex = 0; bufferIndex < 512; bufferIndex += 2)
    {
      uint word = read_word();
      write_buffer[bufferIndex] = word >> 8;
      write_buffer[bufferIndex + 1] = word & 0xFF;
    }

    file.write(reinterpret_cast<char *>(write_buffer.data()), write_buffer.size());

    // TODO REMOVE THIS LIMIT AFTER IMPLEMTING UART
    if (file.tellp() >= 1024 * 1024)
    {
      break;
    }
  }
}

int main()
{
  stdio_init_all();
  for (auto pin : pico_pins_map)
  {
    address_pins.push_back(pin);
  }
  read_cart();
  return 0;
}