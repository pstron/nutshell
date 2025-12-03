// Device.cpp
// A simple C++ enscapsulation of the device functions
// Date: Oct 2025
#include <cmath>
#include <cstdint>
#include <climits>
#include "stm32f1xx_hal.h"
#include "Device.h"
#include "Buffer.h"
#include "main.h"
#include "tim.h"
#include "adc.h"
#include "usart.h"

Buffer<uint16_t, BufferSize> BufferA4;
Buffer<uint16_t, BufferSize> BufferC5;
volatile uint16_t ADC_RawValue[ADCChannelCount];

// Functional

Device::Device() {
  initADC();
  initPWM();
}

void Device::delay(uint32_t ms) {
  HAL_Delay(ms);
}

uint32_t Device::getTick() {
  return HAL_GetTick();
}

void Device::setLightMode(LightMode mode) {
  this->devLightMode = mode;
}

Device::LightMode Device::getLightMode() {
  return this->devLightMode;
}

void Device::light(Device::LightMode mode, uint8_t x) {
  if (mode == this->devLightMode) {
    HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, x & 0b1 ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, (x & 0b10) >> 1 ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_3_GPIO_Port, LED_3_Pin, (x & 0b100) >> 2 ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LED_4_GPIO_Port, LED_4_Pin, (x & 0b1000) >> 3 ? GPIO_PIN_SET : GPIO_PIN_RESET);
  }
}

void Device::forceLight(uint8_t id, bool light) {
  switch (id) {
    case 1:
      HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, light ? GPIO_PIN_SET : GPIO_PIN_RESET);
      break;
    case 2:
      HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, light ? GPIO_PIN_SET : GPIO_PIN_RESET);
      break;
    case 3:
      HAL_GPIO_WritePin(LED_3_GPIO_Port, LED_3_Pin, light ? GPIO_PIN_SET : GPIO_PIN_RESET);
      break;
    case 4:
      HAL_GPIO_WritePin(LED_4_GPIO_Port, LED_4_Pin, light ? GPIO_PIN_SET : GPIO_PIN_RESET);
      break;
  }
}

