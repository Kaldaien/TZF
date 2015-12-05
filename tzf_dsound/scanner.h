#ifndef __TZF__SCANNER_H__
#define __TZF__SCANNER_H__

#include <stdint.h>

void* TZF_Scan (uint8_t* pattern, size_t len, uint8_t* mask = nullptr);

#endif /* __TZF__SCANNER_H__ */