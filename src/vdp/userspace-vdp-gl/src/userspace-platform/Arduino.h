#pragma once

#ifdef USERSPACE
#include "malloc_wrapper.h"
#endif /* USERSPACE */

extern void delay(int ms);

inline int map(int x, int in_min, int in_max, int out_min, int out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
