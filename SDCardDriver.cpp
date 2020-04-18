#include "SDCardDriver.h"

#include "LUFAConfig.h"

#ifdef SDCARD_DRIVER_DEBUG
#define error(ERROR_CODE) Serial1.println(#ERROR_CODE);
#else
#define error(ERROR_CODE)
#endif

uint8_t s_sd_raw_block[512];
static uint32_t s_sd_raw_block_address;

SDCardDriver::SDCardDriver()
  //: m_spi_settings(250000, MSBFIRST, SPI_MODE0)
  : m_spi_settings(F_SPI, MSBFIRST, SPI_MODE0)
  , m_offset(0)
  , m_type(0)
{}

bool SDCardDriver::init(uint8_t chipSelectPin) 
{
  m_inBlock = false;
  m_partialBlockRead = false;
  m_chip_select_pin = chipSelectPin;

  // 16-bit init start time allows over a minute
  unsigned int t0 = millis();
  uint32_t arg;
  
  // set pin modes
  pinMode(m_chip_select_pin, OUTPUT);
  digitalWrite(m_chip_select_pin, HIGH);
  
  SPI.begin();

  // must supply min of 74 clock cycles with CS high.
  SPI.beginTransaction(m_spi_settings);
  for (uint8_t i = 0; i < 10; i++)
    SPI.transfer(0XFF);
  SPI.endTransaction();

  chipSelectLow();

  // command to go idle in SPI mode
  while ((m_status = cardCommand(CMD0, 0)) != R1_IDLE_STATE) {
    if ((millis() - t0) > SD_INIT_TIMEOUT) {
      error(SD_CARD_ERROR_CMD0);
      goto fail;
    }
  }

  // check SD version
  if ((cardCommand(CMD8, 0x1AA) & R1_ILLEGAL_COMMAND)) {
    m_type = SD_CARD_TYPE_SD1;
  } else {
    // only need last byte of r7 response
    for (uint8_t i = 0; i < 4; i++)
      m_status = SPI.transfer(0XFF);
    if (m_status != 0XAA) {
      error(SD_CARD_ERROR_CMD8);
      goto fail;
    }
    m_type = SD_CARD_TYPE_SD2;
  }

  // initialize card and send host supports SDHC if SD2
  arg = m_type == SD_CARD_TYPE_SD2 ? 0X40000000 : 0;

  while ((m_status = cardAcmd(ACMD41, arg)) != R1_READY_STATE) {
    // check for timeout
    if ((millis() - t0) > SD_INIT_TIMEOUT) {
      error(SD_CARD_ERROR_ACMD41);
      goto fail;
    }
  }
  
  // if SD2 read OCR register to check for SDHC card
  if (m_type == SD_CARD_TYPE_SD2) {
    if (cardCommand(CMD58, 0)) {
      error(SD_CARD_ERROR_CMD58);
      goto fail;
    }
    if ((SPI.transfer(0XFF) & 0XC0) == 0XC0)
      m_type = SD_CARD_TYPE_SDHC;
    // discard rest of ocr - contains allowed voltage range
    for (uint8_t i = 0; i < 3; i++)
      SPI.transfer(0XFF);
  }
  chipSelectHigh();
  return true;
  
fail:
  chipSelectHigh();
  return false;
}

