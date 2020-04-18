/*
             LUFA Library
     Copyright (C) Dean Camera, 2017.
  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2017  Dean Camera (dean [at] fourwalledcubicle [dot] com)
  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.
  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Header file for DataflashManager.c.
 */

#ifndef SDCARDMANAGER_H
#define SDCARDMANAGER_H

/* Includes: */
#include <avr/io.h>

#if defined(__cplusplus)

#include "SDCardDriver.h"

extern SDCardDriver s_sdcard_driver;

extern "C" {
#endif
#include "Descriptors.h"

#define DISK_READ_ONLY              false
#define TOTAL_LUNS                  1

#define VIRTUAL_MEMORY_BLOCK_SIZE   512

#define LUN_MEDIA_BLOCKS            (SDCardManager_NumBlocks() / TOTAL_LUNS)    

bool SDCardManager_Init(uint8_t chipSelectPin);

uint32_t SDCardManager_NumBlocks(void);

bool SDCardManager_CheckDataflashOperation();

void SDCardManager_WriteBlocks(USB_ClassInfo_MS_Device_t* const MSInterfaceInfo,
                                  uint32_t BlockAddress,
                                  uint16_t TotalBlocks);
void SDCardManager_ReadBlocks(USB_ClassInfo_MS_Device_t* const MSInterfaceInfo,
                                 uint32_t BlockAddress,
                                 uint16_t TotalBlocks);
#if defined(__cplusplus)
}
#endif

#endif // SDCARDMANAGER_H
