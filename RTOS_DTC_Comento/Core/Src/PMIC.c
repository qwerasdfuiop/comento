#include <main.h>
#include "PMIC.h"

voltage_fault_reg voltage_reg_buff;

current_fault_reg current_reg_buff;

//temperature_fault_reg temperature_reg_buff;

// 임의로 V_REF 단계 설정 가능.
uint16_t v_ref_set = V_REF_SET;

volatile uint8_t is_i2c_busy = 0;

void PMIC_Read_Fault(void){

	is_i2c_busy = 1;
	HAL_I2C_Mem_Read_DMA(&hi2c1, PMIC_I2C_ADDR, VOLTAGE_FAULT_ADDR, I2C_MEMADD_SIZE_8BIT, &voltage_reg_buff.raw, 1);
	while(is_i2c_busy);

	is_i2c_busy = 1;
	HAL_I2C_Mem_Read_DMA(&hi2c1, PMIC_I2C_ADDR, CURRENT_FAULT_ADDR, I2C_MEMADD_SIZE_8BIT, &current_reg_buff.raw, 1);
	while(is_i2c_busy);

//	is_i2c_busy = 1;
//	HAL_I2C_Mem_Read_DMA(&hi2c1, PMIC_I2C_ADDR, TEMPERATURE_FAULT_ADDR, I2C_MEMADD_SIZE_8BIT, &temperature_reg_buff.raw, 1);
//	while(is_i2c_busy);
}

void PMIC_Vref_Change(uint16_t v_ref_set){

	uint8_t v_ref_temp = 0;

    HAL_I2C_Mem_Read(&hi2c1, PMIC_I2C_ADDR, PMIC_V_REFA_HIGH, I2C_MEMADD_SIZE_8BIT, &v_ref_temp, 1, HAL_MAX_DELAY);
    v_ref_temp = (v_ref_temp & 0xFC) | (uint8_t)(v_ref_set >> 8);
	HAL_I2C_Mem_Write(&hi2c1, PMIC_I2C_ADDR, PMIC_V_REFA_HIGH, I2C_MEMADD_SIZE_8BIT, &v_ref_temp, 1, HAL_MAX_DELAY);

	v_ref_temp = v_ref_set & 0xFF;

	HAL_I2C_Mem_Write(&hi2c1, PMIC_I2C_ADDR, PMIC_V_REFA_LOW, I2C_MEMADD_SIZE_8BIT, &v_ref_temp, 1, HAL_MAX_DELAY);

}
