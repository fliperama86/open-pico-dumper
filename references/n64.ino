#include <cstring> // Include the header file for the strcpy function

void readRom_N64()
{
  strcpy(fileName, romName);

  for (unsigned long currByte = romBase; currByte < (romBase + (cartSize * 1024 * 1024)); currByte += 512)
  {
    // Blink led
    if ((currByte & 0x3FFF) == 0)
      blinkLED();

    // Set the address for the next 512 bytes
    setAddress_N64(currByte);

    for (word c = 0; c < 512; c += 2)
    {
      word myWord = readWord_N64();
      sdBuffer[c] = myWord >> 8;
      sdBuffer[c + 1] = myWord & 0xFF;
    }
    myFile.write(sdBuffer, 512);
  }
  // Close the file:
  myFile.close();
}
void adOut_N64()
{
  // A0-A7
  DDRF = 0xFF;
  PORTF = 0x00;
  // A8-A15
  DDRK = 0xFF;
  PORTK = 0x00;
}

// Switch Cartridge address/data pins to read
void adIn_N64()
{
  // A0-A7
  DDRF = 0x00;
  // A8-A15
  DDRK = 0x00;
  // Enable internal pull-up resistors
  // PORTF = 0xFF;
  // PORTK = 0xFF;
}
void setAddress_N64(unsigned long myAddress)
{
  // // Set address pins to output
  // adOut_N64();

  // // Split address into two words
  // word myAdrLowOut = myAddress & 0xFFFF;
  // word myAdrHighOut = myAddress >> 16;

  // // Switch WR(PH5) RD(PH6) ale_L(PC0) ale_H(PC1) to high (since the pins are active low)
  // PORTH |= (1 << 5) | (1 << 6);
  // PORTC |= (1 << 1);
  // __asm__("nop\n\t");
  // PORTC |= (1 << 0);

  // Output high part to address pins
  PORTF = myAdrHighOut & 0xFF;
  PORTK = (myAdrHighOut >> 8) & 0xFF;

  // Leave ale_H high for additional 62.5ns
  __asm__("nop\n\t");

  // Pull ale_H(PC1) low
  PORTC &= ~(1 << 1);

  // Output low part to address pins
  PORTF = myAdrLowOut & 0xFF;
  PORTK = (myAdrLowOut >> 8) & 0xFF;

  // Leave ale_L high for ~125ns
  __asm__("nop\n\t"
          "nop\n\t");

  // Pull ale_L(PC0) low
  PORTC &= ~(1 << 0);

  // Wait ~600ns just to be sure address is set
  __asm__("nop\n\t"
          "nop\n\t"
          "nop\n\t"
          "nop\n\t"
          "nop\n\t"
          "nop\n\t"
          "nop\n\t"
          "nop\n\t"
          "nop\n\t"
          "nop\n\t");

  // Set data pins to input
  adIn_N64();
}
word readWord_N64()
{
  // // Pull read(PH6) low
  // PORTH &= ~(1 << 6);

  // Wait ~310ns
  __asm__("nop\n\t"
          "nop\n\t"
          "nop\n\t"
          "nop\n\t"
          "nop\n\t");

  // Join bytes from PINF and PINK into a word
  word tempWord = ((PINK & 0xFF) << 8) | (PINF & 0xFF);

  // Pull read(PH6) high
  PORTH |= (1 << 6);

  // Wait 62.5ns
  __asm__("nop\n\t");
  return tempWord;
}

void setup_N64_Cart()
{
  // // Set Address Pins to Output and set them low
  // // A0-A7
  // DDRF = 0xFF;
  // PORTF = 0x00;
  // // A8-A15
  // DDRK = 0xFF;
  // PORTK = 0x00;

  // // Set Control Pins to Output RESET(PH0) WR(PH5) RD(PH6) aleL(PC0) aleH(PC1)
  // DDRH |= (1 << 0) | (1 << 5) | (1 << 6);
  // // aleL(PC0) aleH(PC1)
  // DDRC |= (1 << 0) | (1 << 1);

  // // Pull RESET(PH0) low until we are ready
  // PORTH &= ~(1 << 0);

  // // Output a high signal on WR(PH5) RD(PH6), pins are active low therefore everything is disabled now
  // PORTH |= (1 << 5) | (1 << 6);

  // Pull aleL(PC0) low and aleH(PC1) high
  PORTC &= ~(1 << 0);
  PORTC |= (1 << 1);

#ifdef ENABLE_CLOCKGEN
  // Adafruit Clock Generator

  initializeClockOffset();

  if (!i2c_found)
  {
    display_Clear();
    print_FatalError(F("Clock Generator not found"));
  }

  // Set Eeprom clock to 2Mhz
  clockgen.set_freq(200000000ULL, SI5351_CLK1);

  // Start outputting Eeprom clock
  clockgen.output_enable(SI5351_CLK1, 1); // Eeprom clock

#else
  // Set Eeprom Clock Pin(PH1) to Output
  DDRH |= (1 << 1);
  // Output a high signal
  PORTH |= (1 << 1);
#endif

  // Set Eeprom Data Pin(PH4) to Input
  DDRH &= ~(1 << 4);
  // Activate Internal Pullup Resistors
  // PORTH |= (1 << 4);

  // Set sram base address
  sramBase = 0x08000000;

#ifdef ENABLE_CLOCKGEN
  // Wait for clock generator
  clockgen.update_status();
#endif

  // Wait until all is stable
  delay(300);

  // Pull RESET(PH0) high to start eeprom
  PORTH |= (1 << 0);
}

// Read rom ID
void idCart()
{
  // Set the address
  setAddress_N64(romBase);
  // Read first 64 bytes of rom
  for (int c = 0; c < 64; c += 2)
  {
    // split word
    word myWord = readWord_N64();
    byte loByte = myWord & 0xFF;
    byte hiByte = myWord >> 8;

    // write to buffer
    sdBuffer[c] = hiByte;
    sdBuffer[c + 1] = loByte;
  }

  // CRC1
  sprintf(checksumStr, "%02X%02X%02X%02X", sdBuffer[0x10], sdBuffer[0x11], sdBuffer[0x12], sdBuffer[0x13]);

  // Get cart id
  cartID[0] = sdBuffer[0x3B];
  cartID[1] = sdBuffer[0x3C];
  cartID[2] = sdBuffer[0x3D];
  cartID[3] = sdBuffer[0x3E];

  // Get rom version
  romVersion = sdBuffer[0x3F];

  // If name consists out of all japanese characters use cart id
  if (buildRomName(romName, &sdBuffer[0x20], 20) == 0)
  {
    strcpy(romName, cartID);
  }

#ifdef OPTION_N64_SAVESUMMARY
  // Get CRC1
  for (int i = 0; i < 4; i++)
  {
    if (sdBuffer[0x10 + i] < 0x10)
    {
      CRC1 += '0';
    }
    CRC1 += String(sdBuffer[0x10 + i], HEX);
  }

  // Get CRC2
  for (int i = 0; i < 4; i++)
  {
    if (sdBuffer[0x14 + i] < 0x10)
    {
      CRC2 += '0';
    }
    CRC2 += String(sdBuffer[0x14 + i], HEX);
  }
#endif
}