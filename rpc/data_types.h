#ifndef BASE_DATA_TYPES_H_
#define BASE_DATA_TYPES_H_

#include <string>
#include <queue>
#include <set>

#ifndef WIN32
#include <unistd.h>
#endif

#ifdef WIN32
typedef __int8 int8;
typedef unsigned __int8 uint8;
typedef __int16 int16;
typedef unsigned __int16 uint16;
typedef __int32 int32;
typedef unsigned __int32 uint32;
typedef __int64 int64;
typedef unsigned __int64 uint64;
#else
typedef int8_t int8;
typedef uint8_t uint8;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;
#endif

#endif /* BASE_DATA_TYPES_H_ */
