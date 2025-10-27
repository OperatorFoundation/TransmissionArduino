#ifndef PIPE_H_
#define PIPE_H_

#include "Connection.h"
#include "ring_buffer.h"
#include <memory>

// PipeEnd must be fully defined before Pipe since Pipe uses unique_ptr<PipeEnd>
class EXPORT PipeEnd : public Connection {
  public:
    PipeEnd(InterruptSafeRingBuffer<char, 4096>& read_buf,
            InterruptSafeRingBuffer<char, 4096>& write_buf);

    [[nodiscard]] int tryReadOne() override;
    [[nodiscard]] char readOne() override;
    [[nodiscard]] std::vector<char> read(int size) override;
    void write(std::vector<char> bs) override;

    // Additional utility methods for testing
    size_t available() const;
    size_t writeSpace() const;
    void flush();

  private:
    InterruptSafeRingBuffer<char, 4096>& read_buffer;
    InterruptSafeRingBuffer<char, 4096>& write_buffer;
};

class EXPORT Pipe {
  public:
    // Create a pipe with two connected ends
    Pipe();
    ~Pipe() = default;

    // Get the two connection ends
    Connection& getEndA();
    Connection& getEndB();

    // Disable copying and moving
    Pipe(const Pipe&) = delete;
    Pipe& operator=(const Pipe&) = delete;
    Pipe(Pipe&&) = delete;
    Pipe& operator=(Pipe&&) = delete;

  private:
    // Two ring buffers for bidirectional communication
    // Buffer A->B: written by end A, read by end B
    InterruptSafeRingBuffer<char, 4096> buffer_a_to_b;
    // Buffer B->A: written by end B, read by end A
    InterruptSafeRingBuffer<char, 4096> buffer_b_to_a;

    std::unique_ptr<PipeEnd> end_a;
    std::unique_ptr<PipeEnd> end_b;
};

#endif // PIPE_H_