void Device::forceLight(uint8_t x) {
  HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, x & 0b1 ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, (x & 0b10) >> 1 ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_3_GPIO_Port, LED_3_Pin, (x & 0b100) >> 2 ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_4_GPIO_Port, LED_4_Pin, (x & 0b1000) >> 3 ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void Device::buzz(bool enabled) {
  HAL_GPIO_WritePin(Buzz_GPIO_Port, Buzz_Pin, enabled ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

// void Device::IOLight(uint8_t x) {
//   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, static_cast<GPIO_PinState>(x & 0b1));
//   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, static_cast<GPIO_PinState>((x & 0b10) >> 1));
//   HAL_GPIO_WritePin(GPIOB, GPIO_PIN_11, static_cast<GPIO_PinState>((x & 0b100) >> 2));
//   *PWM_TIM1_CH1N_B13 = (x & 0b1000) >> 3 ? 8000 : 0;
//   *PWM_TIM1_CH2N_B14 = (x & 0b10000) >> 4 ? 8000 : 0;
//   *PWM_TIM1_CH3N_B15 = (x & 0b100000) >> 5 ? 8000 : 0;
//   *PWM_TIM8_CH1_C6 = (x & 0b1000000) >> 6 ? 8000 : 0;
// }
//
// void Device::IOLightPWM(uint8_t id, uint32_t duty) {
//   switch (id) {
//     case 0:
//       *PWM_TIM1_CH1N_B13 = duty;
//       break;
//     case 1:
//       *PWM_TIM1_CH2N_B14 = duty;
//       break;
//     case 2:
//       *PWM_TIM1_CH3N_B15 = duty;
//       break;
//     case 3:
//       *PWM_TIM8_CH1_C6 = duty;
//       break;
//     default:
//       break;
//   }
// }

void Device::playNote(Melody::Note note) {
  TIM5->ARR = static_cast<uint32_t>(note);
  TIM5->CCR3 = TIM5->ARR / 2;
  TIM5->CNT = 0;
  TIM5->EGR = TIM_EGR_UG;
}

void Device::playLight(bool set) {
  HAL_GPIO_WritePin(PlayLight_GPIO_Port, PlayLight_Pin, static_cast<GPIO_PinState>(set));
}

uint8_t Device::switchStatus() {
  uint8_t status = 0;
  status |= HAL_GPIO_ReadPin(Switch_1_GPIO_Port, Switch_1_Pin);
  status |= HAL_GPIO_ReadPin(Switch_2_GPIO_Port, Switch_2_Pin) << 1;
  status |= HAL_GPIO_ReadPin(Switch_3_GPIO_Port, Switch_3_Pin) << 2;
  status |= HAL_GPIO_ReadPin(Switch_4_GPIO_Port, Switch_4_Pin) << 3;
  status = ~status & 0x0F; 
  return status;
}

bool Device::switchOn(SwitchID id) {
  uint8_t n = static_cast<uint8_t>(id);
  return (n < 4) ? (this->switchStatus() >> n) & 0x01 : false;
}

bool Device::isEnabled() {
  return HAL_GPIO_ReadPin(Switch_EN_GPIO_Port, Switch_EN_Pin);
}

bool Device::getStopSignal() {
  return HAL_GPIO_ReadPin(Stop_GPIO_Port, Stop_Pin);
}

bool Device::getIRSignal() {
  return !HAL_GPIO_ReadPin(IR_GPIO_Port, IR_Pin);
}

uint16_t Device::getNoseADC(NoseID id, bool enableFiltering) {
  switch (id) {
    case NoseID::L:
      return enableFiltering ? getFiltered(BufferC5) : ADC_RawValue[1];
    case NoseID::R:
      return enableFiltering ? getFiltered(BufferA4) : ADC_RawValue[0];
    default:
      return 0;
  }
}

uint16_t Device::getFiltered(Buffer<uint16_t, BufferSize> buffer) {
  const size_t count = buffer.size();
  if (count == 0) return 0;
  
  uint16_t minVals[4] = {UINT16_MAX, UINT16_MAX, UINT16_MAX, UINT16_MAX};
  uint16_t maxVals[4] = {0, 0, 0, 0};
  
  for (size_t i = 0; i < count; i++) {
    uint16_t val = buffer[i];
    for (int j = 0; j < 4; j++) {
      if (val < minVals[j]) {
        for (int k = 3; k > j; k--) minVals[k] = minVals[k - 1];
        minVals[j] = val;
        break;
      }
    }
    for (int j = 0; j < 4; j++) {
      if (val > maxVals[j]) {
        for (int k = 3; k > j; k--) maxVals[k] = maxVals[k - 1];
        maxVals[j] = val;
        break;
      }
    }
  }
  
  uint64_t weightedSum = 0;
  uint64_t weightSum = 0;
  const float base = 1.1f;
  float weight = 1.0f;
  
  for (size_t i = 0; i < count; i++) {
    uint16_t val = buffer[i];
    bool isMinMax = false;
    for (int j = 0; j < 4; j++) {
      if (val == minVals[j] || val == maxVals[j]) {
        isMinMax = true;
        break;
      }
    }
    if (isMinMax) continue;
    uint64_t w = static_cast<uint64_t>(weight + 0.5f);
    weightedSum += static_cast<uint64_t>(val) * w;
    weightSum += w;
    weight *= base;
  }
  
  if (weightSum == 0) {
    return buffer[-1];
  }
  
  return static_cast<uint16_t>(weightedSum / weightSum);
}

void Device::setDirection(int32_t rotation) {
  rotation = rotation > STEER_MAX ? STEER_MAX : rotation < -STEER_MAX ? -STEER_MAX : rotation;
  uint32_t duty = STEER_CENTER + rotation;
  *PWM_TIM3_CH3_B0 = duty;
}

void Device::setMotorEnabled(bool enabled) {
  if (enabled) {
    HAL_GPIO_WritePin(Enable_IO_GPIO_Port, Enable_IO_Pin, GPIO_PIN_SET);
  } else {
    HAL_GPIO_WritePin(Enable_IO_GPIO_Port, Enable_IO_Pin, GPIO_PIN_RESET);    
  }
}

void Device::setPower(int32_t power) {
  power = power > POWER_MAX ? POWER_MAX : power < -POWER_MAX ? -POWER_MAX : power;
  if (power > 0) {
    *PWM_TIM4_CH2_B7 = power;
    HAL_GPIO_WritePin(Wheel_Left_IO_GPIO_Port, Wheel_Left_IO_Pin, GPIO_PIN_RESET);
  } else {
    *PWM_TIM4_CH2_B7 = 4800U + power;
    HAL_GPIO_WritePin(Wheel_Left_IO_GPIO_Port, Wheel_Left_IO_Pin, GPIO_PIN_SET);
  }
}


// Dull things

extern "C" {
  void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {
    if (hadc->Instance == ADC1) {
      BufferA4.push(ADC_RawValue[0]);
      BufferC5.push(ADC_RawValue[1]);
    }
  }
}

void Device::sendData(float a, float b) {
  float data[2] = {a, b};
  HAL_UART_Transmit(&huart1, (uint8_t*)data, sizeof(float) * 2, 10);
  HAL_UART_Transmit(&huart1, (uint8_t*)this->vofaEnd, 4, 10);
}

void Device::sendData(const std::vector<float>& datas) {
  HAL_UART_Transmit(&huart1, (uint8_t*)datas.data(), datas.size() * sizeof(float), 10);
  HAL_UART_Transmit(&huart1, (uint8_t*)this->vofaEnd, 4, 10);
}

union Convertor {
  float f;
  uint8_t bytes[4];
};

void Device::sendDataSafely(const std::vector<float>& datas) {
  for (float data : datas) {
    Convertor convertor;
    convertor.f = data;
    HAL_UART_Transmit(&huart1, convertor.bytes, 4, 10);
  }
  HAL_UART_Transmit(&huart1, (uint8_t*)this->vofaEnd, 4, 10);
}

void Device::initPWM() {
  // This function is partly from Li_Jiang's PWM.c in the Example

  // Config Output Mode
  TIM1->CCMR1 &= ~TIM_CCMR1_OC1M;
  TIM1->CCMR1 |= (TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2);

  TIM1->CCMR1 &= ~TIM_CCMR1_OC2M;
  TIM1->CCMR1 |= (TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2);

  TIM1->CCMR2 &= ~TIM_CCMR2_OC3M;
  TIM1->CCMR2 |= (TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2);

  // Enabler
  HAL_TIMEx_OCN_Start(&htim1, TIM_CHANNEL_1);   // TIM1_CH1N--->B13
  HAL_TIMEx_OCN_Start(&htim1, TIM_CHANNEL_2);   // TIM1_CH2N--->B14
  HAL_TIMEx_OCN_Start(&htim1, TIM_CHANNEL_3);   // TIM1_CH3N--->B15
  // HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_4);     // TIM1_CH4 --->A11

  // TIM4 Motor
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);     // TIM4_CH1---->B6
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_2);     // TIM4_CH2---->B7
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);     // TIM4_CH3---->B8
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);     // TIM4_CH4---->B9

  // TIM8 Motor
  HAL_TIM_PWM_Start(&htim8, TIM_CHANNEL_1);     // TIM8_CH1---->C6

  // TIM2 Steer
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);     // TIM2_CH1---->A0
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);     // TIM2_CH2---->A1

  // TIM3 Steer
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);     // TIM3_CH1---->A6
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);     // TIM3_CH2---->A7
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);     // TIM3_CH3---->B0

  // TIM5 Music
  HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_3);     // TIM5_CH3---->A2
  // HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_4);     // TIM5_CH4---->A3
}

void Device::initADC() {
    HAL_ADCEx_Calibration_Start(&hadc1);
    // HAL_Delay(500);
    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)ADC_RawValue, ADCChannelCount);
}
