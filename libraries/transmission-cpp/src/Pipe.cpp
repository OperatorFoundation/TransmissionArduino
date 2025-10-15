//
// Created by Dr. Brandon Wiley on 10/15/25.
//

#include "Pipe.h"

#include <stdexcept>

// Pipe implementation
Pipe::Pipe()
    : end_a(std::make_unique<PipeEnd>(buffer_b_to_a, buffer_a_to_b)),
      end_b(std::make_unique<PipeEnd>(buffer_a_to_b, buffer_b_to_a))
{
}

Connection& Pipe::getEndA() {
  return *end_a;
}

Connection& Pipe::getEndB() {
  return *end_b;
}

// PipeEnd implementation
PipeEnd::PipeEnd(InterruptSafeRingBuffer<char, 4096>& read_buf,
                 InterruptSafeRingBuffer<char, 4096>& write_buf)
    : read_buffer(read_buf), write_buffer(write_buf)
{
}

int PipeEnd::tryReadOne() {
  char ch;
  if (read_buffer.get(ch)) {
    return static_cast<unsigned char>(ch);
  }
  return -1;  // No data available
}

char PipeEnd::readOne() {
  char ch;
  if (!read_buffer.get(ch)) {
    throw std::runtime_error("Pipe read failed: no data available");
  }
  return ch;
}

std::vector<char> PipeEnd::read(int size) {
  std::vector<char> result;
  result.reserve(size);

  char ch;
  for (int i = 0; i < size; ++i) {
    if (!read_buffer.get(ch)) {
      break;  // Return partial read if not enough data
    }
    result.push_back(ch);
  }

  return result;
}

void PipeEnd::write(std::vector<char> bs) {
  for (char ch : bs) {
    if (!write_buffer.put(ch)) {
      throw std::runtime_error("Pipe write failed: buffer full");
    }
  }
}

size_t PipeEnd::available() const {
  return read_buffer.count();
}

size_t PipeEnd::writeSpace() const {
  return write_buffer.free();
}

void PipeEnd::flush() {
  // For pipes, flush is a no-op since writes are immediate
}