#ifndef TESTCASE_H_
#define TESTCASE_H_

#define TEST_CASE 1

#ifdef TEST_CASE
typedef enum {
    TEST_PASS = 1,
    TEST_FAIL = 0
} TestResult;

void TestCase_RunAll(void);

void Test_PrintResult(const char *name, TestResult result);

TestResult WB_TC01_FaultBitDecode(void);

TestResult WB_TC02_FaultToDTC(void);

#endif
#endif
