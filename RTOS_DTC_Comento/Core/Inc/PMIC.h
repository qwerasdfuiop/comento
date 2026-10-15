#ifndef PMIC_H_
#define PMIC_H_

// PMIC 주소
#define PMIC_I2C_ADDR  (0x60 << 1)
#define PMIC_V_REFA_HIGH  0x13
#define PMIC_V_REFA_LOW  0x14
#define V_REF_SET 0xFF // 임의의 값 설정

// fault 레지스터들의 메모리 주소를 열거형으로
enum PMICRegisters_t {
	VOLTAGE_FAULT_ADDR = 0x07,
	CURRENT_FAULT_ADDR = 0x08,
	TEMPERATURE_FAULT_ADDR = 0x09,
};

// UV, OV를 확인할 수 있는 레지스터 버퍼
typedef union {
  uint8_t raw;          
  struct {
    uint8_t buckx_uv	:4;
    uint8_t buckx_ov	:4;
  } bit;
} voltage_fault_reg;



// OC를 확인할 수 있는 레지스터 버퍼
typedef union {
  uint8_t raw;          /* HAL_CAN_GetRxMessage / AddTxMessage용 */
  struct {
    uint8_t buckx_oc;
  } bit;
} current_fault_reg;

// UV, OV 고장 정보 레지스터로부터 읽은 값을 저장하는 변수
extern voltage_fault_reg voltage_reg_buff;
// OC 고장 정보 레지스터로부터 읽은 값을 저장하는 변수
extern current_fault_reg current_reg_buff;
// 임의로 V_REF 단계 설정 가능.
extern uint16_t v_ref_set;
// I2C 버스 사용 중인지 확인하기 위한 플래그
extern volatile uint8_t is_i2c_busy;

// PMIC의 VREF 값 변경
void PMIC_Vref_Change(uint16_t v_ref_set);

// PMIC로부터 i2c로 fault여부 읽는 함수
void PMIC_Read_Fault(void);

#endif