uint32_t SDCardDriver::readCapacity()
{
  uint32_t capacity = 0;
  uint32_t csd_c_size = 0;
  uint8_t csd_read_bl_len = 0;
  uint8_t csd_c_size_mult = 0;

  // Read CSD register
  if (cardCommand(CMD9, 0)) {
    error(SD_CARD_ERROR_READ_REG);
    goto fail;
  }
  if (!waitStartBlock())
    goto fail;

  for(uint8_t i = 0; i < 18; ++i)
  {
    uint8_t b = SPI.transfer(0xFF);
    if (m_type == SD_CARD_TYPE_SDHC) {
      switch(i)
      {
          case 7:
              b &= 0x3f;
          case 8:
          case 9:
              csd_c_size <<= 8;
              csd_c_size |= b;
              break;
      }
      if(i == 9)
      {
          ++csd_c_size;
          capacity = csd_c_size * 512 * 1024;
      }
    } else {
      switch(i)
      {
          case 5:
              csd_read_bl_len = b & 0x0f;
              break;
          case 6:
              csd_c_size = b & 0x03;
              csd_c_size <<= 8;
              break;
          case 7:
              csd_c_size |= b;
              csd_c_size <<= 2;
              break;
          case 8:
              csd_c_size |= b >> 6;
              ++csd_c_size;
              break;
          case 9:
              csd_c_size_mult = b & 0x03;
              csd_c_size_mult <<= 1;
              break;
          case 10:
              csd_c_size_mult |= b >> 7;
              capacity = csd_c_size << (csd_c_size_mult + csd_read_bl_len + 2);
              break;
      }
    }
  }

  chipSelectHigh();
  return capacity;

fail:
  chipSelectHigh();
  return 0;
}

bool SDCardDriver::readBlock(uint32_t block, uint8_t* buffer)
{
  uint16_t block_offset = block & 0x01ff;
  uint32_t block_address = block - block_offset;
  
  if (block_offset != 0)
    error(SD_CARD_ERROR_OFFSET);
  
  if (cardCommand(CMD17, m_type == SD_CARD_TYPE_SDHC ? block_address / 512 : block_address)) {
    error(SD_CARD_ERROR_CMD17);
    goto fail;
  }

  // wait for data block (start byte 0xfe)  
  while (SPI.transfer(0xFF) != 0xfe);

  // read data block (512 byte)
  for (uint16_t i = 0; i < 512; ++i, ++buffer) {
#ifdef OPTIMIZE_SDCARD_HARDWARE_SPI
    SPDR = 0xff;
    while (!(SPSR & (1 << SPIF)));
    *buffer = SPDR;
#else
    *buffer = SPI.transfer(0xff);
#endif
  }

  // read crc16
  SPI.transfer16(0xffff);

  chipSelectHigh();
  return true;

fail:
  chipSelectHigh();
  return false;
}

bool SDCardDriver::writeBlock(uint32_t block, const uint8_t *buffer)
{ 
  uint16_t block_offset = block & 0x01ff;
  uint32_t block_address = block - block_offset;
  uint8_t status;

  if (block_offset != 0)
    error(SD_CARD_ERROR_OFFSET);
  
  if (cardCommand(CMD24, m_type == SD_CARD_TYPE_SDHC ? block_address / 512 : block_address)) {
    error(SD_CARD_ERROR_CMD24);
    goto fail;
  }
  
  SPI.transfer(0xfe);
  
#ifdef OPTIMIZE_HARDWARE_SPI
  for (uint16_t i = 0; i < 512; ++i, ++buffer) {
    while (!(SPSR & (1 << SPIF)));
    SPDR = *buffer;
  }
  while (!(SPSR & (1 << SPIF)));
#else
  SPI.transfer(buffer, 512);
#endif
  // write crc16
  SPI.transfer16(0xffff);

  status = SPI.transfer(0xff);
  if ((status & 0x1F) != 0x05) {
    error(SD_CARD_ERROR_WRITE);
    goto fail;
  }
    
  chipSelectHigh();
  return true;
    
fail:
  chipSelectHigh();
  return false; 
}

