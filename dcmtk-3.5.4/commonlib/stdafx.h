// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头中排除极少使用的资料



// TODO: 在此处引用程序需要的其他头文件
#include <windows.h>

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // 某些 CString 构造函数将是显式的

#include <atlbase.h>
#include <atlstr.h>

#include <locale>
#include <string>
#include <iterator>
#include <list>
#include <set>
#include <map>
#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>

#include <signal.h>
#include <direct.h>        /* for _mkdir() */
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
