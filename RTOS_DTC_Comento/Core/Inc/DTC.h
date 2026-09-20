#ifndef DTC_H_
#define DTC_H_

//DTC 고장 코드 종류 개수
#define MAX_DTC_NUM 3

// DTC Code 종류
enum DTC_Code_t {
	UV = 0x1234,
	OV = 0x3456,
	OC = 0x5678,
	//HIGH_TEMP = 0x4567
};

#pragma pack(push, 1)

// DTC Code와 활성화 여부를 같이 저장하기 위한 구조체
typedef struct {
  uint16_t DTC_Code;              // 고장 코드 (예: C1234)
//  char Description[50];           // 설명 문자열
  uint8_t active;                 // 활성화 상태 플래그
} DTC_t;

#pragma pack(pop)

//각 고장 종류가 배열의 어느 인덱스에 있는지 나타내는 열거형
enum DTC_Index_t {
	UVinDTC = 0,
	OVinDTC = 1,
	OCinDTC = 2,
	//HIGH_TEMPinDTC = 3
};

extern DTC_t dtclst[MAX_DTC_NUM];

#endif
