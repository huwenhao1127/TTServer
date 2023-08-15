/**
 * @file NetThread.h
 * @author huwenhao ()
 * @brief 网络线程
 * @date 2023-08-04
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#pragma once
#include "BaseThread.h"

class NetThread : public BaseThread
{
public:
    NetThread() : BaseThread("net thread") {}
    virtual ~NetThread() {}

    virtual int Init(); 

    virtual int Proc();

    virtual int Tick();

    virtual int Tick1S();

    virtual int Tick20S();
};