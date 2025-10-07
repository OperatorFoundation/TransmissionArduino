//
// Created by Dr. Brandon Wiley on 3/17/25.
//

#include "TransmissionMain.h"

int main()
{
  TransmissionMain transmission_main = TransmissionMain();
  while(true)
  {
    transmission_main.loop();
  }
}
