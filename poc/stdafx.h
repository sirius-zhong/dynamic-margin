#pragma once

#ifdef _WIN32
#ifndef WINVER
#define WINVER _WIN32_WINNT_WIN7
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WIN7
#endif
#ifndef _WIN32_WINDOWS
#define _WIN32_WINDOWS _WIN32_WINNT_WIN7
#endif
#ifndef _WIN32_IE
#define _WIN32_IE _WIN32_IE_IE90
#endif
#ifndef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_WIN7
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#include <limits.h>

#include "mt5_api_poc_compat.h"
