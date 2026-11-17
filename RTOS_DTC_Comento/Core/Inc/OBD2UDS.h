#ifndef OBD2UDS_H_
#define OBD2UDS_H_

// UDS 형태의 데이터 프레임 버퍼

#define UDS_REQ_ID   0x6A0U
#define UDS_RESP_ID  0x6A8U
#define DTC_STATUS_AVAIL 0x08U

typedef union {
  uint8_t raw[8];          /* HAL_CAN_GetRxMessage / AddTxMessage용 */
  struct {
    uint8_t DLC_length;
    uint8_t service_id;
    uint8_t sub_function;
    uint8_t status_mask;
    uint8_t reserv4;
    uint8_t reserv5;
    uint8_t reserv6;
    uint8_t reserv7;
  } read_request;
  struct {
    uint8_t DLC_length;
    uint8_t service_id;
    uint8_t sub_function;
    uint8_t status_available_mask;
    uint8_t DTC[3];
    uint8_t DTC_status;
  } read_response;
  struct {
    uint8_t DLC_length;
    uint8_t service_id;
    uint8_t clear_mask_high;
    uint8_t clear_mask_mid;
    uint8_t clear_mask_low;
    uint8_t reserv5;
    uint8_t reserv6;
    uint8_t reserv7;
  } clear_request;
} CANData_t;

// UDS 데이터 처리를 위한 CANDATA_t 인스턴스
extern CANData_t data;

// 인터럽트 완료 확인을 위한 플래그 변수
extern volatile uint8_t can_rx_flag;

// 테스트 케이스에서 사용하기 위한 변수
#if TEST_CASE
extern uint8_t bb_tx_data[8];
extern CAN_TxHeaderTypeDef bb_tx_header;
extern uint8_t bb_tx_seen;
#endif
// CAN 인터럽트 요청에 대한 응답
void Process_CAN_Response(CANData_t data);

#endif
