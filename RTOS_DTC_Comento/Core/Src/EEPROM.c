#include "main.h"
#include "DTC.h"
#include "EEPROM.h"

volatile uint8_t is_spi_busy = 0;

void EEPROM_WriteEnable(void) {
  uint8_t cmd = EEPROM_CMD_WREN;
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
  HAL_SPI_Transmit(&hspi1, &cmd, 1, HAL_MAX_DELAY);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
}

void EEPROM_WriteDTC(void) {

  uint8_t cmd[3];
  EEPROM_WriteEnable();
  cmd[0] = EEPROM_CMD_WRITE;
  cmd[1] = (EEPROM_DTC_ADDR >> 8) & 0xFF;
  cmd[2] = EEPROM_DTC_ADDR & 0xFF;
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
  is_spi_busy = 1;
  HAL_SPI_Transmit_DMA(&hspi1, cmd, 3);
  while(is_spi_busy);
  is_spi_busy = 1;
  HAL_SPI_Transmit_DMA(&hspi1, (uint8_t*)&dtclst, sizeof(dtclst));
  while(is_spi_busy);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
}

void EEPROM_ReadDTC(void) {
  uint8_t cmd[3];
  cmd[0] = EEPROM_CMD_READ;
  cmd[1] = (EEPROM_DTC_ADDR >> 8) & 0xFF;
  cmd[2] = EEPROM_DTC_ADDR & 0xFF;
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
  is_spi_busy = 1;
  HAL_SPI_Transmit_DMA(&hspi1, cmd, 3);
  while(is_spi_busy);
  is_spi_busy = 1;
  HAL_SPI_Receive_DMA(&hspi1, (uint8_t*)&dtclst, sizeof(dtclst));
  while(is_spi_busy);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
}
