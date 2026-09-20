/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <string.h>
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
// SPI EEPROM 명령어
#define EEPROM_CMD_WREN  0x06
#define EEPROM_CMD_WRITE 0x02
#define EEPROM_CMD_READ  0x03
#define EEPROM_DTC_ADDR  0x0000

// PMIC 주소
#define PMIC_I2C_ADDR  (0x60 << 1)
#define PMIC_V_REFA_HIGH  0x13
#define PMIC_V_REFA_LOW  0x14
#define V_REF_SET 0xFF // 임의의 값 설정

//DTC 고장 코드 종류 개수
#define MAX_DTC_NUM 3

//#define CAN_Q_SIZE 8
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

CAN_HandleTypeDef hcan1;

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;
DMA_HandleTypeDef hdma_i2c1_rx;
DMA_HandleTypeDef hdma_i2c1_tx;
DMA_HandleTypeDef hdma_i2c2_rx;
DMA_HandleTypeDef hdma_i2c2_tx;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;
DMA_HandleTypeDef hdma_spi1_rx;
DMA_HandleTypeDef hdma_spi1_tx;
DMA_HandleTypeDef hdma_spi2_rx;
DMA_HandleTypeDef hdma_spi2_tx;

UART_HandleTypeDef huart4;

/* USER CODE BEGIN PV */

// fault 레지스터들의 메모리 주소를 열거형으로
enum PMICRegisters_t {
	VOLTAGE_FAULT_ADDR = 0x07,
	CURRENT_FAULT_ADDR = 0x08,
	TEMPERATURE_FAULT_ADDR = 0x09,
};

// UV, OV를 확인할 수 있는 레지스터 버퍼
typedef union {
  uint8_t raw;          /* HAL_CAN_GetRxMessage / AddTxMessage용 */
  struct {
    uint8_t buckx_uv	:4;
    uint8_t buckx_ov	:4;
  } bit;
} voltage_fault_reg;

voltage_fault_reg voltage_reg_buff;

// OC를 확인할 수 있는 레지스터 버퍼
typedef union {
  uint8_t raw;          /* HAL_CAN_GetRxMessage / AddTxMessage용 */
  struct {
    uint8_t buckx_oc;
  } bit;
} current_fault_reg;

current_fault_reg current_reg_buff;

// HIGH_TEMP를 확인할 수 있는 레지스터 버퍼
//typedef union {
//  uint8_t raw;          /* HAL_CAN_GetRxMessage / AddTxMessage용 */
//  struct {
//    uint8_t reserved	:6;
//    uint8_t high_temp	:2;
//  } bit;
//} temperature_fault_reg;

//temperature_fault_reg temperature_reg_buff;

// OBD, UDS 형태의 데이터 프레임 버퍼
typedef union {
  uint8_t raw[8];          /* HAL_CAN_GetRxMessage / AddTxMessage용 */
  struct {
    uint8_t pci;
    uint8_t sid;
    uint8_t reserv2;
    uint8_t reserv3;
    uint8_t reserv4;
    uint8_t reserv5;
    uint8_t reserv6;
    uint8_t reserv7;
  } field;
} CANData_t;

CANData_t data;

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

// 위 구조체들의 배열
DTC_t dtclst[MAX_DTC_NUM] = {
		{UV, 0},
		{OV, 0},
		{OC, 0},
		//{HIGH_TEMP, 0},
};

//각 고장 종류가 배열의 어느 인덱스에 있는지 나타내는 열거형
enum DTC_Index_t {
	UVinDTC = 0,
	OVinDTC = 1,
	OCinDTC = 2,
	//HIGH_TEMPinDTC = 3
};


// 인터럽트 완료 확인을 위한 플래그 변수
volatile uint8_t is_i2c_busy = 0;
volatile uint8_t is_spi_busy = 0;
volatile uint8_t can_rx_flag = 0;

//volatile uint8_t can_head = 0, can_tail = 0;

