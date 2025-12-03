// ScheduledTasks.h
// A unified and flexible Task Scheduler
// Date: Oct 2025
#pragma once
#include <cstdint>
#include <functional>
#include <algorithm>
#include <limits>
#include <vector>
#include <memory>
#include <unordered_set>
#include <utility>
#include <array>
#include "Device.h"

constexpr uint32_t INF_RUNS = std::numeric_limits<uint32_t>::max();

// The Base Class
class ScheduledTaskBase {
public:
  explicit ScheduledTaskBase(uint32_t period = 0)
    : period_ms(period), last_tick_ms(0), task_id(UINT32_MAX), finished_flag(false) {}
  virtual ~ScheduledTaskBase() = default;

  // tick: perform scheduled work
  virtual void tick(Device& dev, uint32_t now) = 0;

  // finished(): whether the task finished and should be removed
  virtual bool finished() const { return finished_flag; }

  // Getters / setters
  uint32_t period_ms;
  uint32_t last_tick_ms;
  uint32_t task_id;

protected:
  void markFinished() { finished_flag = true; }

private:
  bool finished_flag;
};


// Flexible ScheduledTask: supports either averaged step durations (from period)
// or custom per-step durations via array. Supports initial delay and max_runs.
template <size_t Steps>
class ScheduledTask : public ScheduledTaskBase {
  static_assert(Steps > 0, "ScheduledTask Steps must be > 0");
public:
  using StepCb = std::function<void(Device&, size_t)>;

  // 1) Averaged steps: provide total period (ms) and callback.
  //    Each step gets ceil(period / Steps) ms (at least 1).
  ScheduledTask(uint32_t period,
                StepCb cb,
                uint32_t max_runs = std::numeric_limits<uint32_t>::max())
    : ScheduledTaskBase(period),
      cb(cb),
      cur_step(0),
      initial_delay_ms(0),
      started(true),
      max_runs(max_runs),
      run_count(0)
  {
    init_average_steps(period);
  }

  // 2) Averaged steps + delayed start + optional max_runs
  ScheduledTask(uint32_t initial_delay,
                uint32_t period,
                StepCb cb,
                uint32_t max_runs = std::numeric_limits<uint32_t>::max())
    : ScheduledTaskBase(period),
      cb(cb),
      cur_step(0),
      initial_delay_ms(initial_delay),
      started(false),
      max_runs(max_runs),
      run_count(0)
  {
    init_average_steps(period);
  }

  // 3) Custom per-step durations (array), optional initial delay & max_runs.
  ScheduledTask(const std::array<uint32_t, Steps>& step_durations,
                StepCb cb,
                uint32_t initial_delay = 0,
                uint32_t max_runs = std::numeric_limits<uint32_t>::max())
    : ScheduledTaskBase(0),
      cb(cb),
      cur_step(0),
      step_durations(step_durations.begin(), step_durations.end()),
      initial_delay_ms(initial_delay),
      started(initial_delay == 0),
      max_runs(max_runs),
      run_count(0)
  {
    // ensure each step duration is at least 1 ms
    for (auto &d : this->step_durations) d = std::max<uint32_t>(1, d);
  }

  // Convenience constructor for callbacks of type std::function<void(Device&)>
  ScheduledTask(uint32_t period,
                std::function<void(Device&)> f,
                uint32_t max_runs = std::numeric_limits<uint32_t>::max())
    : ScheduledTask(period, StepCb([f](Device& d, size_t){ f(d); }), max_runs) {}

  ScheduledTask(uint32_t initial_delay,
                uint32_t period,
                std::function<void(Device&)> f,
                uint32_t max_runs = std::numeric_limits<uint32_t>::max())
    : ScheduledTask(initial_delay, period, StepCb([f](Device& d, size_t){ f(d); }), max_runs) {}

  ScheduledTask(const std::array<uint32_t, Steps>& step_durations,
                std::function<void(Device&)> f,
                uint32_t initial_delay = 0,
                uint32_t max_runs = std::numeric_limits<uint32_t>::max())
    : ScheduledTask(step_durations, StepCb([f](Device& d, size_t){ f(d); }), initial_delay, max_runs) {}

  void tick(Device& dev, uint32_t now) override {
    // If not started due to initial delay, check the delay first.
    if (!started) {
      if ((now - last_tick_ms) >= initial_delay_ms) {
        // advance last_tick by the delay consumed
        last_tick_ms += initial_delay_ms;
        started = true;
      } else {
        return; // still waiting for delay
      }
    }

    // If we have per-step durations vector, use it; otherwise use step_period_ms
    while (!reached_limit() && (now - last_tick_ms) >= current_step_duration()) {
      uint32_t dt = current_step_duration();
      last_tick_ms += dt;

      // call back with current step index
      cb(dev, cur_step);

      ++run_count;
      if (reached_limit()) {
        markFinished(); // schedule for removal by Scheduler
        break;
      }

      // advance step
      cur_step = (cur_step + 1) % Steps;
    }
  }

