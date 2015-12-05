#include "scanner.h"

#include <Windows.h>

void*
TZF_Scan (uint8_t* pattern, size_t len)
{
  UINT_PTR addr  = (UINT_PTR)GetModuleHandle (L"Tales of Zestiria.exe");

  uint8_t*  begin = (uint8_t *)addr;
  uint8_t*  it    = begin;
  int       idx   = 0;

  while (it)
  {
    uint8_t* scan_addr = it;

    if (*scan_addr == pattern [idx]) {
      if (++idx == len)
        return (void *)begin;

      ++it;
    } else {
      it  = ++begin;
      idx = 0;
    }
  }

  return nullptr;
}