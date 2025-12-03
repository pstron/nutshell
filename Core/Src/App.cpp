// App.cpp
// Event-driven app using scheduled tasks
// Date: Oct 2025
#include <cstdint>
#include <cmath>
#include <cstdlib>
#include <utility>
#include "App.h"
#include "Device.h"
#include "ScheduledTask.h"
#include "Melodies.h"

constexpr uint8_t runMode = 0;
constexpr int32_t SpeedBase = 650;

constexpr struct {
  uint8_t Lights[20]{1, 2, 4, 8, 4, 2, 1, 2, 4, 8, 4, 2, 1, 5, 10, 5, 10, 5, 10, 0};
  uint32_t MusicTaskID = 2147480000;
  std::size_t BufferSize = 4;
  std::size_t sBufferSize = 20;
} Setting;

Buffer<int32_t, Setting.BufferSize> LBuffer;
Buffer<int32_t, Setting.BufferSize> RBuffer;
Buffer<int32_t, Setting.sBufferSize> ErrBuffer;

struct Params {
  float Kp = 0.0f;
  float Ki = 0.0f;
  float Kd = 0.0f;
  uint16_t DeadZone = 0;
  uint16_t StraightZone = 1250; // When abs(R-L) is less than StraightZone, switch to Track::Mid
  uint16_t OutZone = 2000; // When L+R is less than OutZone, considering Max steering
  int32_t Speed = SpeedBase;
};

struct {
  Params Default = {
    0.044f,
    0.0f,
    0.18f,
    250,
    0,
    1600,
    SpeedBase,
  };
  Params Straight = {
    0.015f,
    0.00f,
    0.25f,
    // 0.10f,
    // 800,
    0,
    1250,
    1800,
    SpeedBase,
  };
  // Params Entry = {};
  Params Mid = {
    0.04f,
    0.00f,
    0.6f,
    // 0.53f,
    0,
    1000,
    3000,
    SpeedBase - 150,
  };
  // Params Exit = {};
  bool SteerEnabled = false;
  uint16_t StartDelay = 2200;
  // uint16_t StartDelay = 0;
  bool UseStop = false;
  uint32_t BrakingTime = 200;
  int32_t BrakingSpeed = -600; // This can be linear decreasing
  bool UseFilter = false;
  bool UseAnalysis = false; // Enable Analysis to switch different Track Conditions
  bool UseRelay = false;
  uint8_t StopPassNeeded = 2;
} Config;

enum class Track : uint8_t {
  Straight = 0,
  Mid, // In a curve
  Default,
};

enum class ControlMode : uint8_t {
  PID = 0,
  Stop,
  Max,
  DOS,
};

struct {
  bool Stopped = false;
  bool Started = false;
  uint8_t MusicPlaying = 0;
  Track Condition = Track::Default;
  float Kp = Config.Default.Kp;
  float Ki = Config.Default.Ki;
  float Kd = Config.Default.Kd;
  int32_t DeadZone = Config.Default.DeadZone;
  int32_t StraightZone = Config.Default.StraightZone;
  int32_t OutZone = Config.Default.OutZone;
  int32_t Speed = Config.Default.Speed;
  uint8_t StopPassed = 0;
  ControlMode Control = ControlMode::PID;
} State;

[[maybe_unused]]
const auto CreatePlayRunningAbout = [](){
  return makeStepTask<2304>(38400, [](Device& dev, size_t step){
    dev.playNote(Melody::RunningAbout[step]);
    dev.playLight(Melody::RunningAbout[step] != Melody::Note::STOP);
  });
};

[[maybe_unused]]
const auto CreatePlayLevelComplete = [](){
  return makeStepTask<42>(5400, [](Device& dev, size_t step){
    dev.playNote(Melody::LevelComplete[step]);
    dev.playLight((step & 1) || step > 28);
  }, 42);
};

[[maybe_unused]]
const auto CreatePlayYouHaveDied = [](){
  return makeStepTask<156>(2600, [](Device& dev, size_t step){
    dev.playNote(Melody::YouHaveDied[step]);
    dev.playLight(Melody::YouHaveDied[step] == Melody::Note::STOP);
  }, 156);
};

