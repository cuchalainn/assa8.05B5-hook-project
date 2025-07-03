// 檔案: pch.h
#ifndef PCH_H
#define PCH_H

#include "framework.h"

// 放入所有專案都會用到的標頭檔
// 【修正】補全所有需要的標準函式庫引用
#include <windows.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <map>              // For std::map
#include <functional>       // For std::function
#include <psapi.h>
#include <time.h>
#include <cstdio>
#include <algorithm>
#include <stdexcept>
#include <random>           // For std::mt19937
#include <cctype>
#include <cmath>
#include <sstream>          // For std::wstringstream
#include <fstream>          // For std::ifstream, std::ofstream
#include <iostream>         // For std::endl

#endif //PCH_H