// 임의로 V_REF 단계 설정 가능.
uint16_t v_ref_set = V_REF_SET;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_CAN1_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C2_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI2_Init(void);
static void MX_UART4_Init(void);

// PMIC의 VREF 값 변경
void PMIC_Vref_Change(uint16_t v_ref_set);

// PMIC로부터 i2c로 fault여부 읽는 함수
void PMIC_Read_Fault(void);

// EEPROM에 Write를 하기 위해서 WREN을 해야 함
void EEPROM_WriteEnable(void);

// fault가 있을때 EEPROM에 고장 정보 저장
void EEPROM_WriteDTC(void);

// 루프 시작 전 EEPROM에서 정보 읽기
void EEPROM_ReadDTC(void);

// CAN 인터럽트 요청에 대한 응답
void Process_CAN_Response(CANData_t data);


/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	const char msg1[] = "ECU System Running\r\n";
	const char msg2[] = "ECU System Went Wrong\r\n";
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_CAN1_Init();
  MX_I2C1_Init();
  MX_I2C2_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_UART4_Init();
  /* USER CODE BEGIN 2 */
  HAL_CAN_Start(&hcan1);
  HAL_CAN_ActivateNotification(&hcan1, CAN_IT_RX_FIFO0_MSG_PENDING);

  EEPROM_ReadDTC();

  //VREF 값 설정
  PMIC_Vref_Change(v_ref_set);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

	//I2C DMA INTERRUPT 방식으로 UV 혹은 OV 여부 READ
	PMIC_Read_Fault();
    //만일 UV 혹은 OV 가 있다면 EEPROM에 DTC를 WRITE
	if(voltage_reg_buff.raw || current_reg_buff.raw){
		if (voltage_reg_buff.bit.buckx_uv) {
		  if (dtclst[UVinDTC].active == 0) {
			  dtclst[UVinDTC].active = 1;
		  }
		}
		if (voltage_reg_buff.bit.buckx_ov) {
		  if (dtclst[OVinDTC].active == 0) {
			  dtclst[OVinDTC].active = 1;
		  }
		}
		if (current_reg_buff.bit.buckx_oc) {
		  if (dtclst[OCinDTC].active == 0) {
			  dtclst[OCinDTC].active = 1;
		  }
		}
//		if (temperature_reg_buff.bit.high_temp) {
//		  if (dtclst[HIGH_TEMPinDTC].active == 0) {
//			  dtclst[HIGH_TEMPinDTC].active = 1;
//		  }
//		}
		EEPROM_WriteDTC();
	}

    // CAN INTERRUPT 방식으로 UDS / OBD 메시지 송수신
    if (can_rx_flag) {
	  can_rx_flag = 0;
	  Process_CAN_Response(data);
    }
    // uart 폴링 방식으로 터미널로 전송
    for(int i = 0; i < MAX_DTC_NUM; i++){
    	if(dtclst[i].active == 1){
    		HAL_UART_Transmit(&huart4, (uint8_t*)msg2, strlen(msg2), HAL_MAX_DELAY);
    	}
    }
    HAL_UART_Transmit(&huart4, (uint8_t*)msg1, strlen(msg1), HAL_MAX_DELAY);



    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief CAN1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_CAN1_Init(void)
{

  /* USER CODE BEGIN CAN1_Init 0 */

  /* USER CODE END CAN1_Init 0 */

  /* USER CODE BEGIN CAN1_Init 1 */

  /* USER CODE END CAN1_Init 1 */
  hcan1.Instance = CAN1;
  hcan1.Init.Prescaler = 16;
  hcan1.Init.Mode = CAN_MODE_NORMAL;
  hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
  hcan1.Init.TimeSeg1 = CAN_BS1_1TQ;
  hcan1.Init.TimeSeg2 = CAN_BS2_1TQ;
  hcan1.Init.TimeTriggeredMode = DISABLE;
  hcan1.Init.AutoBusOff = DISABLE;
  hcan1.Init.AutoWakeUp = DISABLE;
  hcan1.Init.AutoRetransmission = DISABLE;
  hcan1.Init.ReceiveFifoLocked = DISABLE;
  hcan1.Init.TransmitFifoPriority = DISABLE;
  if (HAL_CAN_Init(&hcan1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN CAN1_Init 2 */

  // 필터에 해당하는 메시지만 수신 하도록 초기화
  CAN_FilterTypeDef f = {0};
  f.FilterBank = 0;
  f.FilterMode = CAN_FILTERMODE_IDMASK;
  f.FilterScale = CAN_FILTERSCALE_32BIT;
  f.FilterIdHigh = 0x0000;
  f.FilterIdLow  = 0x0000;
  f.FilterMaskIdHigh = 0x0000;  /* 마스크 0 = 전부 통과 */
  f.FilterMaskIdLow  = 0x0000;
  f.FilterFIFOAssignment = CAN_RX_FIFO0;
  f.FilterActivation = ENABLE;
  f.SlaveStartFilterBank = 14;
  HAL_CAN_ConfigFilter(&hcan1, &f);
  /* USER CODE END CAN1_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 100000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

}

/**
  * @brief UART4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_UART4_Init(void)
{

  /* USER CODE BEGIN UART4_Init 0 */

  /* USER CODE END UART4_Init 0 */

  /* USER CODE BEGIN UART4_Init 1 */

  /* USER CODE END UART4_Init 1 */
  huart4.Instance = UART4;
  huart4.Init.BaudRate = 115200;
  huart4.Init.WordLength = UART_WORDLENGTH_8B;
  huart4.Init.StopBits = UART_STOPBITS_1;
  huart4.Init.Parity = UART_PARITY_NONE;
  huart4.Init.Mode = UART_MODE_TX_RX;
  huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart4.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart4) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN UART4_Init 2 */

  /* USER CODE END UART4_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
  /* DMA1_Stream2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream2_IRQn);
  /* DMA1_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
  /* DMA1_Stream4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
  /* DMA1_Stream6_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
  /* DMA1_Stream7_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream7_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream7_IRQn);
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
  /* DMA2_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2, GPIO_PIN_SET);

  /*Configure GPIO pins : PB0 PB1 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB2 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void PMIC_Read_Fault(void){

	is_i2c_busy = 1;
	HAL_I2C_Mem_Read_DMA(&hi2c1, PMIC_I2C_ADDR, VOLTAGE_FAULT_ADDR, I2C_MEMADD_SIZE_8BIT, &voltage_reg_buff.raw, 1);
	while(is_i2c_busy);

	is_i2c_busy = 1;
	HAL_I2C_Mem_Read_DMA(&hi2c1, PMIC_I2C_ADDR, CURRENT_FAULT_ADDR, I2C_MEMADD_SIZE_8BIT, &current_reg_buff.raw, 1);
	while(is_i2c_busy);

//	is_i2c_busy = 1;
//	HAL_I2C_Mem_Read_DMA(&hi2c1, PMIC_I2C_ADDR, TEMPERATURE_FAULT_ADDR, I2C_MEMADD_SIZE_8BIT, &temperature_reg_buff.raw, 1);
//	while(is_i2c_busy);
}

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

void Process_CAN_Response(CANData_t data) {

  CAN_TxHeaderTypeDef TxHeader;
  uint32_t TxMailbox;
  uint8_t TxData[8] = {0};

  TxHeader.StdId = 0x7E8; // 응답 ID
  TxHeader.IDE = CAN_ID_STD;
  TxHeader.RTR = CAN_RTR_DATA;
  TxHeader.DLC = 8;

  int index = 2;

  // OBD2 0x43: Read DTCs
  if (data.field.sid == 0x03) {

	  TxData[1] = 0x43;
	  if(dtclst[UVinDTC].active == 1){
		  TxData[index] = (dtclst[UVinDTC].DTC_Code >> 8) & 0xFF;
		  index++;
		  TxData[index] = dtclst[UVinDTC].DTC_Code & 0xFF;
		  index++;
	  }
	  if(dtclst[OVinDTC].active == 1){
		  TxData[index] = (dtclst[OVinDTC].DTC_Code >> 8) & 0xFF;
		  index++;
		  TxData[index] = dtclst[OVinDTC].DTC_Code & 0xFF;
		  index++;
	  }
	  if(dtclst[OCinDTC].active == 1){
		  TxData[index] = (dtclst[OCinDTC].DTC_Code >> 8) & 0xFF;
		  index++;
		  TxData[index] = dtclst[OCinDTC].DTC_Code & 0xFF;
		  index++;
	  }
  }
  // OBD2 0x04: Clear DTCs
  else if (data.field.sid == 0x04) {
	for(int i = 0; i < MAX_DTC_NUM; i++){
		dtclst[i].active = 0;
	}
	EEPROM_WriteDTC();
    TxData[1] = 0x44;// 응답
  }
  // UDS 0x19: Read DTCs
  else if (data.field.sid == 0x19) {

	  TxData[1] = 0x59;
	  if(dtclst[UVinDTC].active == 1){
		  TxData[index] = (dtclst[UVinDTC].DTC_Code >> 8) & 0xFF;
		  index++;
		  TxData[index] = dtclst[UVinDTC].DTC_Code & 0xFF;
		  index++;
	  }
	  if(dtclst[OVinDTC].active == 1){
		  TxData[index] = (dtclst[OVinDTC].DTC_Code >> 8) & 0xFF;
		  index++;
		  TxData[index] = dtclst[OVinDTC].DTC_Code & 0xFF;
		  index++;
	  }
	  if(dtclst[OCinDTC].active == 1){
		  TxData[index] = (dtclst[OCinDTC].DTC_Code >> 8) & 0xFF;
		  index++;
		  TxData[index] = dtclst[OCinDTC].DTC_Code & 0xFF;
		  index++;
	  }
  }
  // UDS 0x14: Clear DTCs
  else if (data.field.sid == 0x14) {
	for(int i = 0; i < MAX_DTC_NUM; i++){
		dtclst[i].active = 0;
	}
	EEPROM_WriteDTC();
    TxData[1] = 0x54; // 응답
  }
  else
  {
	/* for misra code*/
  }

  HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox);
}

