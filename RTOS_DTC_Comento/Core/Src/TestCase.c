#include "main.h"
#include "TestCase.h"
#include "PMIC.h"
#include "DTC.h"
#include "OBD2UDS.h"
#include <stdio.h>
#include <string.h>

// 모든 테스트 케이스 실행 및 결과 출력
void TestCase_RunAll(void)
{
    Test_PrintResult("WB-01 Fault bit decode",WB_TC01_FaultBitDecode());
    Test_PrintResult("WB-02 Fault to DTC",WB_TC02_FaultToDTC());
    Test_PrintResult("BB-01 Read DTC",BB_TC01_ReadDtcResponse());
    Test_PrintResult("BB-02 Unsupported Sub Function",BB_TC02_UnsupportedSubFunction());
}

//각 테스트 케이스 함수 실행 결과 출력 함수
void Test_PrintResult(const char *name, TestResult result)
{
    char message[80];
    snprintf(message, sizeof(message),
             "[TEST] %s : %s\r\n",
             name,
             result == TEST_PASS ? "PASS" : "FAIL");
    HAL_UART_Transmit(&huart4,(uint8_t *)message,strlen(message),HAL_MAX_DELAY);
}

//PMIC 메모리 값을 임의로 설정해서 reg_buff값을 테스트하는 함수
TestResult WB_TC01_FaultBitDecode(void)
{
	//각 레지스터 버퍼에 임의값으로 초기화
    voltage_reg_buff.raw = 0x01;
    current_reg_buff.raw = 0x10;

    //의도한대로 버퍼에 레지스터값이 저장되면 1, 그렇지 않으면 0
    uint8_t check_uv;
    uint8_t check_ov;
    uint8_t check_oc;

    check_uv = (voltage_reg_buff.bit.buckx_uv == 0);
    check_ov = (voltage_reg_buff.bit.buckx_ov == 1);
    check_oc = (current_reg_buff.bit.buckx_oc == 16);

    if(!(check_uv && check_ov && check_oc)){
    	return TEST_FAIL;
    }
    return TEST_PASS;
}

//reg_buff값으로부터 DTC로 변환이 잘 됐는지 확인하는 함수
TestResult WB_TC02_FaultToDTC(void)
{
	//각 레지스터 버퍼에 임의값으로 초기화
    voltage_reg_buff.raw = 0x01;
    current_reg_buff.raw = 0x23;

    //각 레지스터 버퍼로부터 DTC로 변환
    DTCProcessFault();

    //테스트 플래그
    uint8_t check_uv;
    uint8_t check_ov;
    uint8_t check_oc;

    //의도한대로 DTC active값이 저장되면 1, 그렇지 않으면 0
    check_uv = (dtclst[UVinDTC].active == 0);
    check_ov = (dtclst[OVinDTC].active == 1);
    check_oc = (dtclst[OCinDTC].active == 1);
    if(!(check_uv && check_ov && check_oc)){
    	return TEST_FAIL;
    }

    //의도한대로 DTC code값이 저장되면 1, 그렇지 않으면 0
    check_uv = (dtclst[UVinDTC].DTC_Code == UV);
    check_ov = (dtclst[UVinDTC].DTC_Code == OV);
    check_oc = (dtclst[UVinDTC].DTC_Code == OC);
    if(!(check_uv && check_ov && check_oc)){
    	return TEST_FAIL;
    }

    return TEST_PASS;
}

static void BB_ResetFaults(void)
{
    voltage_reg_buff.raw = 0;
    current_reg_buff.raw = 0;
    for (int i = 0; i < MAX_DTC_NUM; i++) {
        dtclst[i].active = 0;
    }
    bb_tx_seen = 0;
}

/* OV 고장 후 ReadDTC(0x19 02 01) 응답이 규격 프레임인지 */
TestResult BB_TC01_ReadDtcResponse(void)
{
    CANData_t req = {0};
    const uint8_t expect[] = {0x59, 0x02, 0x01, 0x34, 0x56, 0x01};

    BB_ResetFaults();
    voltage_reg_buff.raw = 0x10; /* 상위 니블 OV */
    DTCProcessFault();

    req.read_request.sid = 0x19;
    req.read_request.sub_func = 0x02;
    req.read_request.status_mask = 0x01;
    Process_CAN_Response(req);

    if (memcmp(&bb_tx_data[1], expect, sizeof(expect)) != 0) {
        return TEST_FAIL;
    }
    return TEST_PASS;
}

/* 지원하지 않는 sub-function이면 NRC 0x12 */
TestResult BB_TC02_UnsupportedSubFunction(void)
{
    CANData_t req = {0};
    const uint8_t expect[] = {0x7F, 0x19, 0x12};

    BB_ResetFaults();
    req.read_request.sid = 0x19;
    req.read_request.sub_func = 0xFF;
    req.read_request.status_mask = 0x01;
    Process_CAN_Response(req);

    if (memcmp(&bb_tx_data[1], expect, sizeof(expect)) != 0) {
        return TEST_FAIL;
    }
    return TEST_PASS;
}
