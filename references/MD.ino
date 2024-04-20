void setup_MD()
{
  // Request 5V
  setVoltage(VOLTS_SET_5V);

#if defined(ENABLE_CONFIG)
  segaSram16bit = configGetLong(F("md.saveType"));
#elif defined(use_md_conf)
  mdLoadConf();
#endif /*ENABLE_CONFIG*/

  // Set Address Pins to Output
  // A0-A7
  DDRF = 0xFF;
  // A8-A15
  DDRK = 0xFF;
  // A16-A23
  DDRL = 0xFF;

  // Set Control Pins to Output RST(PH0) CLK(PH1) CS(PH3) WRH(PH4) WRL(PH5) OE(PH6)
  DDRH |= (1 << 0) | (1 << 1) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6);

  // Set TIME(PJ0), AS(PJ1) to Output
  DDRJ |= (1 << 0) | (1 << 1);

  // set ASEL(PG5) to Output
  DDRG |= (1 << 5);

  // Set Data Pins (D0-D15) to Input
  DDRC = 0x00;
  DDRA = 0x00;

  // Setting RST(PH0) CS(PH3) WRH(PH4) WRL(PH5) OE(PH6) HIGH
  PORTH |= (1 << 0) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6);

  // Setting TIME(PJ0) AS(PJ1) HIGH
  PORTJ |= (1 << 0) | (1 << 1);

  // setting ASEL(PG5) HIGH
  PORTG |= (1 << 5);

  delay(200);

  // Print all the info
  getCartInfo_MD();
}

word readWord_MD(unsigned long myAddress)
{
  PORTF = myAddress & 0xFF;
  PORTK = (myAddress >> 8) & 0xFF;
  PORTL = (myAddress >> 16) & 0xFF;

  // Arduino running at 16Mhz -> one nop = 62.5ns
  NOP;

  // Setting CS(PH3) LOW
  PORTH &= ~(1 << 3);
  // Setting OE(PH6) LOW
  PORTH &= ~(1 << 6);
  // Setting AS(PJ1) LOW
  PORTJ &= ~(1 << 1);
  // Setting ASEL(PG5) LOW
  PORTG &= ~(1 << 5);
  // Pulse CLK(PH1), needed for SVP enhanced carts
  pulse_clock(10);

  // most MD ROMs are 200ns, comparable to SNES > use similar access delay of 6 x 62.5 = 375ns
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;

  // Read
  word tempWord = ((PINA & 0xFF) << 8) | (PINC & 0xFF);

  // Setting CS(PH3) HIGH
  PORTH |= (1 << 3);
  // Setting OE(PH6) HIGH
  PORTH |= (1 << 6);
  // Setting AS(PJ1) HIGH
  PORTJ |= (1 << 1);
  // Setting ASEL(PG5) HIGH
  PORTG |= (1 << 5);
  // Pulse CLK(PH1), needed for SVP enhanced carts
  pulse_clock(10);

  // these 6x nop delays have been here before, unknown what they mean
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;

  return tempWord;
}
// Switch data pins to write
void dataOut_MD()
{
  DDRC = 0xFF;
  DDRA = 0xFF;
}

// Switch data pins to read
void dataIn_MD()
{
  DDRC = 0x00;
  DDRA = 0x00;
}

