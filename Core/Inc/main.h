/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN Private defines */
#define BTN_VAL_INC 	3
#define BTN_VAL_DEC 	5
#define BTN_VAL_ABRT	1
#define BTN_VAL_SP 		6

typedef enum {
	NOT_PRESSED, BOUNCE, PRESSED
} button_state_t;

typedef enum {
	EVENT_HANDLED, EVENT_IGNORED, EVENT_TRANSITION
} event_status_t;

typedef enum {
	INC_TIME, DEC_TIME, TIME_TICK, START_PAUSE, ABRT,
//	Internal activity signal
	ENTRY,
	EXIT
} protime_signal_t;

struct protimer_tag;
struct event_tag;

typedef event_status_t(*protimer_state_t)(struct protimer_tag *const,
		struct event_tag const *const);
//for user generated event
typedef struct protimer_tag{
	uint32_t curr_time;
	uint32_t elapsed_time;
	uint32_t pro_time;
	protimer_state_t active_state;
} protimer_t;

void protimer_init(protimer_t *mobj);

// Base event
typedef struct event_tag{
	uint8_t sig;
} event_t;

// User event - inherits from event_t
typedef struct {
	event_t super;  // ← MUST be the full struct, not uint8_t!
} protimer_user_event_t;

// Tick event
typedef struct {
	event_t super;
	uint8_t ss;
} protimer_tick_event_t;


void protimer_init(protimer_t *mobj);
event_status_t protimer_state_machine(protimer_t *const mobj,
		event_t const *const e);
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
