//
// Created by Dr. Brandon Wiley on 3/23/25.
//

#define CATCH_CONFIG_RUNNER
#include <catch2/catch_all.hpp>

int main(int argc, char** argv)
{
  const int result = Catch::Session().run(argc, argv);

  return result;

}
