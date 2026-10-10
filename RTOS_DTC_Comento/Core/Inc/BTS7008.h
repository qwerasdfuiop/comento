#ifndef BTS7008_H_
#define BTS7008_H_

#include <stdint.h>

/*
 * Project assumption.
 * 실제 HW에서는 IS-ADC 회로에 맞춰 결정해야 함.
 */
#define BTS7008_FAULT_IIS_THRESHOLD   0.0044f
#define BTS7008_VIS_SCALE         2.0f
#define BTS7008_RSENSE_OHM        1200.0f
#define RESOLUTION_LEVEL_MAX      4095U

typedef enum {
    BTS7008_DIAG_NORMAL = 0,
    BTS7008_DIAG_FAULT
} BTS7008_DiagState;

void BTS7008_Init(void);

void BTS7008_SetOutput(uint8_t state);

BTS7008_DiagState BTS7008_CheckFault(uint16_t adc_value);

#endif