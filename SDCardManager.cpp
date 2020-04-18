#include "SDCardManager.h"

#include "Arduino.h"

SDCardDriver s_sdcard_driver;

static uint32_t s_cached_total_blocks = 0;
extern uint8_t s_sd_raw_block[512];

bool SDCardManager_Init(uint8_t chipSelectPin)
{
  s_sdcard_driver = SDCardDriver();
  if (!s_sdcard_driver.init(chipSelectPin))
    return false;
  //delay(100);
  s_cached_total_blocks = s_sdcard_driver.readCapacity() / VIRTUAL_MEMORY_BLOCK_SIZE;
  return (s_cached_total_blocks > 0);
}

uint32_t SDCardManager_NumBlocks(void)
{
  return s_cached_total_blocks;
}

bool SDCardManager_CheckDataflashOperation()
{
  return true;
}

/** Writes blocks (OS blocks, not Dataflash pages) to the storage medium, the board Dataflash IC(s),
 * from
 *  the pre-selected data OUT endpoint. This routine reads in OS sized blocks from the endpoint and
 * writes
 *  them to the Dataflash in Dataflash page sized blocks.
 *
 *  \param[in] MSInterfaceInfo  Pointer to a structure containing a Mass Storage Class configuration
 * and state
 *  \param[in] BlockAddress  Data block starting address for the write sequence
 *  \param[in] TotalBlocks   Number of blocks of data to write
 */
void SDCardManager_WriteBlocks(USB_ClassInfo_MS_Device_t *const MSInterfaceInfo,
                               uint32_t BlockAddress, uint16_t TotalBlocks) 
{
#ifdef SDCARD_DRIVER_DEBUG
  Serial1.print("W ");
  Serial1.print(BlockAddress);
  Serial1.write(' ');
  Serial1.println(TotalBlocks);
#endif

   /* Wait until endpoint is ready before continuing */
  if (Endpoint_WaitUntilReady())
    return;

  while (TotalBlocks) {
    uint8_t *buffer = s_sd_raw_block;
    for (uint16_t offset = 0; offset < VIRTUAL_MEMORY_BLOCK_SIZE; offset += MASS_STORAGE_IO_EPSIZE) {
      if (!Endpoint_IsReadWriteAllowed()) {
        Endpoint_ClearOUT();
        if (Endpoint_WaitUntilReady())
          break;
      }
      
      for (uint8_t i = 0; i < MASS_STORAGE_IO_EPSIZE; ++i, ++buffer)
        *buffer = Endpoint_Read_8();
      
      /* Check if the current command is being aborted by the host */
      if (MSInterfaceInfo->State.IsMassStoreReset)
        return;
    }
    if (!s_sdcard_driver.writeBlock(BlockAddress * VIRTUAL_MEMORY_BLOCK_SIZE, s_sd_raw_block))
      break;

    /* Decrement the blocks remaining counter */
    BlockAddress++;
    TotalBlocks--;
  }

  /* If the endpoint is empty, clear it ready for the next packet from the host */
  if (!(Endpoint_IsReadWriteAllowed()))
      Endpoint_ClearOUT();
}

/** Reads blocks (OS blocks, not Dataflash pages) from the storage medium, the board Dataflash
 * IC(s), into
 *  the pre-selected data IN endpoint. This routine reads in Dataflash page sized blocks from the
 * Dataflash
 *  and writes them in OS sized blocks to the endpoint.
 *
 *  \param[in] MSInterfaceInfo  Pointer to a structure containing a Mass Storage Class configuration
 * and state
 *  \param[in] BlockAddress  Data block starting address for the read sequence
 *  \param[in] TotalBlocks   Number of blocks of data to read
 */
void SDCardManager_ReadBlocks(USB_ClassInfo_MS_Device_t *const MSInterfaceInfo,
                          uint32_t BlockAddress, uint16_t TotalBlocks) 
{
#ifdef SDCARD_DRIVER_DEBUG
  Serial1.print("R ");
  Serial1.print(BlockAddress);
  Serial1.write(' ');
  Serial1.println(TotalBlocks);
#endif
  
  /* Wait until endpoint is ready before continuing */
  if (Endpoint_WaitUntilReady())
    return;

  while (TotalBlocks) {
    if (!s_sdcard_driver.readBlock(BlockAddress * VIRTUAL_MEMORY_BLOCK_SIZE, s_sd_raw_block))
      break;
    uint8_t *buffer = s_sd_raw_block;
    for (uint16_t offset = 0; offset < VIRTUAL_MEMORY_BLOCK_SIZE; offset += MASS_STORAGE_IO_EPSIZE) {
      if (!Endpoint_IsReadWriteAllowed())  {
        Endpoint_ClearIN();
        if (Endpoint_WaitUntilReady())
          break;
      }
    
      for (uint8_t i = 0; i < MASS_STORAGE_IO_EPSIZE; ++i, ++buffer)
        Endpoint_Write_8(*buffer);
    }
      
    /* Decrement the blocks remaining counter and reset the sub block counter */
    BlockAddress++;
    TotalBlocks--;
  }

  /* If the endpoint is empty, clear it ready for the next packet from the host */
  if (!(Endpoint_IsReadWriteAllowed()))
    Endpoint_ClearOUT();
}
