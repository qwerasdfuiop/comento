#ifndef OBD2UDS_H_
#define OBD2UDS_H_

// OBD, UDS 형태의 데이터 프레임 버퍼

typedef union {
  uint8_t raw[8];          /* HAL_CAN_GetRxMessage / AddTxMessage용 */
  struct {
    uint8_t pci;
    uint8_t sid;
    uint8_t reserv2;
    uint8_t reserv3;
    uint8_t reserv4;
    uint8_t reserv5;
    uint8_t reserv6;
    uint8_t reserv7;
  } field;
} CANData_t;

extern CANData_t data;

// 인터럽트 완료 확인을 위한 플래그 변수
extern volatile uint8_t can_rx_flag;

// CAN 인터럽트 요청에 대한 응답
void Process_CAN_Response(CANData_t data);

#endif