void getCartInfo_MD()
{
  // Set control
  dataIn_MD();

  // Get cart size
  cartSize = ((long(readWord_MD(0xD2)) << 16) | readWord_MD(0xD3)) + 1;

  // Check for 32x compatibility
  if ((readWord_MD(0x104 / 2) == 0x2033) && (readWord_MD(0x106 / 2) == 0x3258))
    is32x = 1;
  else
    is32x = 0;

  // Get cart checksum
  chksum = readWord_MD(0xC7);

  // Get cart ID
  char id[15];
  memset(id, 0, 15);
  for (byte c = 0; c < 14; c += 2)
  {
    // split word
    word myWord = readWord_MD((0x180 + c) / 2);
    byte loByte = myWord & 0xFF;
    byte hiByte = myWord >> 8;

    // write to buffer
    id[c] = hiByte;
    id[c + 1] = loByte;
  }

  // Identify games using SVP chip
  if (!strncmp("GM MK-1229 ", id, 11) || !strncmp("GM G-7001  ", id, 11)) // Virtua Racing (E/U/J)
    isSVP = 1;
  else
    isSVP = 0;

  // Fix cartridge sizes according to no-intro database
  if (cartSize == 0x400000)
  {
    switch (chksum)
    {
    case 0xCE25: // Super Street Fighter 2 (J) 40Mbit
    case 0xE41D: // Super Street Fighter 2 (E) 40Mbit
    case 0xE017: // Super Street Fighter 2 (U) 40Mbit
      cartSize = 0x500000;
      break;
    case 0x0000: // Demons of Asteborg v1.0 (W) 120Mbit
      cartSize = 0xEAF2F4;
      break;
    case 0xBCBF: // Demons of Asteborg v1.1 (W) 120Mbit
    case 0x6E1E: // Demons of Asteborg v1.11 (W) 120Mbit
      cartSize = 0xEA0000;
      break;
    }
  }
  if (cartSize == 0x300000)
  {
    switch (chksum)
    {
    case 0xBC5F: // Batman Forever (World)
    case 0x3CDD: // Donald in Maui Mallard (Brazil) (En)
    case 0x44AD: // Donald in Maui Mallard (Europe) (Rev A)
    case 0x2D9A: // Foreman for Real (World)
    case 0x5648: // Justice League Task Force (World)
    case 0x0A29: // Mega 6 Vol. 3 (Europe)
    case 0x7651: // NFL Quarterback Club (World)
    case 0x74CA: // WWF RAW (World)
      cartSize = 0x400000;
      break;
    }
  }
  if (cartSize == 0x200000)
  {
    switch (chksum)
    {
    case 0x2078: // Dynamite Headdy (USA, Europe)
      chksum = 0x9877;
      break;
    case 0xAE95: // Winter Olympic Games (USA)
      chksum = 0x56A0;
      break;
    }
  }
  if (cartSize == 0x180000)
  {
    switch (chksum)
    {
    case 0xFFE2: // Cannon Fodder (Europe)
    case 0xF418: // Chaos Engine, The (Europe)
    case 0xF71D: // Fatal Fury (Europe, Korea) (En)
    case 0xA884: // Flashback (Europe) (En,Fr)
    case 0x7D68: // Flashback - The Quest for Identity (USA) (En,Fr)
    case 0x030D: // Shining Force (Europe)
    case 0xE975: // Shining Force (USA)
      cartSize = 0x200000;
      break;
    }
  }
  if (cartSize == 0x100000)
  {
    switch (chksum)
    {
    case 0xCDF5: // Life on Mars (Aftermarket)
      cartSize = 0x400000;
      chksum = 0x603A;
      break;
    case 0xF85F: // Metal Dragon (Aftermarket)
      cartSize = 0x200000;
      chksum = 0x6965;
      break;
    }
  }
  if (cartSize == 0xC0000)
  {
    switch (chksum)
    {
    case 0x9D79: // Wonder Boy in Monster World (USA, Europe)
      cartSize = 0x100000;
      break;
    }
  }
  if (cartSize == 0x80000)
  {
    switch (chksum)
    {
    case 0x06C1: // Madden NFL 98 (USA)
      cartSize = 0x200000;
      chksum = 0x8473;
      break;
    case 0x5B3A: // NHL 98 (USA)
      cartSize = 0x200000;
      chksum = 0x5613;
      break;
    case 0xD07D: // Zero Wing (Japan)
      cartSize = 0x100000;
      chksum = 0xF204;
      break;
    case 0x95C9: // Zero Wing (Europe)
    case 0x9144: // Zoop (Europe)
    case 0xB8D4: // Zoop (USA)
      cartSize = 0x100000;
      break;
    case 0xC422: // Jeopardy! (USA)
      chksum = 0xC751;
      break;
    case 0x0C6A: // Monopoly (USA)
      chksum = 0xE1AA;
      break;
    case 0xA760: // Gain Ground (USA)
      chksum = 0x97CD;
      break;
    case 0x1404: // Wonder Boy III - Monster Lair (Japan, Europe) (En)
      chksum = 0x53B9;
      break;
    }
  }
  if (cartSize == 0x40000)
  {
    switch (chksum)
    {
    case 0x8BC6: // Pac-Attack (USA)
    case 0xB344: // Pac-Panic (Europe)
      cartSize = 0x100000;
      break;
    }
  }
  if (cartSize == 0x20000)
  {
    switch (chksum)
    {
    case 0x7E50: // Micro Machines 2 - Turbo Tournament (Europe)
      cartSize = 0x100000;
      chksum = 0xD074;
      break;
    case 0x168B: // Micro Machines - Military (Europe)
      cartSize = 0x100000;
      chksum = 0xCEE0;
      break;
    }
  }

  // Fatman (Japan).md
  if (!strncmp("GM T-44013 ", id, 11) && (chksum == 0xFFFF))
  {
    chksum = 0xC560;
    cartSize = 0xA0000;
  }

  // Beggar Prince (Rev 1)(Aftermarket)
  if (!strncmp("SF-001", id, 6) && (chksum == 0x3E08))
  {
    cartSize = 0x400000;
  }

  // Legend of Wukong (Aftermarket)
  if (!strncmp("SF-002", id, 6) && (chksum == 0x12B0))
  {
    chksum = 0x45C6;
  }

  // YM2612 Instrument Editor (Aftermarket)
  if (!strncmp("GM 10101010", id, 11) && (chksum == 0xC439))
  {
    chksum = 0x21B0;
    cartSize = 0x100000;
  }

  // Technoptimistic (Aftermarket)
  if (!strncmp("MU REMUTE01", id, 11) && (chksum == 0x0000))
  {
    chksum = 0xB55C;
    cartSize = 0x400000;
  }

  // Decoder (Aftermarket)
  if (!strncmp("GM REMUTE02", id, 11) && (chksum == 0x0000))
  {
    chksum = 0x5426;
    cartSize = 0x400000;
  }

  // Handy Harvy (Aftermarket)
  if (!strncmp("GM HHARVYSG", id, 11) && (chksum == 0x0000))
  {
    chksum = 0xD9D2;
    cartSize = 0x100000;
  }

  // Jim Power - The Lost Dimension in 3D (Aftermarket)
  if (!strncmp("GM T-107036", id, 11) && (chksum == 0x0000))
  {
    chksum = 0xAA28;
  }

  // mikeyeldey95 (Aftermarket)
  if (!strncmp("GM 00000000-43", id, 14) && (chksum == 0x0000))
  {
    chksum = 0x921B;
    cartSize = 0x400000;
  }

  // Enryuu Seiken Xiao-Mei (Aftermarket)
  if (!strncmp("GM 00000000-00", id, 14) && (chksum == 0x1E0C))
  {
    chksum = 0xE7E5;
    cartSize = 0x400000;
  }

  // Life on Earth - Reimagined (Aftermarket)
  if (!strncmp("GM 00000000-00", id, 14) && (chksum == 0x6BD5))
  {
    chksum = 0x1FEA;
    cartSize = 0x400000;
  }

  // Sasha Darko's Sacred Line I (Aftermarket)
  if (!strncmp("GM 00000005-00", id, 14) && (chksum == 0x9F34))
  {
    chksum = 0xA094;
    cartSize = 0x400000;
  }

  // Sasha Darko's Sacred Line II (Aftermarket)
  if (!strncmp("GM 00000005-00", id, 14) && (chksum == 0x0E9B))
  {
    chksum = 0x6B4B;
    cartSize = 0x400000;
  }

  // Sasha Darko's Sacred Line (Watermelon Release) (Aftermarket)
  if (!strncmp("GM T-574323-00", id, 14) && (chksum == 0xAEDD))
  {
    cartSize = 0x400000;
  }

  // Kromasphere (Aftermarket)
  if (!strncmp("GM MK-0000 -00", id, 14) && (chksum == 0xC536))
  {
    chksum = 0xFAB1;
    cartSize = 0x200000;
  }

  // YM2017 (Aftermarket)
  if (!strncmp("GM CSET0001-02", id, 14) && (chksum == 0x0000))
  {
    chksum = 0xE3A9;
  }

  // The Curse of Illmore Bay (Aftermarket)
  if (!strncmp("1774          ", id, 14) && (chksum == 0x0000))
  {
    chksum = 0x6E34;
    cartSize = 0x400000;
  }

  // Coffee Crisis (Aftermarket)
  if (!strncmp("JN-20160131-03", id, 14) && (chksum == 0x0000))
  {
    chksum = 0x8040;
    cartSize = 0x400000;
  }

  // Sonic & Knuckles Check
  SnKmode = 0;
  if (chksum == 0xDFB3)
  {

    // Sonic & Knuckles ID:GM MK-1563 -00
    if (!strcmp("GM MK-1563 -00", id))
    {
      char labelLockon[17];
      memset(labelLockon, 0, 17);

      // Get labelLockon
      for (byte c = 0; c < 16; c += 2)
      {
        // split word
        word myWord = readWord_MD((0x200100 + c) / 2);
        byte loByte = myWord & 0xFF;
        byte hiByte = myWord >> 8;

        // write to buffer
        labelLockon[c] = hiByte;
        labelLockon[c + 1] = loByte;
      }

      // check Lock-on game presence
      if (!(strcmp("SEGA MEGA DRIVE ", labelLockon) & strcmp("SEGA GENESIS    ", labelLockon)))
      {
        char idLockon[15];
        memset(idLockon, 0, 15);

        // Lock-on cart checksum
        chksumLockon = readWord_MD(0x1000C7);
        // Lock-on cart size
        cartSizeLockon = ((long(readWord_MD(0x1000D2)) << 16) | readWord_MD(0x1000D3)) + 1;

        // Get IdLockon
        for (byte c = 0; c < 14; c += 2)
        {
          // split word
          word myWord = readWord_MD((0x200180 + c) / 2);
          byte loByte = myWord & 0xFF;
          byte hiByte = myWord >> 8;

          // write to buffer
          idLockon[c] = hiByte;
          idLockon[c + 1] = loByte;
        }

        if (!strncmp("GM 00001009-0", idLockon, 13) || !strncmp("GM 00004049-0", idLockon, 13))
        {
          // Sonic1 ID:GM 00001009-0? or GM 00004049-0?
          SnKmode = 2;
        }
        else if (!strcmp("GM 00001051-00", idLockon) || !strcmp("GM 00001051-01", idLockon) || !strcmp("GM 00001051-02", idLockon))
        {
          // Sonic2 ID:GM 00001051-00 or GM 00001051-01 or GM 00001051-02
          SnKmode = 3;

          // Prepare Sonic2 Banks
          writeSSF2Map(0x509878, 1); // 0xA130F1
        }
        else if (!strcmp("GM MK-1079 -00", idLockon))
        {
          // Sonic3 ID:GM MK-1079 -00
          SnKmode = 4;
        }
        else
        {
          // Other game
          SnKmode = 5;
        }
      }
      else
      {
        SnKmode = 1;
      }
    }
  }

  // Serial EEPROM Check
  for (int i = 0; i < eepcount; i++)
  {
    int index = i * 2;
    word eepcheck = pgm_read_word(eepid + index);
    if (eepcheck == chksum)
    {
      eepdata = pgm_read_word(eepid + index + 1);
      eepType = eepdata & 0x7;
      eepSize = eepdata & 0xFFF8;
      saveType = 4; // SERIAL EEPROM
      break;
    }
  }

  // Greatest Heavyweights of the Ring (J) has blank chksum 0x0000
  // Other non-save carts might have the same blank chksum
  // Check header for Serial EEPROM Data
  if (chksum == 0x0000)
  {
    if (readWord_MD(0xD9) != 0xE840)
    { // NOT SERIAL EEPROM
      eepType = 0;
      eepSize = 0;
      saveType = 0;
    }
  }

  // Codemasters EEPROM Check
  // Codemasters used the same incorrect header across multiple carts
  // Carts with checksum 0x165E or 0x168B could be EEPROM or non-EEPROM
  // Check the first DWORD in ROM (0x444E4C44) to identify the EEPROM carts
  if ((chksum == 0x165E) || (chksum == 0x168B))
  {
    if (readWord_MD(0x00) != 0x444E)
    { // NOT SERIAL EEPROM
      eepType = 0;
      eepSize = 0;
      saveType = 0;
    }
  }

  // CD Backup RAM Cart Check
  // 4 = 128KB (2045 Blocks) Sega CD Backup RAM Cart
  // 6 = 512KB (8189 Blocks) Ultra CD Backup RAM Cart (Aftermarket)
  word bramCheck = readWord_MD(0x00);
  if ((((bramCheck & 0xFF) == 0x04) && ((chksum & 0xFF) == 0x04)) || (((bramCheck & 0xFF) == 0x06) && ((chksum & 0xFF) == 0x06)))
  {
    unsigned long p = 1 << (bramCheck & 0xFF);
    bramSize = p * 0x2000L;
  }
  if (saveType != 4)
  { // NOT SERIAL EEPROM
    // Check if cart has sram
    saveType = 0;
    sramSize = 0;

    // FIXED CODE FOR SRAM/FRAM/PARALLEL EEPROM
    // 0x5241F820 SRAM (ODD BYTES/EVEN BYTES)
    // 0x5241F840 PARALLEL EEPROM - READ AS SRAM
    // 0x5241E020 SRAM (BOTH BYTES)
    if (readWord_MD(0xD8) == 0x5241)
    {
      word sramType = readWord_MD(0xD9);
      if ((sramType == 0xF820) || (sramType == 0xF840))
      { // SRAM/FRAM ODD/EVEN BYTES
        // Get sram start and end
        sramBase = ((long(readWord_MD(0xDA)) << 16) | readWord_MD(0xDB));
        sramEnd = ((long(readWord_MD(0xDC)) << 16) | readWord_MD(0xDD));
        if (sramBase == 0x20000020 && sramEnd == 0x00010020)
        { // Fix for Psy-o-blade
          sramBase = 0x200001;
          sramEnd = 0x203fff;
        }
        // Check alignment of sram
        // 0x300001 for HARDBALL '95 (U)
        // 0x3C0001 for Legend of Wukong (Aftermarket)
        if ((sramBase == 0x200001) || (sramBase == 0x300001) || (sramBase == 0x3C0001))
        {
          // low byte
          saveType = 1; // ODD
          sramSize = (sramEnd - sramBase + 2) / 2;
          // Right shift sram base address so [A21] is set to high 0x200000 = 0b001[0]00000000000000000000
          sramBase = sramBase >> 1;
        }
        else if (sramBase == 0x200000)
        {
          // high byte
          saveType = 2; // EVEN
          sramSize = (sramEnd - sramBase + 1) / 2;
          // Right shift sram base address so [A21] is set to high 0x200000 = 0b001[0]00000000000000000000
          sramBase = sramBase / 2;
        }
        else
        {
          print_Msg(("sramType: "));
          print_Msg_PaddedHex16(sramType);
          println_Msg(FS(FSTRING_EMPTY));
          print_Msg(("sramBase: "));
          print_Msg_PaddedHex32(sramBase);
          println_Msg(FS(FSTRING_EMPTY));
          print_Msg(("sramEnd: "));
          print_Msg_PaddedHex32(sramEnd);
          println_Msg(FS(FSTRING_EMPTY));
          print_FatalError(F("Unknown Sram Base"));
        }
      }
      else if (sramType == 0xE020)
      { // SRAM BOTH BYTES
        // Get sram start and end
        sramBase = ((long(readWord_MD(0xDA)) << 16) | readWord_MD(0xDB));
        sramEnd = ((long(readWord_MD(0xDC)) << 16) | readWord_MD(0xDD));

        if (sramBase == 0x200001)
        {
          saveType = 3; // BOTH
          sramSize = sramEnd - sramBase + 2;
          sramBase = sramBase >> 1;
        }
        else if (sramBase == 0x200000)
        {
          saveType = 3; // BOTH
          sramSize = sramEnd - sramBase + 1;
          sramBase = sramBase >> 1;
        }
        else if (sramBase == 0x3FFC00)
        {
          // Used for some aftermarket carts without sram
          saveType = 0;
        }
        else
        {
          print_Msg(("sramType: "));
          print_Msg_PaddedHex16(sramType);
          println_Msg(FS(FSTRING_EMPTY));
          print_Msg(("sramBase: "));
          print_Msg_PaddedHex32(sramBase);
          println_Msg(FS(FSTRING_EMPTY));
          print_Msg(("sramEnd: "));
          print_Msg_PaddedHex32(sramEnd);
          println_Msg(FS(FSTRING_EMPTY));
          print_FatalError(F("Unknown Sram Base"));
        }
      }
    }
    else
    {
      // SRAM CARTS WITH BAD/MISSING HEADER SAVE DATA
      switch (chksum)
      {
      case 0xC2DB:    // Winter Challenge (UE)
        saveType = 1; // ODD
        sramBase = 0x200001;
        sramEnd = 0x200FFF;
        break;

      case 0xD7B6:    // Buck Rogers: Countdown to Doomsday (UE)
      case 0xFE3E:    // NBA Live '98 (U)
      case 0xFDAD:    // NFL '94 starring Joe Montana (U)
      case 0x632E:    // PGA Tour Golf (UE) (Rev 1)
      case 0xD2BA:    // PGA Tour Golf (UE) (Rev 2)
      case 0x44FE:    // Super Hydlide (J)
        saveType = 1; // ODD
        sramBase = 0x200001;
        sramEnd = 0x203FFF;
        break;

      case 0xDB5E:    // Might & Magic: Gates to Another World (UE) (Rev 1)
      case 0x3428:    // Starflight (UE) (Rev 0)
      case 0x43EE:    // Starflight (UE) (Rev 1)
        saveType = 3; // BOTH
        sramBase = 0x200001;
        sramEnd = 0x207FFF;
        break;

      case 0xBF72:    // College Football USA '96 (U)
      case 0x72EF:    // FIFA Soccer '97 (UE)
      case 0xD723:    // Hardball III (U)
      case 0x06C1:    // Madden NFL '98 (U)
      case 0xDB17:    // NHL '96 (UE)
      case 0x5B3A:    // NHL '98 (U)
      case 0x2CF2:    // NFL Sports Talk Football '93 starring Joe Montana (UE)
      case 0xE9B1:    // Summer Challenge (U)
      case 0xEEE8:    // Test Drive II: The Duel (U)
        saveType = 1; // ODD
        sramBase = 0x200001;
        sramEnd = 0x20FFFF;
        break;
      }
      if (saveType == 1)
      {
        sramSize = (sramEnd - sramBase + 2) / 2;
        sramBase = sramBase >> 1;
      }
      else if (saveType == 3)
      {
        sramSize = sramEnd - sramBase + 2;
        sramBase = sramBase >> 1;
      }
    }
  }

  // Get name
  for (byte c = 0; c < 48; c += 2)
  {
    // split word
    word myWord = readWord_MD((0x150 + c) / 2);
    byte loByte = myWord & 0xFF;
    byte hiByte = myWord >> 8;

    // write to buffer
    sdBuffer[c] = hiByte;
    sdBuffer[c + 1] = loByte;
  }
  romName[copyToRomName_MD(romName, sdBuffer, sizeof(romName) - 1)] = 0;

  // Get Lock-on cart name
  if (SnKmode >= 2)
  {
    char romNameLockon[12];

    // Change romName
    strcpy(romName, "SnK_");

    for (byte c = 0; c < 48; c += 2)
    {
      // split word
      word myWord = readWord_MD((0x200150 + c) / 2);
      byte loByte = myWord & 0xFF;
      byte hiByte = myWord >> 8;

      // write to buffer
      sdBuffer[c] = hiByte;
      sdBuffer[c + 1] = loByte;
    }
    romNameLockon[copyToRomName_MD(romNameLockon, sdBuffer, sizeof(romNameLockon) - 1)] = 0;

    switch (SnKmode)
    {
    case 2:
      strcat(romName, "SONIC1");
      break;
    case 3:
      strcat(romName, "SONIC2");
      break;
    case 4:
      strcat(romName, "SONIC3");
      break;
    case 5:
      strcat(romName, romNameLockon);
      break;
    }
  }

  // Realtec Mapper Check
  word realtecCheck1 = readWord_MD(0x3F080); // 0x7E100 "SEGA" (BootROM starts at 0x7E000)
  word realtecCheck2 = readWord_MD(0x3F081);
  if ((realtecCheck1 == 0x5345) && (realtecCheck2 == 0x4741))
  {
    realtec = 1;
    strcpy(romName, "Realtec");
    cartSize = 0x80000;
  }

  // Some games are missing the ROM size in the header, in this case calculate ROM size by looking for mirror of the first line of the ROM
  // This does not work for cartridges that have SRAM mapped directly after the maskrom like Striker (Europe)
  if ((cartSize < 0x8000) || (cartSize > 0xEAF400))
  {
    for (cartSize = 0x20000 / 2; cartSize < 0x400000 / 2; cartSize += 0x20000 / 2)
    {
      if ((readWord_MD(0x0) == readWord_MD(cartSize)) && (readWord_MD(0x1) == readWord_MD(0x1 + cartSize)) && (readWord_MD(0x2) == readWord_MD(0x2 + cartSize)) && (readWord_MD(0x3) == readWord_MD(0x3 + cartSize)) && (readWord_MD(0x4) == readWord_MD(0x4 + cartSize)) && (readWord_MD(0x5) == readWord_MD(0x5 + cartSize)) && (readWord_MD(0x6) == readWord_MD(0x6 + cartSize)) && (readWord_MD(0x7) == readWord_MD(0x7 + cartSize)) && (readWord_MD(0x8) == readWord_MD(0x8 + cartSize)))
      {
        break;
      }
    }
    cartSize = cartSize * 2;
  }

  display_Clear();
  println_Msg(F("Cart Info"));
  println_Msg(FS(FSTRING_SPACE));
  print_Msg(F("Name: "));
  println_Msg(romName);
  if (bramCheck != 0x00FF)
  {
    print_Msg(F("bramCheck: "));
    print_Msg_PaddedHex16(bramCheck);
    println_Msg(FS(FSTRING_EMPTY));
  }
  if (bramSize > 0)
  {
    print_Msg(F("bramSize(KB): "));
    println_Msg(bramSize >> 10);
  }
  print_Msg(F("Size: "));
  print_Msg(cartSize * 8 / 1024 / 1024);
  switch (SnKmode)
  {
  case 2:
  case 4:
  case 5:
    print_Msg(F("+"));
    print_Msg(cartSizeLockon * 8 / 1024 / 1024);
    break;
  case 3:
    print_Msg(F("+"));
    print_Msg(cartSizeLockon * 8 / 1024 / 1024);
    print_Msg(F("+"));
    print_Msg(cartSizeSonic2 * 8 / 1024 / 1024);
    break;
  }
  println_Msg(F(" MBit"));
  print_Msg(F("ChkS: "));
  print_Msg_PaddedHexByte((chksum >> 8));
  print_Msg_PaddedHexByte((chksum & 0x00ff));
  switch (SnKmode)
  {
  case 2:
  case 4:
  case 5:
    print_Msg(F("+"));
    print_Msg_PaddedHexByte((chksumLockon >> 8));
    print_Msg_PaddedHexByte((chksumLockon & 0x00ff));
    break;
  case 3:
    print_Msg(F("+"));
    print_Msg_PaddedHexByte((chksumLockon >> 8));
    print_Msg_PaddedHexByte((chksumLockon & 0x00ff));
    print_Msg(F("+"));
    print_Msg_PaddedHexByte((chksumSonic2 >> 8));
    print_Msg_PaddedHexByte((chksumSonic2 & 0x00ff));
    break;
  }
  println_Msg(FS(FSTRING_EMPTY));
  if (saveType == 4)
  {
    print_Msg(F("Serial EEPROM: "));
    print_Msg(eepSize * 8 / 1024);
    println_Msg(F(" KBit"));
  }
  else
  {
    print_Msg(F("Sram: "));
    if (sramSize > 0)
    {
      print_Msg(sramSize * 8 / 1024);
      println_Msg(F(" KBit"));
    }
    else
      println_Msg(F("None"));
  }
  println_Msg(FS(FSTRING_SPACE));

  // Wait for user input
#if (defined(ENABLE_LCD) || defined(ENABLE_OLED))
  // Prints string out of the common strings array either with or without newline
  print_STR(press_button_STR, 1);
  display_Update();
  wait();
#endif
}

