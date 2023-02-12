/*
 * @Author: huwenhao 1398744100@qq.com
 * @Date: 2023-02-04 21:27:56
 * @LastEditors: huwenhao 1398744100@qq.com
 * @LastEditTime: 2023-02-04 23:01:05
 * @FilePath: /TTServer/src/base/CommDef.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once
#include <memory.h>
#include <string>
#include <stdint.h>
#include <list>
#include <map>
#include <sstream>
#include <fstream>

#define ZeroStruct(stDst) memset(&stDst, 0, sizeof(stDst));