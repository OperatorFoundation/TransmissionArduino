//
// Created by Dr. Brandon Wiley on 5/26/25.
//

#include "Connection.h"

#include <cstring>

void Connection::write(std::string s)
{
  std::vector<char> bs(s.begin(), s.end());
  write(bs);
}