#include "main.h"
#include "DTC.h"
#include "EEPROM.h"
#include "OBD2UDS.h"
#include "TestCase.h"

#ifdef TEST_CASE
uint8_t bb_tx_data[8];
CAN_TxHeaderTypeDef bb_tx_header;
uint8_t bb_tx_seen;
#endif

CANData_t data;

// 인터럽트 완료 확인을 위한 플래그 변수
volatile uint8_t can_rx_flag = 0;

void Process_CAN_Response(CANData_t data) {

  CAN_TxHeaderTypeDef TxHeader;
  uint32_t TxMailbox;
  uint8_t TxData[8] = {0};

  TxHeader.StdId = 0x7E8; // 응답 ID
  TxHeader.IDE = CAN_ID_STD;
  TxHeader.RTR = CAN_RTR_DATA;
  TxHeader.DLC = 8;

  int index = 4;

  // UDS 0x19: Read DTCs
	if (data.read_request.sid == 0x19) {
		if(data.read_request.status_mask != 0x01){
			return;
		}

		if(data.read_request.sub_func == 0x02){
			TxData[1] = 0x59;
			TxData[2] = 0x02;
			TxData[3] = 0x01;
			if(dtclst[UVinDTC].active == 1){
				TxData[index] = (dtclst[UVinDTC].DTC_Code >> 8) & 0xFF;
				index++;
				TxData[index] = dtclst[UVinDTC].DTC_Code & 0xFF;
				index++;
				TxData[index] = 0x01;
			}
			else if(dtclst[OVinDTC].active == 1){
				TxData[index] = (dtclst[OVinDTC].DTC_Code >> 8) & 0xFF;
				index++;
				TxData[index] = dtclst[OVinDTC].DTC_Code & 0xFF;
				index++;
				TxData[index] = 0x01;
			}
			else if(dtclst[OCinDTC].active == 1){
				TxData[index] = (dtclst[OCinDTC].DTC_Code >> 8) & 0xFF;
				index++;
				TxData[index] = dtclst[OCinDTC].DTC_Code & 0xFF;
				index++;
				TxData[index] = 0x01;
			}
			else{
                /* for misra code*/
			}
		}
		else{
			TxData[1] = 0x7F;
			TxData[2] = 0x19;
			TxData[3] = 0x12;
		}
	}
	// UDS 0x14: Clear DTCs
	else if (data.read_request.sid == 0x14) {
		for(int i = 0; i < MAX_DTC_NUM; i++){
			dtclst[i].active = 0;
		}
		EEPROM_WriteDTC();
		TxData[1] = 0x54; // 응답
	}
	else
	{
		/* for misra code*/
	}

	//   // OBD2 0x03: Read DTCs
//   if (data.field.sid == 0x03) {

// 	  TxData[1] = 0x43;
// 	  if(dtclst[UVinDTC].active == 1){
// 		  TxData[index] = (dtclst[UVinDTC].DTC_Code >> 8) & 0xFF;
// 		  index++;
// 		  TxData[index] = dtclst[UVinDTC].DTC_Code & 0xFF;
// 		  index++;
// 	  }
// 	  if(dtclst[OVinDTC].active == 1){
// 		  TxData[index] = (dtclst[OVinDTC].DTC_Code >> 8) & 0xFF;
// 		  index++;
// 		  TxData[index] = dtclst[OVinDTC].DTC_Code & 0xFF;
// 		  index++;
// 	  }
// 	  if(dtclst[OCinDTC].active == 1){
// 		  TxData[index] = (dtclst[OCinDTC].DTC_Code >> 8) & 0xFF;
// 		  index++;
// 		  TxData[index] = dtclst[OCinDTC].DTC_Code & 0xFF;
// 		  index++;
// 	  }
//   }
//   // OBD2 0x04: Clear DTCs
//   else if (data.field.sid == 0x04) {
// 	for(int i = 0; i < MAX_DTC_NUM; i++){
// 		dtclst[i].active = 0;
// 	}
// 	EEPROM_WriteDTC();
//     TxData[1] = 0x44;// 응답

#ifdef TEST_CASE
	bb_tx_header = TxHeader;
	for (int i = 0; i < 8; i++) {
	bb_tx_data[i] = TxData[i];
	}
	bb_tx_seen = 1;
#endif

  	HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox);
}
