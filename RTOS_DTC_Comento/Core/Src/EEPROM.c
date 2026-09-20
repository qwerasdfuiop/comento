#include "main.h"
#include "DTC.h"
#include "EEPROM.h"

volatile uint8_t is_spi_busy = 0;

//EEPROM에 Write를 하기 위해서는 WREN을 먼저 보내야함.
void EEPROM_WriteEnable(void) {
  //WREN 명령
  uint8_t cmd = EEPROM_CMD_WREN;
  // PMIC Slave 선택
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
  // 명령 전송
  HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
  // PMIC Slave 통신 종료
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
}

//EEPROM에 고장 정보를 WRITE
void EEPROM_WriteDTC(void) {
  //WRITE 명령과 WRITE할 메모리 주소
  uint8_t cmd[3];
  EEPROM_WriteEnable();
  cmd[0] = EEPROM_CMD_WRITE;
  cmd[1] = (EEPROM_DTC_ADDR >> 8) & 0xFF;
  cmd[2] = EEPROM_DTC_ADDR & 0xFF;
  // PMIC Slave 선택
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
  // 명령 전송
  is_spi_busy = 1;
  HAL_SPI_Transmit_DMA(&hspi1, cmd, 3);
  while(is_spi_busy);
  // DTC 정보 전송
  is_spi_busy = 1;
  HAL_SPI_Transmit_DMA(&hspi1, (uint8_t*)&dtclst, sizeof(dtclst));
  while(is_spi_busy);
  // PMIC Slave 통신 종료
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
}

//EEPROM으로부터 고장 정보를 READ
void EEPROM_ReadDTC(void) {
  //READ 명령과 READ할 메모리 주소
  uint8_t cmd[3];
  cmd[0] = EEPROM_CMD_READ;
  cmd[1] = (EEPROM_DTC_ADDR >> 8) & 0xFF;
  cmd[2] = EEPROM_DTC_ADDR & 0xFF;
  // PMIC Slave 선택
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
  // 명령 전송
  is_spi_busy = 1;
  HAL_SPI_Transmit_DMA(&hspi1, cmd, 3);
  while(is_spi_busy);
  // DTC 정보 읽기
  is_spi_busy = 1;
  HAL_SPI_Receive_DMA(&hspi1, (uint8_t*)&dtclst, sizeof(dtclst));
  while(is_spi_busy);
  // PMIC Slave 통신 종료
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
}