void App() {
  Device device;

  // capture a single start tick to initialize tasks so they won't fire immediately
  uint32_t start_tick = device.getTick();

  Scheduler scheduler(start_tick);

  // Task: Board IO
  auto boardIOTaskID = scheduler.addTaskAndInit(
    makeTask(50, [](Device& dev){
      static uint8_t count{0};
      count = (count + 1) % 20;
      // Switch Input
      switch (runMode) {
        case 0:
          // Prepared Configs for Contest
          Config.SteerEnabled = true;
          // Config.UseFilter = true;
          Config.UseFilter = true;
          Config.UseStop = (dev.switchStatus() & 0b1000) >> 3;
          Config.UseAnalysis = true;
          // Config.UseAnalysis = (dev.switchStatus() & 0b0100) >> 2;
          // Config.UseRelay = (dev.switchStatus() & 0b0100) >> 2;
          Config.Straight.Speed = dev.switchStatus() & 0b0111 ? 700 : 650;
          Config.BrakingTime = 500;
          Config.BrakingSpeed = -1000;
          Config.Mid.Speed = !(dev.switchStatus() & 0b0111) ? 500 : 580 + 20 * ((dev.switchStatus() & 0b0111) - 1);
          /*
          switch (dev.switchStatus() & 0b0111) {
            case 0b0000:
            default:
              Config.Straight.Speed = 650;
              Config.Mid.Speed = 500;
              Config.Default = {
                0.0f,
                0.0f,
                0,
              };
              Config.Default.Speed = 500;
              if (!Config.UseAnalysis) Config.SteerEnabled = false;
              break;
            case 0b0001:
              if (Config.UseAnalysis) {
                Config.BrakingTime = 500;
                Config.BrakingSpeed = -1000;
              }
              Config.Straight.Speed = 750;
              Config.Mid.Speed = 600;
              Config.Default = {
                0.044f,
                0.18f,
                250,
              };
              Config.Default.Speed = 500;
              break;
            case 0b0010:
              if (Config.UseAnalysis) {
                Config.BrakingTime = 500;
                Config.BrakingSpeed = -1000;
              }
              Config.Straight.Speed = 750;
              Config.Mid.Speed = 650;
              Config.Default = {
                0.044f,
                0.18f,
                250,
              };
              Config.Default.Speed = 550;
              break;
            case 0b0011:
              if (Config.UseAnalysis) {
                Config.BrakingTime = 500;
                Config.BrakingSpeed = -1000;
              }
              Config.Straight.Speed = 750;
              Config.Mid.Speed = 700;
              Config.Default = {
                0.044,
                0.18,
                200,
              };
              Config.Default.Speed = 450;
              break;
          }
          */
          break;
        case 1: 
          // Static Test Cases
          // Config.Default.Kp = 0.04f + 0.01f * (dev.switchStatus() & 0b1111);
          Config.Default.Kp = 0.04f;
          Config.Default.Kd = 0.45f + 0.01f * (dev.switchStatus() & 0b1111);
          // Config.Default.Kd = 0.25f;
          // Config.Default.Speed = 500 + 100 * dev.switchStatus();
          Config.Default.Speed = 600;
          // Config.BrakingSpeed = -1500;
          Config.Default.DeadZone = 0;
          // Config.Kd = 0.01f + 0.01f * (dev.switchStatus() & 0b0011);
          Config.UseStop = false;
          // Config.SteerEnabled = (dev.switchStatus() & 0b1000) >> 3;
          Config.SteerEnabled = true;
          // Config.Speed = SpeedBase + 50 * (dev.switchStatus() >> 3);
          // Config.UseFilter = (dev.switchStatus() & 0b1000) >> 3;
          Config.UseFilter = true;
          // Config.StartDelay = dev.switchStatus() & 0b0100 ? 2200 : 0;
          Config.StartDelay = 0;
          // Config.UseAnalysis = dev.switchStatus() & 0b0001;
          Config.UseAnalysis = false;
          break;
        case 2:
          // Dynamic Adjusting
          Config.UseStop = true;
          Config.UseFilter = true;
          Config.Straight.Speed = SpeedBase + 50 * ((dev.switchStatus() & 0b0010) >> 1);
          Config.SteerEnabled = (dev.switchStatus() & 0b0100) >> 2;
          auto& param = Config.Straight.Kd;
          auto step = 0.01f;
          if (count == 0) {
            if (dev.switchStatus() & 0b0001) {
              dev.light(Device::LightMode::Adjusting, 0b1101);
              param += step;
            } else if (dev.switchStatus() & 0b1000) {
              dev.light(Device::LightMode::Adjusting, 0b1110);
              param -= step;
            } else {
              dev.light(Device::LightMode::Adjusting, 0b1100);
            }
          } else if (count < 10) {
            dev.light(Device::LightMode::Adjusting, 0b1000);
          } else {
            dev.light(Device::LightMode::Adjusting, 0b0000);
          }
          break;
      }
      // Board LED Output
      dev.light(Device::LightMode::Normal, dev.switchStatus());
      if (dev.getStopSignal() && count & 1)
        dev.forceLight(0b1111); // To show that the stop signal or the relay signal is gotten
    })
  );

  // Task: Enable Switch IO
  auto enableIOTaskID = scheduler.addTaskAndInit(
    makeTask(50, [&scheduler](Device& dev){
      static bool enabledPrev = false;
      bool enabledNow = dev.isEnabled();
      // Start
      if (enabledNow && !enabledPrev) {
        auto Start = makeTask(Config.StartDelay, 1, [&scheduler](Device& dev){
          dev.setMotorEnabled(true);
          dev.setPower(Config.Straight.Speed);
          State.Started = true;
          if (State.MusicPlaying != 1) {
            scheduler.removeTask(Setting.MusicTaskID);
            scheduler.addTaskAndInitWithID(CreatePlayRunningAbout(), dev.getTick(), Setting.MusicTaskID);
            State.MusicPlaying = 1;
          }
        }, 1);
        scheduler.addTaskAndInit(std::move(Start), dev.getTick());
      }
      // Speed Adjusting
      if (State.Started && enabledNow) {
        dev.setPower(State.Speed);
      }
      // Halt
      if (!enabledNow) {
        dev.setMotorEnabled(false);
        dev.setPower(0);
        dev.playNote(Melody::Note::STOP);
        scheduler.removeTask(Setting.MusicTaskID);
        State.MusicPlaying = 0;
        State.Started = false;
      }
      enabledPrev = enabledNow;
    })
  );

  // Task: Speed Control
  // auto speedTaskID = scheduler.addTaskAndInit(
  //   makeStepTask<100>(100, [](Device& dev, size_t step){
  //     static uint16_t count = 0;
  //     static bool prev = false;
  //     static int32_t I = 0;
  //     static int32_t prev_err = 0;
  //     int32_t power = State.Speed;
  //     bool now = dev.getIRSignal();
  //     // [TODO] Adjust these
  //     const float Kp = 0.0f + 10.0f * dev.switchStatus();
  //     constexpr float Ki = 0.0f;
  //     constexpr float Kd = 0.0f;
  //     constexpr float Kt = 0.0f;
  //     if (prev ^ now) ++count;
  //     prev = now;
  //     if (step == 0) count = 0;
  //     if (step == 0 && State.Started && dev.isEnabled()) {
  //       if (Config.UseSpeed) {
  //         uint16_t target = std::max((uint16_t)((int32_t)14 - (int32_t)(Kt * std::abs(State.pid_out))), (uint16_t)10/*[TODO]*/);
  //         int32_t err = target - count;
  //         float P = static_cast<int32_t>(Kp * err);
  //         I += err;
  //         I *= 0.9f;
  //         I = std::clamp(I, (int32_t)-500, (int32_t)500);
  //         int32_t D = err - prev_err;
  //         int32_t pid = P + static_cast<int32_t>(Ki * I) + static_cast<int32_t>(Kd * D);
  //         power = 750 + pid;
  //         dev.setPower(power);
  //         prev_err = err;
  //       } else {
  //         power = State.Speed;
  //         dev.setPower(std::max(power, (int32_t)500));
  //       }
  //       dev.sendData({(float)count, (float)power});
  //       count = 0;
  //     }
  //   })
  // );

  // Task: Light Show On Startup
  scheduler.addTaskAndInit(
    makeStepTask<20>(1600, [](Device& dev, size_t step){
      dev.light(Device::LightMode::Show, Setting.Lights[step]);
    }, 20)
  );

  // Task: Let Light Display In Proper Mode
  scheduler.addTaskAndInit(
    makeTask(1600, 1, [](Device& dev){
      switch (runMode) {
        case 0:
          dev.setLightMode(Device::LightMode::Normal);
          break;
        case 2:
          dev.setLightMode(Device::LightMode::Adjusting);
          break;
        default:
          dev.setLightMode(Device::LightMode::Normal);
          break;
      }
    }, 1)
  );

  // Task: ADC Data Collection
  auto dataCollectionTaskID = scheduler.addTaskAndInit(
    makeTask(20, 5, [](Device& dev){
      LBuffer.push(dev.getNoseADC(Device::NoseID::L, Config.UseFilter));
      RBuffer.push(dev.getNoseADC(Device::NoseID::R, Config.UseFilter));
    })
  );
  
  // Task: Statistic Data Collection
  auto sdataCollectionTaskID = scheduler.addTaskAndInit(
    makeTask(20, 50, [](Device& dev){
      int32_t latest_err = State.Control == ControlMode::DOS ? (RBuffer[-1] - LBuffer[-1] / (RBuffer[-1] + RBuffer[-1])) : RBuffer[-1] - LBuffer[-1];
      ErrBuffer.push(latest_err);
    })
  );

  // Task: PD control for direction & Send debug messages
  auto controlTaskID = scheduler.addTaskAndInit(
    makeTask(20, 20, [&](Device& dev){
      uint16_t ad_left = LBuffer[-1], ad_right = RBuffer[-1];
      int32_t latest_err = State.Control == ControlMode::DOS ? (RBuffer[-1] - LBuffer[-1] / (RBuffer[-1] + RBuffer[-1])) : RBuffer[-1] - LBuffer[-1];
      int32_t previous_err = State.Control == ControlMode::DOS ? (RBuffer[-2] - LBuffer[-2] / (RBuffer[-2] + RBuffer[-2])) : RBuffer[-2] - LBuffer[-2];

      [[maybe_unused]]
      uint16_t stateFlag = 0;
      // Changing State based on Analysis
      if (Config.UseAnalysis) {
        if (std::abs(latest_err) > State.StraightZone || ad_left + ad_right < State.OutZone) {
          State.Condition = Track::Mid;
          State.Control = ad_left + ad_right < State.OutZone ? ControlMode::Max : ControlMode::PID;
          stateFlag = ad_left + ad_right < State.OutZone ? 3000 : 3500;
        } else {
          State.Condition = Track::Straight;
          State.Control = ControlMode::PID;
          stateFlag = 0;
        }
      }
      // Staistic
      /*
      int64_t sum = 0;
      uint64_t sum_sq = 0;

      for (size_t i = 1; i <= Setting.sBufferSize; ++i) {
          int32_t value = ErrBuffer[-i];
          sum += value;
          sum_sq += (uint64_t)value * (uint64_t)value;
      }

      int32_t mean = sum / Setting.sBufferSize;
      uint64_t variance = (sum_sq / Setting.sBufferSize) - (mean * mean); 
      */

      // if (State.Condition == Track::Mid && ad_left + ad_right > 4000 && ad_left > 1500 && ad_right > 1500) {
      // if (variance > 250000) {
      //   State.Condition = Track::Straight;
      //   State.Control = ControlMode::PID;
      //   stateFlag = 2500;
      // }

      const auto& config = [&]() -> const auto& {
        switch (State.Condition) {
          case Track::Straight: return Config.Straight;
          case Track::Mid:      return Config.Mid;
          default:
          case Track::Default:  return Config.Default;
        }
      }();
      State.Kp = config.Kp;
      State.Ki = config.Ki;
      State.Kd = config.Kd;
      State.DeadZone = config.DeadZone;
      State.StraightZone = config.StraightZone;
      State.OutZone = config.OutZone;
      State.Speed = config.Speed;
      if (std::abs(latest_err) < State.DeadZone) State.Control = ControlMode::Stop;

      // Integral is removed temporarily
      // constexpr float dt = 0.005f;
      // static float integral = 0.0f;
      // constexpr float integralMax = 2000.0f;
      // constexpr float integralMin = -2000.0f;

      float P = static_cast<float>(latest_err);

      // integral += static_cast<float>(latest_err) * dt;
      // integral = std::clamp(integral, integralMin, integralMax);
      // float I = integral;

      float D = 0.0f;
      D = latest_err - previous_err;

      float pid_out = State.Kp * P + /*State.Ki * I*/ + State.Kd * D;

      [[maybe_unused]]
      uint16_t outFlag = 0; // Flag used for debugging

      if (Config.SteerEnabled) {
        switch (State.Control) {
          case ControlMode::Stop:
            dev.setDirection(0);
            break;
          case ControlMode::Max:
            if (ad_left < ad_right) {
              outFlag = 3500;
              dev.setDirection(90);
            } else {
              outFlag = 2500;
              dev.setDirection(-90);
            }
            break;
          [[likely]]
          case ControlMode::PID:
          case ControlMode::DOS:
            dev.setDirection(static_cast<int32_t>(pid_out));
            break;
        }
      } else {
        dev.setDirection(0); // Steer not enabled
      }

      dev.sendData({(float)ad_left, (float)ad_right, (float)latest_err, (float)stateFlag, P, D, (float)(dev.switchStatus() * 100)});
    })
  );

  // Task: Add Task Check Stop when it's proper
  auto stopTriggerTaskID = scheduler.addTaskAndInit(
    makeTask(50, [&scheduler](Device& dev){
      // Task: Check if reached the stop
      auto checkStop = makeTask(5000, 5, [](Device& dev){
        if (Config.UseStop) {
          static bool prev = 0;
          bool now = dev.getStopSignal();
          if (prev == 0 && now == 1) ++State.StopPassed;
          prev = now;
        }
      });
      if ((Config.UseStop || Config.UseRelay) && dev.isEnabled()) {
        scheduler.addTaskAndInit(std::move(checkStop), dev.getTick());
        scheduler.removeTask(scheduler.currentTaskId());
      }
    })
  );

  // Task: Braking
  auto brake = makeTask(1, [](Device& dev){
    dev.setPower(Config.BrakingSpeed);
    if (Config.UseRelay) dev.buzz(true);
  }, 1);

  // Task: After Stopping
  const auto createStop = [](){
    return makeTask(Config.BrakingTime, 1, [](Device& dev){
      dev.setMotorEnabled(false);
      dev.setPower(0);
      dev.setLightMode(Device::LightMode::Show);
    }, 1);
  };

  // Task: Light Show In the End
  auto endShow = makeStepTask<20>(1600, [](Device& dev, size_t step){
    dev.light(Device::LightMode::Show, Setting.Lights[step]);
  }, 20);

  auto relayBuzz = makeTask(3000, 1, [](Device& dev){
    dev.buzz(false);
  }, 1);

  // Main loop
  while (1) {
    uint32_t now = device.getTick();
    static bool stopped = false;
    scheduler.runOnce(device, now);
    if (State.StopPassed >= Config.StopPassNeeded) {
      State.Stopped = true;
    }
    if (State.Stopped && !stopped) {
      scheduler.removeTask(boardIOTaskID);
      scheduler.removeTask(enableIOTaskID);
      scheduler.removeTask(dataCollectionTaskID);
      scheduler.removeTask(sdataCollectionTaskID);
      scheduler.removeTask(controlTaskID);
      scheduler.removeTask(stopTriggerTaskID);
      scheduler.removeTask(Setting.MusicTaskID);
      scheduler.resetTime(now);
      stopped = true;
      device.setDirection(0);
      scheduler.addTaskAndInit(std::move(brake));
      scheduler.addTaskAndInit(createStop());
      scheduler.addTaskAndInit(std::move(endShow));
      scheduler.addTaskAndInit(std::move(relayBuzz));
      scheduler.addTaskAndInit(CreatePlayLevelComplete());
    }
    device.delay(1);
  }
}
