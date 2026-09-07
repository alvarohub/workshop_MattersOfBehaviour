#ifndef MBK_CIRCULAR_BUFFER_H
#define MBK_CIRCULAR_BUFFER_H

#include <Arduino.h>
#include "Command.h"

template <size_t N = 16>
struct CircularBuffer {
  static constexpr size_t capacity = N;
  Command buffer[N]{};
  size_t count = 0;
  size_t head = 0;
  size_t tail = 0;

  bool isEmpty() const { return count == 0; }
  bool isFull() const { return count == capacity; }
  size_t size() const { return count; }
  
  void clear() {
    head = 0;
    tail = 0;
    count = 0;
  }
  
  const Command& peek() const {
    assert(!isEmpty());
    return buffer[head];
  }

  const Command& pop() {
    assert(!isEmpty());
    const Command& item = buffer[head];
    head = (head + 1) % capacity;
    count--;
    return item;
  }
  
  bool push(const Command& cmd) {
    if (count == capacity) {
      head = (head + 1) % capacity;
    } else {
      count++;
    }
    buffer[tail] = cmd;
    tail = (tail + 1) % capacity;
    return count < capacity;
  }

  bool push(const char* data, size_t len) {
    Command cmd;
    memcpy(cmd.data, data, len);
    cmd.len = len;
    return push(cmd);
  }
};

#endif