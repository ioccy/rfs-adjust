// header.h : include file for standard system include files,
// or project specific include files
//

#pragma once

#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma comment(lib, "comctl32.lib")

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <stdio.h>
#include <windowsx.h>

#include <commctrl.h>  // for NM_CLICK, NM_RETURN, NMLINK
#include <shellapi.h>  // for ShellExecuteW

#include <string>
#include <stdexcept>
#include <cmath>