#ifndef OBD2UDS_H_
#define OBD2UDS_H_

// UDS 형태의 데이터 프레임 버퍼

typedef union {
  uint8_t raw[8];        
  struct {
    uint8_t iso_tp;
    uint8_t sid;
    uint8_t sub_func;
    uint8_t status_mask;
    uint8_t reserv4;
    uint8_t reserv5;
    uint8_t reserv6;
    uint8_t reserv7;
  } read_request;
} CANData_t;

// UDS 데이터 처리를 위한 CANDATA_t 인스턴스
extern CANData_t data;

// 인터럽트 완료 확인을 위한 플래그 변수
extern volatile uint8_t can_rx_flag;

#ifdef TEST_CASE
extern uint8_t bb_tx_data[8];
extern CAN_TxHeaderTypeDef bb_tx_header;
extern uint8_t bb_tx_seen;
#endif
// CAN 인터럽트 요청에 대한 응답
void Process_CAN_Response(CANData_t data);

#endif
