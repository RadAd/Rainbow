// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define NOMINMAX

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <strsafe.h>

#pragma warning(push)
#pragma warning(disable: 4995)
#include <algorithm>
#include <map>
#include <vector>
#include <regex>
#include <fstream>
#include <sstream>
#include <locale>
#pragma warning(pop)

void DisplayAboutMessage(HINSTANCE hInstance);

