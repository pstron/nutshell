// Device.h
// The declaration of class Device
// Date: Oct 2025
#pragma once
#include <vector>
#include <stdint.h>
#include "Melodies.h"
#include "Buffer.h"

class Device {
public:
  enum class SwitchID : uint8_t {
    SW1 = 0,
    SW2,
    SW3,
    SW4,
  };
  enum class NoseID : uint8_t {
    L = 0,
    R,
  };
  enum class LightMode : uint8_t {
    Normal = 0,
    Show,
    Adjusting,
  };

  Device();
  Device(Device &&) = default;
  Device(const Device &) = default;
  Device &operator=(Device &&) = default;
  Device &operator=(const Device &) = default;
  ~Device() = default;
  
  void delay(uint32_t ms);
  uint32_t getTick();
  void setLightMode(LightMode mode);
  LightMode getLightMode();

  void light(LightMode mode, uint8_t x);
  void forceLight(uint8_t id, bool light);
  void forceLight(uint8_t x);

  void buzz(bool enabled);

  // void IOLight(uint8_t x);
  // void IOLightPWM(uint8_t id, uint32_t duty);

  void playNote(Melody::Note note);
  void playLight(bool set);

  uint8_t switchStatus();
  bool switchOn(SwitchID id);

  bool isEnabled();
  bool getStopSignal();
  bool getIRSignal();
  uint16_t getNoseADC(NoseID id, bool enableFiltering = false);
  void setDirection(int32_t rotation);
  void setMotorEnabled(bool enabled);
  void setPower(int32_t power);

  void sendData(float a, float b);
  void sendData(const std::vector<float>& datas);
  void sendDataSafely(const std::vector<float>& datas);

private:
  static constexpr uint32_t STEER_CENTER{741};
  static constexpr int32_t STEER_MAX{90};
  static constexpr int32_t POWER_MAX{4800};
  static constexpr uint8_t vofaEnd[4] = {0x00, 0x00, 0x80, 0x7f};
  void initPWM();
  void initADC();
  uint16_t getFiltered(Buffer<uint16_t, BufferSize> buffer);
  LightMode devLightMode{LightMode::Show};
};
