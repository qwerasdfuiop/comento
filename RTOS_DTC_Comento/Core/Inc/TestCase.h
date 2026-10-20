#ifndef TESTCASE_H_
#define TESTCASE_H_

// 테스트 케이스 활성화 여부 정의
#define TEST_CASE 0

// 테스트 케이스 함수 실행 결과 정의
#if TEST_CASE
typedef enum {
    TEST_PASS = 1,
    TEST_FAIL = 0
} TestResult;

// 모든 테스트 케이스 실행 및 결과 출력
void TestCase_RunAll(void);
//각 테스트 케이스 함수 실행 결과 출력 함수
void Test_PrintResult(const char *name, TestResult result);
//PMIC 메모리 값을 임의로 설정해서 reg_buff값을 테스트하는 함수
TestResult WB_TC01_FaultBitDecode(void);
//reg_buff값으로부터 DTC로 변환이 잘 됐는지 확인하는 함수
TestResult WB_TC02_FaultToDTC(void);
// 고장 정보에 대한 응답이 규격 프레임인지 확인하는 함수
TestResult BB_TC01_ReadDtcResponse(void);
// 지원하지 않는 sub-function 요청에 대한 응답이 규격 프레임인지 확인하는 함수
TestResult BB_TC02_UnsupportedSubFunction(void);

#endif
#endif