  bool finished() const override {
    return ScheduledTaskBase::finished();
  }

protected:
  StepCb cb;
  size_t cur_step;

  // per-step durations in ms (size Steps). If empty, we use step_period_ms.
  std::vector<uint32_t> step_durations;
  uint32_t step_period_ms = 0; // used when step_durations empty

  uint32_t initial_delay_ms;
  bool started;

  uint32_t max_runs;
  uint32_t run_count;

  bool reached_limit() const {
    return run_count >= max_runs;
  }

  uint32_t current_step_duration() const {
    if (!step_durations.empty()) return step_durations[cur_step];
    return step_period_ms;
  }

  void init_average_steps(uint32_t period) {
    // average/ceil period across Steps, ensure at least 1 ms per step.
    step_period_ms = std::max<uint32_t>(1, (period + Steps - 1) / Steps);
    step_durations.clear();
  }
};


// Specialization for Steps == 1 (simpler and efficient)
template <>
class ScheduledTask<1> : public ScheduledTaskBase {
public:
  using StepCb = std::function<void(Device&)>;

  // Normal (no delay) with optional max_runs
  ScheduledTask(uint32_t period, StepCb cb, uint32_t max_runs = std::numeric_limits<uint32_t>::max())
    : ScheduledTaskBase(period), cb_wrap([cb](Device& d, size_t){ cb(d); }),
      initial_delay_ms(0), started(true), max_runs(max_runs), run_count(0) {}

  // Delayed start
  ScheduledTask(uint32_t initial_delay, uint32_t period, StepCb cb, uint32_t max_runs = std::numeric_limits<uint32_t>::max())
    : ScheduledTaskBase(period), cb_wrap([cb](Device& d, size_t){ cb(d); }),
      initial_delay_ms(initial_delay), started(false), max_runs(max_runs), run_count(0) {}

  void tick(Device& dev, uint32_t now) override {
    if (!started) {
      if ((now - last_tick_ms) >= initial_delay_ms) {
        last_tick_ms += initial_delay_ms;
        started = true;
      } else {
        return;
      }
    }

    uint32_t dur = (single_step_duration > 0) ? single_step_duration : period_ms;
    if (dur == 0) dur = 1;
    if (!reached_limit() && (now - last_tick_ms) >= dur) {
      last_tick_ms += dur;
      cb_wrap(dev, 0);
      ++run_count;
      if (reached_limit()) markFinished();
    }
  }

  bool finished() const override { return ScheduledTaskBase::finished(); }

private:
  std::function<void(Device&, size_t)> cb_wrap;
  uint32_t initial_delay_ms;
  bool started;
  uint32_t single_step_duration = 0; // if >0 use instead of period_ms
  uint32_t max_runs;
  uint32_t run_count;

  bool reached_limit() const { return run_count >= max_runs; }
};


class Scheduler {
public:
  using TaskPtr = std::unique_ptr<ScheduledTaskBase>;

  Scheduler(uint32_t start_tick) : start_tick(start_tick), next_id(1), in_run(false), current_task(UINT32_MAX) {}

  // Add a task. If called during runOnce(), the task is queued and will be activated
  // after the current run loop. Returns assigned task id.
  uint32_t addTask(TaskPtr t) {
    if (!t) return UINT32_MAX;
    uint32_t id = next_id++;
    t->task_id = id;
    if (in_run) {
      pending_add.emplace_back(std::move(t));
    } else {
      tasks.emplace_back(std::move(t));
    }
    return id;
  }

  uint32_t addTaskWithID(TaskPtr t, uint32_t id) {
    if (!t) return UINT32_MAX;
    t->task_id = id;
    if (in_run) {
      pending_add.emplace_back(std::move(t));
    } else {
      tasks.emplace_back(std::move(t));
    }
    return id;
  }

  // Add and initialize last_tick_ms
  uint32_t addTaskAndInit(TaskPtr t) {
    if (!t) return UINT32_MAX;
    t->last_tick_ms = this->start_tick;
    return addTask(std::move(t));
  }
  uint32_t addTaskAndInit(TaskPtr t, uint32_t last_tick) {
    if (!t) return UINT32_MAX;
    t->last_tick_ms = last_tick;
    return addTask(std::move(t));
  }

  // Add and initialize and set ID
  uint32_t addTaskAndInitWithID(TaskPtr t, uint32_t last_tick, uint32_t id) {
    if (!t) return UINT32_MAX;
    t->last_tick_ms = last_tick;
    return addTaskWithID(std::move(t), id);
  }

  // Request removal of a task by id. If called during runOnce(), removal takes
  // effect immediately for subsequent ticks in the same run (the task is skipped),
  // and memory is reclaimed after run finishes.
  void removeTask(uint32_t id) {
    if (id == UINT32_MAX) return;
    pending_remove.insert(id);
    if (!in_run) {
      // not in run -> flush immediately
      flushPending();
    }
  }

