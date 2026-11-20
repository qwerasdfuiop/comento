#include <stdint.h>
#include "DTC.h"
#include "PMIC.h"
#include "EEPROM.h"
#include "TestCase.h"

// DTC_t 구조체들의 배열
DTC_t dtclst[MAX_DTC_NUM] = {
	{{0x05, 0x62, 0x16}, 0},
	{{0x05, 0x63, 0x17}, 0},
	{{0x31, 0x01, 0x19}, 0},
};

//PMIC 레지스터 버퍼로부터 DTC로 변환하는 함수
void DTCProcessFault(void){
#if TEST_CASE
#else
	if(voltage_reg_buff.raw || current_reg_buff.raw){
#endif
		//레지스터 값이 0이 아니면 confirmedDTC = 1
		if (voltage_reg_buff.bit.buckx_uv) {
			if(PG_volt == 1){
				if(PMIC_Read_Power_Status() == 1){
					dtclst[UVinDTC].DTCStatus |= 0x08;
				}
			}
		}
		if (voltage_reg_buff.bit.buckx_ov) {
			if(PMIC_Read_Power_Status() == 1){
			  dtclst[OVinDTC].DTCStatus |= 0x08;
			}
		}
		if (current_reg_buff.bit.buckx_oc) {
			if(PG_volt == 1){
				if(PMIC_Read_Power_Status() == 1){
					dtclst[OCinDTC].DTCStatus |= 0x08;
				}
			}
		}
#if TEST_CASE
#else
		//고장이 났을 경우에만 EEPROM에 고장 정보 WRITE
		EEPROM_WriteDTC();
	}
#endif
}
