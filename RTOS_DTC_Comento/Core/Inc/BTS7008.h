#ifndef BTS7008_H_
#define BTS7008_H_

#include <stdint.h>

/*
 * 600옴 저항 2개로 전압 분압하여 ADC에 읽힌다고 가정
 * CHANNEL을 ON한 상태에서 FAULT CURRENT 여부인지만 판단(OPEN LOAD 미구현)
 */
#define BTS7008_FAULT_IIS_THRESHOLD   0.0044f
#define BTS7008_VIS_SCALE         2.0f
#define BTS7008_RSENSE_OHM        1200.0f
#define RESOLUTION_LEVEL_MAX      4095U
#define BTS7008_ADC_VREF              3.3f

typedef enum {
    BTS7008_DIAG_NORMAL = 0,
    BTS7008_DIAG_FAULT
} BTS7008_DiagState;

void BTS7008_Init(void);

//IN0핀 제어 함수
void BTS7008_SetOutput(uint8_t state);

//ADC값을 통해 FAULT 여부 계산
BTS7008_DiagState BTS7008_CheckFault(uint16_t adc_value);

#endif