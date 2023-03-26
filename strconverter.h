#pragma once
#if 1	// MFC
#include "stdafx.h"
#else	// WIN32
#include	<windows.h>
#endif

TCHAR* convert_from_utf8(const char* utf8_str);
char* convert_to_utf8(const TCHAR* t_str);