void SDCardDriver::printBlock(uint32_t block)
{    
  if (m_type != SD_CARD_TYPE_SDHC)
    block <<= 9;
  
  if (cardCommand(CMD17, block)) {
    error(SD_CARD_ERROR_CMD17);
    goto fail;
  }
  while (SPI.transfer(0xFF) != 0xfe);

  Serial1.print("Block: ");
  Serial1.println(block);
  
  for (uint16_t i = 0; i < 512; ++i) {
    uint8_t b = SPI.transfer(0xff);
    if (b < 0x10)
      Serial1.write('0');
    Serial1.print(b, HEX);
    if ((i > 0) && (i + 1) % 32 == 0)
      Serial1.println();
    else
      Serial1.write(' ');
  }
  Serial1.println();

  // read crc16
  SPI.transfer16(0xffff);

  chipSelectHigh();
  return true;

fail:
  chipSelectHigh();
  return false;
}

SDCardDriver::SDCardType SDCardDriver::type() const
{
  return static_cast<SDCardType>(m_type);
}

void SDCardDriver::chipSelectHigh(void) 
{
  digitalWrite(m_chip_select_pin, HIGH);
  if (m_chip_select_asserted) {
    m_chip_select_asserted = false;
    SPI.endTransaction();
  }
}

void SDCardDriver::chipSelectLow(void) 
{
  if (!m_chip_select_asserted) {
    m_chip_select_asserted = true;
    SPI.beginTransaction(m_spi_settings);
  }
  digitalWrite(m_chip_select_pin, LOW);
}

uint8_t SDCardDriver::cardCommand(uint8_t cmd, uint32_t arg) 
{
  // end read if in partialBlockRead mode
  readEnd();

  // select card
  chipSelectLow();

  // wait up to 300 ms if busy
  waitNotBusy(300);

  // send command
  SPI.transfer(cmd | 0x40);

  // send argument
  for (int8_t s = 24; s >= 0; s -= 8)
    SPI.transfer(arg >> s);

  // send CRC
  uint8_t crc = 0XFF;
  if (cmd == CMD0)
    crc = 0X95;  // correct crc for CMD0 with arg 0
  if (cmd == CMD8)
    crc = 0X87;  // correct crc for CMD8 with arg 0X1AA
  SPI.transfer(crc);

  // wait for response
  for (uint8_t i = 0; ((m_status = SPI.transfer(0xFF)) & 0X80) && i != 0XFF; i++);
  return m_status;
}

uint8_t SDCardDriver::cardAcmd(uint8_t cmd, uint32_t arg)
{
  cardCommand(CMD55, 0);
  return cardCommand(cmd, arg);
}

void SDCardDriver::readEnd()
{
  if (m_inBlock) {
    // skip data and crc
    while (m_offset++ < 514)
      SPI.transfer(0xFF);
    chipSelectHigh();
    m_inBlock = false;
  }
}

bool SDCardDriver::waitNotBusy(unsigned int timeout_ms)
{
  unsigned int t0 = millis();
  unsigned int d;
  do {
    if (SPI.transfer(0xFF) == 0XFF)
      return true;
    d = millis() - t0;
  } while (d < timeout_ms);
  return false;
}

bool SDCardDriver::waitStartBlock()
{
  unsigned int t0 = millis();
  while ((m_status = SPI.transfer(0xFF)) == 0XFF) {
    unsigned int d = millis() - t0;
    if (d > SD_READ_TIMEOUT) {
      error(SD_CARD_ERROR_READ_TIMEOUT);
      goto fail;
    }
  }
  if (m_status != DATA_START_BLOCK) {
    error(SD_CARD_ERROR_READ);
    goto fail;
  }
  return true;

fail:
  chipSelectHigh();
  return false;
}

bool SDCardDriver::readRegister(uint8_t cmd, void* buf)
{
  uint8_t *dst = reinterpret_cast<uint8_t*>(buf);
  if (cardCommand(cmd, 0)) {
    error(SD_CARD_ERROR_READ_REG);
    goto fail;
  }
  if (!waitStartBlock())
    goto fail;
  // transfer data
  for (uint16_t i = 0; i < 16; i++)
    dst[i] = SPI.transfer(0xff);
  SPI.transfer16(0xffff); // read crc16
  chipSelectHigh();
  return true;

fail:
  chipSelectHigh();
  return false;
}
