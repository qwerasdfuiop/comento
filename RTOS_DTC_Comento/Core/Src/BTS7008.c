#include "main.h"
#include "BTS7008.h"

void BTS7008_Init(void)
{
    /* Channel 0 ON */
    HAL_GPIO_WritePin(
        BTS_IN0_GPIO_Port,
        BTS_IN0_Pin,
        GPIO_PIN_SET
    );

    /* DSEL LOW -> Channel 0 diagnosis */
    HAL_GPIO_WritePin(
        BTS_DSEL_GPIO_Port,
        BTS_DSEL_Pin,
        GPIO_PIN_RESET
    );

    /* Diagnosis Enable */
    HAL_GPIO_WritePin(
        BTS_DEN_GPIO_Port,
        BTS_DEN_Pin,
        GPIO_PIN_SET
    );
}

void BTS7008_SetOutput(uint8_t state)
{
    HAL_GPIO_WritePin(
        BTS_IN0_GPIO_Port,
        BTS_IN0_Pin,
        state
            ? GPIO_PIN_SET
            : GPIO_PIN_RESET
    );
}


BTS7008_DiagState BTS7008_CheckFault(uint16_t adc_value)
{

    float vis_value = adc_value * BTS7008_VIS_SCALE * BTS7008_ADC_VREF / RESOLUTION_LEVEL_MAX;
    float iis_value = vis_value / BTS7008_RSENSE_OHM;
    if (iis_value >= BTS7008_FAULT_IIS_THRESHOLD) {
        return BTS7008_DIAG_FAULT;
    }

    return BTS7008_DIAG_NORMAL;
}