  // Return the id of the task currently being ticked (or UINT32_MAX if none).
  uint32_t currentTaskId() const { return current_task; }

  // Run single scheduling iteration: tick all tasks with "now".
  // This should be called from your main loop repeatedly.
  void runOnce(Device& dev, uint32_t now) {
    in_run = true;
    for (size_t i = 0; i < tasks.size(); ++i) {
      TaskPtr &t = tasks[i];
      if (!t) continue;
      uint32_t id = t->task_id;
      if (pending_remove.find(id) != pending_remove.end()) continue;
      current_task = id;
      t->tick(dev, now);
      // If task reached its internal limit and marked finished, schedule its removal
      if (t->finished()) {
        pending_remove.insert(id);
      }
      current_task = UINT32_MAX;
    }
    in_run = false;
    flushPending();
  }

  // Convenience: number of active tasks (after pending flushed)
  size_t taskCount() const { return tasks.size(); }

  // Remove all tasks
  void clearAll() {
    tasks.clear();
    pending_add.clear();
    pending_remove.clear();
  }

  void resetTime(uint32_t tick) {
    this->start_tick = tick;
  }

private:
  uint32_t start_tick;
  uint32_t next_id;
  bool in_run;
  uint32_t current_task;

  std::vector<TaskPtr> tasks;
  std::vector<TaskPtr> pending_add;
  std::unordered_set<uint32_t> pending_remove;

  // integrate pending adds and remove flagged tasks
  void flushPending() {
    if (!pending_remove.empty()) {
      tasks.erase(std::remove_if(tasks.begin(), tasks.end(),
                                [&](const TaskPtr &t){
                                  return !t || pending_remove.find(t->task_id) != pending_remove.end();
                                }),
                  tasks.end());
      pending_remove.clear();
    }
    if (!pending_add.empty()) {
      for (auto &pt : pending_add) {
        if (pt) tasks.emplace_back(std::move(pt));
      }
      pending_add.clear();
    }
  }
};


// Factory helpers
inline std::unique_ptr<ScheduledTask<1>> makeTask(
    uint32_t period_ms,
    std::function<void(Device&)> cb,
    uint32_t max_runs = INF_RUNS)
{
    return std::make_unique<ScheduledTask<1>>(period_ms, cb, max_runs);
}

inline std::unique_ptr<ScheduledTask<1>> makeTask(
    uint32_t initial_delay,
    uint32_t period_ms,
    std::function<void(Device&)> cb,
    uint32_t max_runs = INF_RUNS)
{
    return std::make_unique<ScheduledTask<1>>(initial_delay, period_ms, cb, max_runs);
}

template <size_t Steps>
inline std::unique_ptr<ScheduledTask<Steps>> makeStepTask(
    uint32_t period_ms,
    typename ScheduledTask<Steps>::StepCb cb,
    uint32_t max_runs = INF_RUNS)
{
    return std::make_unique<ScheduledTask<Steps>>(period_ms, cb, max_runs);
}

template <size_t Steps>
inline std::unique_ptr<ScheduledTask<Steps>> makeStepTask(
    uint32_t initial_delay,
    uint32_t period_ms,
    typename ScheduledTask<Steps>::StepCb cb,
    uint32_t max_runs = INF_RUNS)
{
    return std::make_unique<ScheduledTask<Steps>>(initial_delay, period_ms, cb, max_runs);
}

template <size_t Steps>
inline std::unique_ptr<ScheduledTask<Steps>> makeStepTask(
    const std::array<uint32_t, Steps>& step_durations,
    typename ScheduledTask<Steps>::StepCb cb,
    uint32_t initial_delay = 0,
    uint32_t max_runs = INF_RUNS)
{
    return std::make_unique<ScheduledTask<Steps>>(step_durations, cb, initial_delay, max_runs);
}

template <size_t Steps>
inline std::unique_ptr<ScheduledTask<Steps>> makeStepTask(
    uint32_t period_ms,
    std::function<void(Device&)> cb,
    uint32_t max_runs = INF_RUNS)
{
    return std::make_unique<ScheduledTask<Steps>>(period_ms, cb, max_runs);
}

template <size_t Steps>
inline std::unique_ptr<ScheduledTask<Steps>> makeStepTask(
    uint32_t initial_delay,
    uint32_t period_ms,
    std::function<void(Device&)> cb,
    uint32_t max_runs = INF_RUNS)
{
    return std::make_unique<ScheduledTask<Steps>>(initial_delay, period_ms, cb, max_runs);
}

template <size_t Steps>
inline std::unique_ptr<ScheduledTask<Steps>> makeStepTask(
    const std::array<uint32_t, Steps>& step_durations,
    std::function<void(Device&)> cb,
    uint32_t initial_delay = 0,
    uint32_t max_runs = INF_RUNS)
{
    return std::make_unique<ScheduledTask<Steps>>(step_durations, cb, initial_delay, max_runs);
}
