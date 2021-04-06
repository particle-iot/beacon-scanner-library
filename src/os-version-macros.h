#ifndef OS_VERSION_MACROS
#define OS_VERSION_MACROS

#include "Particle.h"

#if SYSTEM_VERSION >= SYSTEM_VERSION_ALPHA(3, 0, 0, 0)
#define ADDRESS(p) p->address()
#define ADVERTISING_DATA(p) p->advertisingData()
#define RSSI(p) p->rssi()
#else
#define ADDRESS(p) p->address
#define ADVERTISING_DATA(p) p->advertisingData
#define RSSI(p) p->rssi
#endif

#endif