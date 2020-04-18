#ifndef SDCARDDRIVER_H
#define SDCARDDRIVER_H

#include <SPI.h>

class SDCardDriver {
public:
  SDCardDriver();

  bool init(uint8_t chipSelectPin);

  uint32_t readCapacity();
  
  bool readBlock(uint32_t block, uint8_t *buffer);
  bool writeBlock(uint32_t block, const uint8_t *buffer);

  void printBlock(uint32_t block);
  
  enum SDCardType {
    SD_CARD_TYPE_SD1 = 1, // Standard capacity V1 SD card
    SD_CARD_TYPE_SD2, // Standard capacity V2 SD card
    SD_CARD_TYPE_SDHC, // High Capacity SD card
  };
  SDCardType type() const;

private:
  void chipSelectHigh();
  void chipSelectLow();
  uint8_t cardCommand(uint8_t cmd, uint32_t arg);
  uint8_t cardAcmd(uint8_t cmd, uint32_t arg);
  void readEnd();
  bool waitNotBusy(unsigned int timeout_ms);
  bool waitStartBlock();
  bool readRegister(uint8_t cmd, void* buf);

  uint8_t m_chip_select_pin;
  uint8_t m_status;
  uint8_t m_type;
  uint16_t m_offset;
  uint32_t m_block;
  bool m_chip_select_asserted;
  bool m_inBlock;
  bool m_partialBlockRead;
  SPISettings m_spi_settings;
  
  static unsigned int constexpr SD_INIT_TIMEOUT = 2000;
  static unsigned int constexpr SD_READ_TIMEOUT = 300;

  enum SDCardCommands {
    CMD0 = 0x00, // GO_IDLE_STATE - init card in spi mode if CS low
    CMD8 = 0x08, // SEND_IF_COND - verify SD Memory Card interface operating condition.
    CMD9 = 0x09, // SEND_CSD - read the Card Specific Data (CSD register)
    CMD10 = 0x0A, // SEND_CID - read the card identification information (CID register) 
    CMD17 = 0x11, // READ_BLOCK - read a single data block from the card
    CMD24 = 0x18, // WRITE_BLOCK - write a single data block to the card
    CMD55 = 0x37, // APP_CMD - escape for application specific command
    CMD58 = 0x3A, // READ_OCR - read the OCR register of a card
    ACMD23 = 0x17, // SET_WR_BLK_ERASE_COUNT - Set the number of write blocks to be pre-erased before writing
    ACMD41 = 0x29, // SD_SEND_OP_COMD - Sends host capacity support information and activates the card's initialization process
  };
  enum SDCardStatus {
    R1_READY_STATE = 0x00, // status for card in the ready state
    R1_IDLE_STATE = 0x01, // status for card in the idle state
    R1_ILLEGAL_COMMAND = 0x04, // status bit for illegal command
    DATA_START_BLOCK = 0xFE, // start data token for read or write single block
  };
  
};

#endif // SDCARDDRIVER_H