void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  if (hi2c->Instance == I2C1) {
    is_i2c_busy = 0;
  }
}
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
  if (hspi->Instance == SPI1) {
    is_spi_busy = 0;
  }
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
  CAN_RxHeaderTypeDef hdr;

  if(HAL_CAN_GetRxMessage(hcan, CAN_RX_FIFO0, &hdr, data.raw) == HAL_OK){
    can_rx_flag = 1;
  }
}

void PMIC_Vref_Change(uint16_t v_ref_set){

	uint8_t v_ref_temp = 0;

    HAL_I2C_Mem_Read(&hi2c1, PMIC_I2C_ADDR, PMIC_V_REFA_HIGH, I2C_MEMADD_SIZE_8BIT, &v_ref_temp, 1, HAL_MAX_DELAY);
    v_ref_temp = (v_ref_temp & 0xFC) | (uint8_t)(v_ref_set >> 8);
	HAL_I2C_Mem_Write(&hi2c1, PMIC_I2C_ADDR, PMIC_V_REFA_HIGH, I2C_MEMADD_SIZE_8BIT, &v_ref_temp, 1, HAL_MAX_DELAY);

	v_ref_temp = v_ref_set & 0xFF;

	HAL_I2C_Mem_Write(&hi2c1, PMIC_I2C_ADDR, PMIC_V_REFA_LOW, I2C_MEMADD_SIZE_8BIT, &v_ref_temp, 1, HAL_MAX_DELAY);

}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
