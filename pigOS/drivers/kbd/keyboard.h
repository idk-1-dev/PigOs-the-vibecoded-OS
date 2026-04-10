#pragma once
// keyboard.h - uses combined ps2.h
#include "../ps2/ps2.h"
// kb_avail, kb_get, kb_push, KEY_* all defined in ps2.h
static inline void kb_poll(){ ps2_poll(); }
