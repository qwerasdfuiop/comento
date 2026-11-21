#include "main.h"
#include "TestCase.h"
#include "PMIC.h"
#include "DTC.h"
#include "OBD2UDS.h"
#include <stdio.h>
#include <string.h>

#if TEST_CASE

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

    //의도한대로 버퍼에 레지스터값이 저장되면 check_ 변수에 1, 그렇지 않으면 0
    uint8_t check_uv;
    uint8_t check_ov;
    uint8_t check_oc;

    check_uv = (voltage_reg_buff.bit.buckx_uv == 1);
    check_ov = (voltage_reg_buff.bit.buckx_ov == 0);
    check_oc = (current_reg_buff.bit.buckx_oc == 16);

    if(!(check_uv && check_ov && check_oc)){
    	return TEST_FAIL;
    }
    return TEST_PASS;
}

//reg_buff값으로부터 DTC로 변환이 잘 됐는지 확인하는 함수
TestResult WB_TC02_FaultToDTC(void)
{
    PG_volt = 1;
    
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
    check_uv = ((dtclst[UVinDTC].DTCStatus & 0x08) != 0);
    check_ov = ((dtclst[OVinDTC].DTCStatus & 0x08) == 0);
    check_oc = ((dtclst[OCinDTC].DTCStatus & 0x08) != 0);
    if(!(check_uv && check_ov && check_oc)){
    	return TEST_FAIL;
    }

    //의도한대로 DTC code값이 저장되면 1, 그렇지 않으면 0
    check_uv = (dtclst[UVinDTC].DTC_Code[0] == 0x05) && (dtclst[UVinDTC].DTC_Code[1] == 0x62) && (dtclst[UVinDTC].DTC_Code[2] == 0x16);
    check_ov = (dtclst[OVinDTC].DTC_Code[0] == 0x05) && (dtclst[OVinDTC].DTC_Code[1] == 0x63) && (dtclst[OVinDTC].DTC_Code[2] == 0x17);
    check_oc = (dtclst[OCinDTC].DTC_Code[0] == 0x31) && (dtclst[OCinDTC].DTC_Code[1] == 0x01) && (dtclst[OCinDTC].DTC_Code[2] == 0x19);
    if(!(check_uv && check_ov && check_oc)){
    	return TEST_FAIL;
    }

    return TEST_PASS;
}

//PMIC 레지스터 버퍼, DTC 버퍼를 초기화하는 함수
static void BB_ResetFaults(void)
{
    voltage_reg_buff.raw = 0;
    current_reg_buff.raw = 0;
    for (int i = 0; i < MAX_DTC_NUM; i++) {
        dtclst[i].DTCStatus = 0x00;
    }
    bb_tx_seen = 0;
}

// 고장 정보에 대한 응답이 규격 프레임인지 확인하는 함수
TestResult BB_TC01_ReadDtcResponse(void)
{
    // 가상의 요청 데이터 프레임 버퍼 req
    CANData_t req = {0};
    // 기대하는 정답 프레임
    const uint8_t expect[] = {0x07, 0x59, 0x02, DTC_STATUS_AVAIL, 0x05, 0x63, 0x17, 0x08};

    // 고장 상태 초기화
    BB_ResetFaults();
    // 임의로 전압 고장 상태 설정
    voltage_reg_buff.raw = 0x10;
    // 전압 고장 상태를 DTC로 변환
    DTCProcessFault();

    // 임의로 UDS 요청 프레임 설정
    req.read_request.DLC_length = 0x03;
    req.read_request.service_id = 0x19;
    req.read_request.sub_function = 0x02;
    req.read_request.status_mask = 0xFF;
    // UDS 요청 프레임 처리
    Process_CAN_Response(req);

    // 응답 프레임을 정답 프레임과 비교
    if (memcmp(bb_tx_data, expect, sizeof(expect)) != 0) {
        return TEST_FAIL;
    }
    return TEST_PASS;
}

// 지원하지 않는 sub-function 요청에 대한 응답이 규격 프레임인지 확인하는 함수
TestResult BB_TC02_UnsupportedSubFunction(void)
{
    // 가상의 요청 데이터 프레임 버퍼 req
    CANData_t req = {0};
    // 기대하는 정답 프레임
    const uint8_t expect[] = {0x03, 0x7F, 0x19, 0x12, 0x00, 0x00, 0x00, 0x00};

    // 고장 상태 초기화
    BB_ResetFaults();
    // 임의로 지원하지 않는 sub-function 요청 프레임 설정
    req.read_request.service_id = 0x19;
    req.read_request.sub_function = 0xFF;
    req.read_request.status_mask = 0x01;
    // UDS 요청 프레임 처리
    Process_CAN_Response(req);

    // 응답 프레임을 정답 프레임과 비교
    if (memcmp(bb_tx_data, expect, sizeof(expect)) != 0) {
        return TEST_FAIL;
    }
    return TEST_PASS;
}
#endif