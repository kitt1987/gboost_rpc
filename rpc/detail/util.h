#pragma once
#include "base.h"
#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

inline void SleepInSeconds(int32 seconds) {
#ifdef WIN32
  Sleep(seconds * 1000);
#else
  uint32 left = static_cast<uint32>(seconds);
  while (left > 0) {
    left = sleep(left);
  }
#endif
}

template<typename Integer>
std::string ConvertToStringFrom(Integer integer, int const number_system) {
  std::string text;
  if (integer == 0)
    return "0";

  CHECK(
      number_system == 2 || number_system == 8 || number_system == 10
          || number_system == 16);

  bool minus = integer < Integer();
  if (minus)
    integer = static_cast<Integer>(~(integer - 1));

  while (integer > 0) {
    Integer mod = integer % static_cast<Integer>(number_system);
    if (mod < 10) {
      text.push_back(static_cast<char>('0' + mod));
    } else {
      text.push_back(static_cast<char>('A' + mod - 10));
    }

    integer /= static_cast<Integer>(number_system);
  }

  if (minus)
    text.push_back('-');

  std::reverse(text.begin(), text.end());
  return text;
}

template<typename Container>
void ClearHeapElemsInSequentialContainer(Container* container) {
  if (container->empty())
    return;

  for (typename Container::iterator each = container->begin();
      each != container->end(); ++each) {
    delete *each;
  }

  container->clear();
}

template<typename Container>
void ClearHeapElemsInAssociativeContainer(Container* container) {
  if (container->empty())
    return;

  for (typename Container::iterator each = container->begin();
      each != container->end(); ++each) {
    delete each->second;
  }

  container->clear();
}
