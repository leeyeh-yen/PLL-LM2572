/********************************** (C) COPYRIGHT *******************************
* File Name          : lmx2572.h
* Description        : LMX2572 SPI register programming interface.
*******************************************************************************/
#ifndef __LMX2572_H
#define __LMX2572_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ch32v30x.h"

void LMX2572_Init(void);
void LMX2572_WriteAllRegisters(void);
void LMX2572_SetFrequencyMHz(uint32_t frequency_mhz);
uint32_t LMX2572_GetNextSweepFrequencyMHz(uint32_t current_frequency_mhz);
uint8_t LMX2572_GetLockDetect(void);

#ifdef __cplusplus
}
#endif

#endif