void readROM_MD()
{
  // Checksum
  uint16_t calcCKS = 0;
  uint16_t calcCKSLockon = 0;
  uint16_t calcCKSSonic2 = 0;

  // Set control
  dataIn_MD();

  // Get name, add extension and convert to char array for sd lib
  strcpy(fileName, romName);
  strcat(fileName, ".BIN");

  // create a new folder
  EEPROM_readAnything(0, foldern);
  sprintf(folder, "MD/ROM/%s/%d", romName, foldern);
  sd.mkdir(folder, true);
  sd.chdir(folder);

  display_Clear();
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
    print_FatalError(sd_error_STR);
  }

  byte buffer[1024] = {0};

  // Phantasy Star/Beyond Oasis with 74HC74 and 74HC139 switch ROM/SRAM at address 0x200000
  if (0x200000 < cartSize && cartSize < 0x400000)
  {
    enableSram_MD(0);
  }

  // Prepare SSF2 Banks
  if (cartSize > 0x400000)
  {
    writeSSF2Map(0x50987E, 6); // 0xA130FD
    writeSSF2Map(0x50987F, 7); // 0xA130FF
  }
  byte offsetSSF2Bank = 0;
  word d = 0;

  // Initialize progress bar
  uint32_t processedProgressBar = 0;
  uint32_t totalProgressBar = (uint32_t)(cartSize);
  if (SnKmode >= 2)
    totalProgressBar += (uint32_t)cartSizeLockon;
  if (SnKmode == 3)
    totalProgressBar += (uint32_t)cartSizeSonic2;
  draw_progressbar(0, totalProgressBar);

  for (unsigned long currBuffer = 0; currBuffer < cartSize / 2; currBuffer += 512)
  {
    // Blink led
    if (currBuffer % 16384 == 0)
      blinkLED();

    if (currBuffer == 0x200000)
    {
      writeSSF2Map(0x50987E, 8); // 0xA130FD
      writeSSF2Map(0x50987F, 9); // 0xA130FF
      offsetSSF2Bank = 1;
    }

    // Demons of Asteborg Additional Banks
    else if (currBuffer == 0x280000)
    {
      writeSSF2Map(0x50987E, 10); // 0xA130FD
      writeSSF2Map(0x50987F, 11); // 0xA130FF
      offsetSSF2Bank = 2;
    }
    else if (currBuffer == 0x300000)
    {
      writeSSF2Map(0x50987E, 12); // 0xA130FD
      writeSSF2Map(0x50987F, 13); // 0xA130FF
      offsetSSF2Bank = 3;
    }
    else if (currBuffer == 0x380000)
    {
      writeSSF2Map(0x50987E, 14); // 0xA130FD
      writeSSF2Map(0x50987F, 15); // 0xA130FF
      offsetSSF2Bank = 4;
    }
    else if (currBuffer == 0x400000)
    {
      writeSSF2Map(0x50987E, 16); // 0xA130FD
      writeSSF2Map(0x50987F, 17); // 0xA130FF
      offsetSSF2Bank = 5;
    }
    else if (currBuffer == 0x480000)
    {
      writeSSF2Map(0x50987E, 18); // 0xA130FD
      writeSSF2Map(0x50987F, 19); // 0xA130FF
      offsetSSF2Bank = 6;
    }
    else if (currBuffer == 0x500000)
    {
      writeSSF2Map(0x50987E, 20); // 0xA130FD
      writeSSF2Map(0x50987F, 21); // 0xA130FF
      offsetSSF2Bank = 7;
    }
    else if (currBuffer == 0x580000)
    {
      writeSSF2Map(0x50987E, 22); // 0xA130FD
      writeSSF2Map(0x50987F, 23); // 0xA130FF
      offsetSSF2Bank = 8;
    }
    else if (currBuffer == 0x600000)
    {
      writeSSF2Map(0x50987E, 24); // 0xA130FD
      writeSSF2Map(0x50987F, 25); // 0xA130FF
      offsetSSF2Bank = 9;
    }
    else if (currBuffer == 0x680000)
    {
      writeSSF2Map(0x50987E, 26); // 0xA130FD
      writeSSF2Map(0x50987F, 27); // 0xA130FF
      offsetSSF2Bank = 10;
    }
    else if (currBuffer == 0x700000)
    {
      writeSSF2Map(0x50987E, 28); // 0xA130FD
      writeSSF2Map(0x50987F, 29); // 0xA130FF
      offsetSSF2Bank = 11;
    }

    d = 0;

    for (int currWord = 0; currWord < 512; currWord++)
    {
      unsigned long myAddress = currBuffer + currWord - (offsetSSF2Bank * 0x80000);
      PORTF = myAddress & 0xFF;
      PORTK = (myAddress >> 8) & 0xFF;
      PORTL = (myAddress >> 16) & 0xFF;

      // Arduino running at 16Mhz -> one nop = 62.5ns
      NOP;
      // Setting CS(PH3) LOW
      PORTH &= ~(1 << 3);
      // Setting OE(PH6) LOW
      PORTH &= ~(1 << 6);
      // Setting AS(PJ1) LOW
      PORTJ &= ~(1 << 1);
      // Setting ASEL(PG5) LOW
      PORTG &= ~(1 << 5);
      // Pulse CLK(PH1)
      if (isSVP)
        pulse_clock(10);

      // most MD ROMs are 200ns, comparable to SNES > use similar access delay of 6 x 62.5 = 375ns
      NOP;
      NOP;
      NOP;
      NOP;
      NOP;
      NOP;

      // Read
      buffer[d] = PINA;
      buffer[d + 1] = PINC;

      // Setting CS(PH3) HIGH
      PORTH |= (1 << 3);
      // Setting OE(PH6) HIGH
      PORTH |= (1 << 6);
      // Setting AS(PJ1) HIGH
      PORTJ |= (1 << 1);
      // Setting ASEL(PG5) HIGH
      PORTG |= (1 << 5);
      // Pulse CLK(PH1)
      if (isSVP)
        pulse_clock(10);

      // Skip first 256 words
      if (((currBuffer == 0) && (currWord >= 256)) || (currBuffer > 0))
      {
        calcCKS += ((buffer[d] << 8) | buffer[d + 1]);
      }
      d += 2;
    }
    myFile.write(buffer, 1024);

    // update progress bar
    processedProgressBar += 1024;
    draw_progressbar(processedProgressBar, totalProgressBar);
  }
  if (SnKmode >= 2)
  {
    for (unsigned long currBuffer = 0; currBuffer < cartSizeLockon / 2; currBuffer += 512)
    {
      // Blink led
      if (currBuffer % 16384 == 0)
        blinkLED();

      d = 0;

      for (int currWord = 0; currWord < 512; currWord++)
      {
        unsigned long myAddress = currBuffer + currWord + cartSize / 2;
        PORTF = myAddress & 0xFF;
        PORTK = (myAddress >> 8) & 0xFF;
        PORTL = (myAddress >> 16) & 0xFF;

        // Arduino running at 16Mhz -> one nop = 62.5ns
        NOP;
        // Setting CS(PH3) LOW
        PORTH &= ~(1 << 3);
        // Setting OE(PH6) LOW
        PORTH &= ~(1 << 6);
        // Setting AS(PJ1) LOW
        PORTJ &= ~(1 << 1);
        // Setting ASEL(PG5) LOW
        PORTG &= ~(1 << 5);
        // Pulse CLK(PH1)
        if (isSVP)
          pulse_clock(10);

        // most MD ROMs are 200ns, comparable to SNES > use similar access delay of 6 x 62.5 = 375ns
        NOP;
        NOP;
        NOP;
        NOP;
        NOP;
        NOP;

        // Read
        buffer[d] = PINA;
        buffer[d + 1] = PINC;

        // Setting CS(PH3) HIGH
        PORTH |= (1 << 3);
        // Setting OE(PH6) HIGH
        PORTH |= (1 << 6);
        // Setting AS(PJ1) HIGH
        PORTJ |= (1 << 1);
        // Setting ASEL(PG5) HIGH
        PORTG |= (1 << 5);
        // Pulse CLK(PH1)
        if (isSVP)
          pulse_clock(10);

        // Skip first 256 words
        if (((currBuffer == 0) && (currWord >= 256)) || (currBuffer > 0))
        {
          calcCKSLockon += ((buffer[d] << 8) | buffer[d + 1]);
        }
        d += 2;
      }
      myFile.write(buffer, 1024);

      // update progress bar
      processedProgressBar += 1024;
      draw_progressbar(processedProgressBar, totalProgressBar);
    }
  }
  if (SnKmode == 3)
  {
    for (unsigned long currBuffer = 0; currBuffer < cartSizeSonic2 / 2; currBuffer += 512)
    {
      // Blink led
      if (currBuffer % 16384 == 0)
        blinkLED();

      d = 0;

      for (int currWord = 0; currWord < 512; currWord++)
      {
        unsigned long myAddress = currBuffer + currWord + (cartSize + cartSizeLockon) / 2;
        PORTF = myAddress & 0xFF;
        PORTK = (myAddress >> 8) & 0xFF;
        PORTL = (myAddress >> 16) & 0xFF;

        // Arduino running at 16Mhz -> one nop = 62.5ns
        NOP;
        // Setting CS(PH3) LOW
        PORTH &= ~(1 << 3);
        // Setting OE(PH6) LOW
        PORTH &= ~(1 << 6);
        // Setting AS(PJ1) LOW
        PORTJ &= ~(1 << 1);
        // Setting ASEL(PG5) LOW
        PORTG &= ~(1 << 5);
        // Pulse CLK(PH1)
        if (isSVP)
          PORTH ^= (1 << 1);

        // most MD ROMs are 200ns, comparable to SNES > use similar access delay of 6 x 62.5 = 375ns
        NOP;
        NOP;
        NOP;
        NOP;
        NOP;
        NOP;

        // Read
        buffer[d] = PINA;
        buffer[d + 1] = PINC;

        // Setting CS(PH3) HIGH
        PORTH |= (1 << 3);
        // Setting OE(PH6) HIGH
        PORTH |= (1 << 6);
        // Setting AS(PJ1) HIGH
        PORTJ |= (1 << 1);
        // Setting ASEL(PG5) HIGH
        PORTG |= (1 << 5);
        // Pulse CLK(PH1)
        if (isSVP)
          PORTH ^= (1 << 1);

        calcCKSSonic2 += ((buffer[d] << 8) | buffer[d + 1]);
        d += 2;
      }
      myFile.write(buffer, 1024);

      // update progress bar
      processedProgressBar += 1024;
      draw_progressbar(processedProgressBar, totalProgressBar);
    }
  }
  // Close the file:
  myFile.close();

  // Reset SSF2 Banks
  if (cartSize > 0x400000)
  {
    writeSSF2Map(0x50987E, 6); // 0xA130FD
    writeSSF2Map(0x50987F, 7); // 0xA130FF
  }

  // Calculate internal checksum
  print_Msg(F("Internal checksum..."));
  display_Update();
  if (chksum == calcCKS)
  {
    println_Msg(FS(FSTRING_OK));
    display_Update();
  }
  else
  {
    println_Msg(F("Error"));
    char calcsumStr[5];
    sprintf(calcsumStr, "%04X", calcCKS);
    println_Msg(calcsumStr);
    print_Error(FS(FSTRING_EMPTY));
    display_Update();
  }

  // More checksums
  if (SnKmode >= 2)
  {
    print_Msg(F("Lock-on checksum..."));
    if (chksumLockon == calcCKSLockon)
    {
      println_Msg(FS(FSTRING_OK));
      display_Update();
    }
    else
    {
      print_Msg(F("Error"));
      char calcsumStr[5];
      sprintf(calcsumStr, "%04X", calcCKSLockon);
      println_Msg(calcsumStr);
      print_Error(FS(FSTRING_EMPTY));
      display_Update();
    }
  }
  if (SnKmode == 3)
  {
    print_Msg(F("Adittional checksum..."));
    if (chksumSonic2 == calcCKSSonic2)
    {
      println_Msg(FS(FSTRING_OK));
      display_Update();
    }
    else
    {
      print_Msg(F("Error"));
      char calcsumStr[5];
      sprintf(calcsumStr, "%04X", calcCKSSonic2);
      println_Msg(calcsumStr);
      print_Error(FS(FSTRING_EMPTY));
      display_Update();
    }
  }
}