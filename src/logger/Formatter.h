/*
 * @Author: huwenhao 1398744100@qq.com
 * @Date: 2023-02-04 17:24:35
 * @LastEditors: huwenhao 1398744100@qq.com
 * @LastEditTime: 2023-02-05 12:06:07
 * @FilePath: /TTServer/src/Logger/Formatter.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */

#pragma once
#include "LoggerCommDef.h" 
#include "Singleton.h"

namespace tts {

class BaseAppender;

class Formatter : public Singleton<Formatter>
{
public:
    typedef std::shared_ptr<Formatter> ptr;
    Formatter() {}
    ~Formatter() {}

public:
    const std::string& Format(const STLogRecord& stRecord);

private:
   std::string m_sFormatter; 
   BaseAppender::ptr m_ptrAppender;     // 所属的Appender
};

}