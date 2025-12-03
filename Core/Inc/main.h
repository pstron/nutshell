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
#include "stm32f1xx_hal.h"

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
#define huart_DBG huart1
#define huart_USER huart3
#define htim_RC htim6
#define Enable_IO_Pin GPIO_PIN_13
#define Enable_IO_GPIO_Port GPIOC
#define PPM_Pin GPIO_PIN_3
#define PPM_GPIO_Port GPIOC
#define PPM_EXTI_IRQn EXTI3_IRQn
#define ENC_CH1_A_Pin GPIO_PIN_0
#define ENC_CH1_A_GPIO_Port GPIOA
#define ENC_CH1_B_Pin GPIO_PIN_1
#define ENC_CH1_B_GPIO_Port GPIOA
#define Music_Pin GPIO_PIN_2
#define Music_GPIO_Port GPIOA
#define ENC_CH2_A_Pin GPIO_PIN_6
#define ENC_CH2_A_GPIO_Port GPIOA
#define ENC_CH2_B_Pin GPIO_PIN_7
#define ENC_CH2_B_GPIO_Port GPIOA
#define Steer_B0_Pin GPIO_PIN_0
#define Steer_B0_GPIO_Port GPIOB
#define Buzz_Pin GPIO_PIN_1
#define Buzz_GPIO_Port GPIOB
#define Stop_Pin GPIO_PIN_10
#define Stop_GPIO_Port GPIOB
#define PlayLight_Pin GPIO_PIN_11
#define PlayLight_GPIO_Port GPIOB
#define IR_Pin GPIO_PIN_12
#define IR_GPIO_Port GPIOB
#define Switch_4_Pin GPIO_PIN_7
#define Switch_4_GPIO_Port GPIOC
#define Switch_2_Pin GPIO_PIN_8
#define Switch_2_GPIO_Port GPIOC
#define Switch_3_Pin GPIO_PIN_9
#define Switch_3_GPIO_Port GPIOC
#define Switch_1_Pin GPIO_PIN_8
#define Switch_1_GPIO_Port GPIOA
#define LED_1_Pin GPIO_PIN_11
#define LED_1_GPIO_Port GPIOA
#define Switch_EN_Pin GPIO_PIN_12
#define Switch_EN_GPIO_Port GPIOA
#define LED_2_Pin GPIO_PIN_15
#define LED_2_GPIO_Port GPIOA
#define LED_3_Pin GPIO_PIN_12
#define LED_3_GPIO_Port GPIOC
#define LED_4_Pin GPIO_PIN_3
#define LED_4_GPIO_Port GPIOB
#define Wheel_Right_PWM_Pin GPIO_PIN_6
#define Wheel_Right_PWM_GPIO_Port GPIOB
#define Wheel_Left_PWM_Pin GPIO_PIN_7
#define Wheel_Left_PWM_GPIO_Port GPIOB
#define Wheel_Left_IO_Pin GPIO_PIN_8
#define Wheel_Left_IO_GPIO_Port GPIOB
#define Wheel_Right_IO_Pin GPIO_PIN_9
#define Wheel_Right_IO_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

// TIM1_CH1N--->B13
// TIM1_CH2N--->B14
// TIM1_CH3N--->B15
#define PWM_TIM1_CH1N_B13       ((uint32_t *)&TIM1->CCR1)
#define PWM_TIM1_CH2N_B14       ((uint32_t *)&TIM1->CCR2)
#define PWM_TIM1_CH3N_B15       ((uint32_t *)&TIM1->CCR3)
#define PWM_TIM1_CH4_A11        ((uint32_t *)&TIM1->CCR4)

// TIM2_CH1---->A0
// TIM2_CH2---->A1
#define PWM_TIM2_CH1_A0         ((uint32_t *)&TIM2->CCR1)
#define PWM_TIM2_CH2_A1         ((uint32_t *)&TIM2->CCR2)

// TIM3_CH1---->A6
// TIM3_CH2---->A7
// TIM3_CH3---->B0
#define PWM_TIM3_CH1_A6         ((uint32_t *)&TIM3->CCR1)
#define PWM_TIM3_CH2_A7         ((uint32_t *)&TIM3->CCR2)
#define PWM_TIM3_CH3_B0         ((uint32_t *)&TIM3->CCR3)

// TIM4_CH1---->B6
// TIM4_CH2---->B7
// TIM4_CH3---->B8
// TIM4_CH4---->B9
#define PWM_TIM4_CH1_B6         ((uint32_t *)&TIM4->CCR1)
#define PWM_TIM4_CH2_B7         ((uint32_t *)&TIM4->CCR2)
#define PWM_TIM4_CH3_B8         ((uint32_t *)&TIM4->CCR3)
#define PWM_TIM4_CH4_B9         ((uint32_t *)&TIM4->CCR4)

// TIM5_CH3---->A2
// TIM5_CH4---->A3
#define PWM_TIM5_CH3_A2         ((uint32_t *)&TIM5->CCR3)
#define PWM_TIM5_CH4_A3         ((uint32_t *)&TIM5->CCR4)

// TIM8_CH1---->C6
#define PWM_TIM8_CH1_C6         ((uint32_t *)&TIM8->CCR1)

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
