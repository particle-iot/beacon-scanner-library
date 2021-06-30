/*
 * Copyright (c) 2020 Particle Industries, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef OS_VERSION_MACROS
#define OS_VERSION_MACROS

#include "Particle.h"

#if SYSTEM_VERSION >= SYSTEM_VERSION_ALPHA(3, 0, 0, 0)
#define ADDRESS(p) p->address()
#define ADVERTISING_DATA(p) p->advertisingData()
#define SCAN_RESPONSE(p) p->scanResponse()
#define RSSI(p) p->rssi()
#else
#define ADDRESS(p) p->address
#define ADVERTISING_DATA(p) p->advertisingData
#define SCAN_RESPONSE(p) p->scanResponse
#define RSSI(p) p->rssi
#endif

#endif