#include <stdint.h>
#include "DTC.h"
#include "PMIC.h"
#include "EEPROM.h"
#include "TestCase.h"

// 위 구조체들의 배열
DTC_t dtclst[MAX_DTC_NUM] = {
		{UV, 0},
		{OV, 0},
		{OC, 0},
		//{HIGH_TEMP, 0},
};

//PMIC 레지스터 버퍼로부터 DTC로 변환하는 함수
void DTCProcessFault(void){
#ifdef TEST_CASE
#else
	if(voltage_reg_buff.raw || current_reg_buff.raw){
#endif
		//레지스터 값이 0이 아니면 active = 1
		if (voltage_reg_buff.bit.buckx_uv) {
		  if (dtclst[UVinDTC].active == 0) {
			  dtclst[UVinDTC].active = 1;
		  }
		}
		if (voltage_reg_buff.bit.buckx_ov) {
		  if (dtclst[OVinDTC].active == 0) {
			  dtclst[OVinDTC].active = 1;
		  }
		}
		if (current_reg_buff.bit.buckx_oc) {
		  if (dtclst[OCinDTC].active == 0) {
			  dtclst[OCinDTC].active = 1;
		  }
		}
//		if (temperature_reg_buff.bit.high_temp) {
//		  if (dtclst[HIGH_TEMPinDTC].active == 0) {
//			  dtclst[HIGH_TEMPinDTC].active = 1;
//		  }
//		}
#ifdef TEST_CASE
#else
		//고장이 났을 경우에만 EEPROM에 고장 정보 WRITE
		EEPROM_WriteDTC();
	}
#endif
}
