/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "SSD1306.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
static void protimer_event_dispatcher(protimer_t *const mobj,
		event_t const *const e);
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
typedef struct {
	uint8_t stable;       // 0 or 1 (last debounced level)
	uint8_t counter;      // integrator
} Debounce;

static Debounce dbA6 = { 1, 0 }; // pull-up => idle=1
static Debounce dbA7 = { 1, 0 };
static Debounce dbB0 = { 1, 0 };

static uint8_t debounce_step(Debounce *db, uint8_t raw) {
	// 3-bit integrator: counts toward raw level; flips when fully counted
	const uint8_t MAXC = 5;  // ~5 samples to confirm (~5*1ms if we call @1ms)
	if (raw == db->stable) {
		if (db->counter)
			db->counter--;
	} else {
		if (db->counter < MAXC)
			db->counter++;
		if (db->counter >= MAXC) {
			db->stable = raw;
			db->counter = 0;
		}
	}
	return db->stable;
}

// Packs debounced bits like your original (b1<<2)|(b2<<1)|b3
static uint8_t read_buttons_packed(void) {
	// raw reads
	uint8_t rA6 = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6);
	uint8_t rA7 = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_7);
	uint8_t rB0 = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);

	// debounce once per call
	uint8_t b1 = debounce_step(&dbA6, rA6);
	uint8_t b2 = debounce_step(&dbA7, rA7);
	uint8_t b3 = debounce_step(&dbB0, rB0);

	// pack like before
	return (uint8_t) ((b1 << 2) | (b2 << 1) | b3);
}

// Helper: convert active-LOW packed value into your event code
// Returns 0 when no (new) press detected.
static uint8_t decode_btn_event(uint8_t packed_now) {
	// We trigger only on a *new* press (transition 1->0 on any line).
	// Keep previous pack:
	static uint8_t prev = 0x07; // all idle (1,1,1)
	uint8_t event = 0;

	// Look for any change
	if (packed_now != prev) {
		// Single-button presses (active-LOW => the bit that went 0)
		if (packed_now == 3)
			event = BTN_VAL_INC;  // 0b011
		else if (packed_now == 5)
			event = BTN_VAL_DEC; // 0b101
		else if (packed_now == 1)
			event = BTN_VAL_ABRT; // 0b110
		else if (packed_now == 6)
			event = BTN_VAL_SP;   // 0b001 (combo)
		// else: ignore (multiple noise or release)
		prev = packed_now;
	}
	return event;
}
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */

//Main application object
static protimer_t protimer;
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void protimer_event_dispatcher(protimer_t *const mobj,
		event_t const *const e) {
	event_status_t status;
	protimer_state_t source, target;

	source = mobj->active_state;
	status = (*mobj->active_state)(mobj, e);

	if (status == EVENT_TRANSITION) {
		target = mobj->active_state;

		// 1) tell the *source* state to EXIT
		event_t ee;
		ee.sig = EXIT;
		(*source)(mobj, &ee);

		// 2) now switch to *target* and send ENTRY
		ee.sig = ENTRY;
		(*target)(mobj, &ee);
	}
}

uint8_t process_btn_val(uint8_t btn_st_val) {
	static button_state_t btn_sm_state = NOT_PRESSED;
	static uint32_t curr_time = HAL_GetTick();

	switch (btn_sm_state) {
	case NOT_PRESSED: {
		if (btn_st_val) {
			btn_sm_state = BOUNCE;
			curr_time = HAL_GetTick();
		}
		break;
	}
	case BOUNCE: {
		if (HAL_GetTick() - curr_time >= 50) {
			//50ms has passed
			if (btn_st_val) {
				btn_sm_state = PRESSED;
				return btn_st_val;
			} else
				btn_sm_state = NOT_PRESSED;
		}
		break;
	}
	case PRESSED: {
		if (!btn_st_val) {
			btn_sm_state = BOUNCE;
			curr_time = HAL_GetTick();
		}
		break;
	}

	}
	return 0;
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

	/* USER CODE BEGIN 1 */

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
	MX_I2C1_Init();
	/* USER CODE BEGIN 2 */
	SSD1306_Init();
	protimer_init(&protimer);
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	protimer_tick_event_t te = { .super = { .sig = TIME_TICK }, .ss = 0 };
	uint32_t last_tick = HAL_GetTick();
	uint32_t last_scan = HAL_GetTick();
	while (1) {
		// 1) scan buttons at ~1 kHz for snappy debounce
		if (HAL_GetTick() - last_scan >= 1) {
			last_scan = HAL_GetTick();
			uint8_t packed = read_buttons_packed();
			uint8_t code = decode_btn_event(packed);

			if (code) {
				protimer_user_event_t ue;
				if (code == BTN_VAL_INC)
					ue.super.sig = INC_TIME;
				else if (code == BTN_VAL_DEC)
					ue.super.sig = DEC_TIME;
				else if (code == BTN_VAL_ABRT)
					ue.super.sig = ABRT;
				else if (code == BTN_VAL_SP)
					ue.super.sig = START_PAUSE;
				protimer_event_dispatcher(&protimer, &ue.super);
			}
		}

		// 2) dispatch TIME_TICK every 100 ms (independent of buttons)
		if (HAL_GetTick() - last_tick >= 100) {
			last_tick += 100;
			te.super.sig = TIME_TICK;
			te.ss = (te.ss % 10) + 1;   // 1..10
			protimer_event_dispatcher(&protimer, &te.super);
		}
//			/* USER CODE END WHILE */

//			/* USER CODE BEGIN 3 */

		/* USER CODE END 3 */
	}
}
/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

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
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C1_Init(void) {

	/* USER CODE BEGIN I2C1_Init 0 */

	/* USER CODE END I2C1_Init 0 */

	/* USER CODE BEGIN I2C1_Init 1 */

	/* USER CODE END I2C1_Init 1 */
	hi2c1.Instance = I2C1;
	hi2c1.Init.ClockSpeed = 400000;
	hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
	hi2c1.Init.OwnAddress1 = 0;
	hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
	hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
	hi2c1.Init.OwnAddress2 = 0;
	hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
	hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
	if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN I2C1_Init 2 */

	/* USER CODE END I2C1_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };
	/* USER CODE BEGIN MX_GPIO_Init_1 */

	/* USER CODE END MX_GPIO_Init_1 */

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);

	/*Configure GPIO pins : PA6 PA7 */
	GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	/*Configure GPIO pin : PB0 */
	GPIO_InitStruct.Pin = GPIO_PIN_0;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pin : PB1 */
	GPIO_InitStruct.Pin = GPIO_PIN_1;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/* USER CODE BEGIN MX_GPIO_Init_2 */

	/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
	/* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
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
