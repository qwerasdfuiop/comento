#ifndef EEPROM_H_
#define EEPROM_H_

// SPI EEPROM 명령어
#define EEPROM_CMD_WREN  0x06
#define EEPROM_CMD_WRITE 0x02
#define EEPROM_CMD_READ  0x03
#define EEPROM_DTC_ADDR  0x0000

extern volatile uint8_t is_spi_busy;

// EEPROM에 Write를 하기 위해서 WREN을 해야 함
void EEPROM_WriteEnable(void);

// fault가 있을때 EEPROM에 고장 정보 저장
void EEPROM_WriteDTC(void);

// 루프 시작 전 EEPROM에서 정보 읽기
void EEPROM_ReadDTC(void);

#endif
