#include "main.h"
#include "DTC.h"
#include "EEPROM.h"
#include "OBD2UDS.h"
#include "TestCase.h"

// 테스트 케이스에서 사용하기 위한 변수
#if TEST_CASE
uint8_t bb_tx_data[8];
CAN_TxHeaderTypeDef bb_tx_header;
uint8_t bb_tx_seen;
#endif

// UDS 데이터 처리를 위한 CANDATA_t 인스턴스
CANData_t data;

// 인터럽트 완료 확인을 위한 플래그 변수
volatile uint8_t can_rx_flag = 0;

// UDS 요청을 처리하기 위한 함수
void Process_CAN_Response(CANData_t data) {

  CAN_TxHeaderTypeDef TxHeader;
  uint32_t TxMailbox;
  uint8_t TxData[8] = {0};

  TxHeader.StdId = 0x7E8; // 응답 ID
  TxHeader.IDE = CAN_ID_STD;
  TxHeader.RTR = CAN_RTR_DATA;
  TxHeader.DLC = 8;

  int index = 4;

  // UDS 0x19 DTC 읽기 요청 처리
	if (data.read_request.sid == 0x19) {
		if(data.read_request.status_mask != 0x01){
			return;
		}
        // 고장이 있는 항목만 송신 버퍼에 추가
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
		// 지원하지 않는 sub function 요청 응답
		else{
			TxData[1] = 0x7F;
			TxData[2] = 0x19;
			TxData[3] = 0x12;
		}
	}
	// UDS 0X14 DTC 초기화 요청 처리
	else if (data.read_request.sid == 0x14) {
		for(int i = 0; i < MAX_DTC_NUM; i++){
			dtclst[i].active = 0;
		}
		EEPROM_WriteDTC();
		TxData[1] = 0x54;
	}
	else
	{
		/* for misra code*/
	}

	// 블랙박스 테스트에서 확인하기 위한 변수로 송신 버퍼의 내용을 복사
#if TEST_CASE
	bb_tx_header = TxHeader;
	for (int i = 0; i < 8; i++) {
		bb_tx_data[i] = TxData[i];
	}
	bb_tx_seen = 1;
#endif
	// CAN 송신
  	HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox);
}
