#include "LUFAConfig.h"

#include <LUFA.h>
#include <avr/wdt.h>

#include "MassStorage.h"
#include "SDCardManager.h"

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial1.begin(9600);
  Serial1.print("Init ... ");

  if (!SDCardManager_Init(SS))
    Serial1.println("ERR");
  else
    Serial1.println("OK");
    
  Serial1.print("Blocks: ");
  Serial1.println(SDCardManager_NumBlocks());

  SetupHardware();
}

void loop() {
  ProcessHardware();
}
