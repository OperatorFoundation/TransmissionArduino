#ifndef RELIABLE_CONNECTION_H_
#define RELIABLE_CONNECTION_H_

#define EXPORT
#include <vector>
#include <string>
#include <cstdint>

class EXPORT Connection
{
    public:
        virtual ~Connection() = default;

        void write(std::string s);

        [[nodiscard]] virtual int tryReadOne() = 0;
        [[nodiscard]] virtual char readOne() = 0;
        [[nodiscard]] virtual std::vector<char> read(int size) = 0;
        virtual void write(std::vector<char> bs) = 0;
};

#endif
