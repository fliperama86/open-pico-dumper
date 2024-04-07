void readRom_N64()
{
  // Get name, add extension and convert to char array for sd lib
  strcpy(fileName, romName);
  strcat(fileName, ".Z64");

  // create a new folder
  EEPROM_readAnything(0, foldern);
  sprintf(folder, "N64/ROM/%s/%d", romName, foldern);
  sd.mkdir(folder, true);
  sd.chdir(folder);

  // clear the screen
  // display_Clear();
  print_STR(saving_to_STR, 0);
  print_Msg(folder);
  println_Msg(F("/..."));
  display_Update();

  // write new folder number back to eeprom
  foldern = foldern + 1;
  EEPROM_writeAnything(0, foldern);

  // Open file on sd card
  if (!myFile.open(fileName, O_RDWR | O_CREAT))
  {
    print_FatalError(create_file_STR);
  }

  // Initialize progress bar
  uint32_t processedProgressBar = 0;
  uint32_t totalProgressBar = (uint32_t)(cartSize) * 1024 * 1024;
  draw_progressbar(0, totalProgressBar);

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

    processedProgressBar += 512;
    draw_progressbar(processedProgressBar, totalProgressBar);
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
  // Set address pins to output
  adOut_N64();

  // Split address into two words
  word myAdrLowOut = myAddress & 0xFFFF;
  word myAdrHighOut = myAddress >> 16;

  // Switch WR(PH5) RD(PH6) ale_L(PC0) ale_H(PC1) to high (since the pins are active low)
  PORTH |= (1 << 5) | (1 << 6);
  PORTC |= (1 << 1);
  __asm__("nop\n\t");
  PORTC |= (1 << 0);

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
  // Pull read(PH6) low
  PORTH &= ~(1 << 6);

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