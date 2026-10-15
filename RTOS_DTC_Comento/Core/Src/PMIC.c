#include <main.h>
#include "PMIC.h"

// UV, OV 고장 정보 레지스터로부터 읽은 값을 저장하는 변수
voltage_fault_reg voltage_reg_buff;
// OC 고장 정보 레지스터로부터 읽은 값을 저장하는 변수
current_fault_reg current_reg_buff;


// 임의로 V_REF 단계 설정 가능.
uint16_t v_ref_set = V_REF_SET;
// I2C 버스 사용 중인지 확인하기 위한 플래그
volatile uint8_t is_i2c_busy = 0;

// 각각의 fault register로부터 고장 정보 읽기
void PMIC_Read_Fault(void){
	// UV, OV 고장 정보 레지스터로부터 고장 정보 읽기
	is_i2c_busy = 1;
	HAL_I2C_Mem_Read_DMA(&hi2c1, PMIC_I2C_ADDR, VOLTAGE_FAULT_ADDR, I2C_MEMADD_SIZE_8BIT, &voltage_reg_buff.raw, 1);
	while(is_i2c_busy);
	// OC 고장 정보 레지스터로부터 고장 정보 읽기
	is_i2c_busy = 1;
	HAL_I2C_Mem_Read_DMA(&hi2c1, PMIC_I2C_ADDR, CURRENT_FAULT_ADDR, I2C_MEMADD_SIZE_8BIT, &current_reg_buff.raw, 1);
	while(is_i2c_busy);

}

// v_reg_set에 따라 PMIC의 VRET 값 설정.
void PMIC_Vref_Change(uint16_t v_ref_set){

	/*
	PMIC_V_REFA_HIGH 레지스터의 하위 2비트가 VREF 설정 비트에 해당하기 때문에
	먼저 PMIC_V_REFA_HIGH 레지스터 값을 읽어들인후에
	상위 6비트는 그대로 두고 하위 2비트만 변경
	*/
	uint8_t v_ref_temp = 0;

    HAL_I2C_Mem_Read(&hi2c1, PMIC_I2C_ADDR, PMIC_V_REFA_HIGH, I2C_MEMADD_SIZE_8BIT, &v_ref_temp, 1, HAL_MAX_DELAY);
    v_ref_temp = (v_ref_temp & 0xFC) | (uint8_t)(v_ref_set >> 8);
	HAL_I2C_Mem_Write(&hi2c1, PMIC_I2C_ADDR, PMIC_V_REFA_HIGH, I2C_MEMADD_SIZE_8BIT, &v_ref_temp, 1, HAL_MAX_DELAY);

	// PMIC_V_REFA_LOW 레지스터 값 변경
	v_ref_temp = v_ref_set & 0xFF;

	HAL_I2C_Mem_Write(&hi2c1, PMIC_I2C_ADDR, PMIC_V_REFA_LOW, I2C_MEMADD_SIZE_8BIT, &v_ref_temp, 1, HAL_MAX_DELAY);

}
