// Buffer.h
// A simple buffer for ADC storage
// Date: Oct 2025
#pragma once
#include <array>
#include <algorithm>
#include <cstdint>

constexpr std::size_t BufferSize{64};
constexpr std::size_t ADCChannelCount{2u};

template<typename T, std::size_t N>
class Buffer {
public:
  Buffer() { buffer.fill(T{}); }

  void push(T item) {
    buffer[head] = item;
    head = (head + 1) % N;
  }

  std::size_t size() const { return N; }

  const T& getMax() const { return *std::max_element(buffer.cbegin(), buffer.cend()); }
  const T& getMin() const { return *std::min_element(buffer.cbegin(), buffer.cend()); }

  T& operator[](int index) {
    return buffer[(head + N + index) % N];
  }
  const T& operator[](int index) const {
    return buffer[(head + N + index) % N];
  }

private:
  std::array<T, N> buffer;
  std::size_t head = 0;
};

extern Buffer<uint16_t, BufferSize> BufferA4;
extern Buffer<uint16_t, BufferSize> BufferC5;
extern volatile uint16_t ADC_RawValue[ADCChannelCount];
