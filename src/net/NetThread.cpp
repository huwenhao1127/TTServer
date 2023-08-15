#include "NetThread.h"
#include "Now.h"
#include "NetMgr.h"

int NetThread::Init()
{
    m_dwIdleSleepTm = 1000;
    return 0;
}

int NetThread::Proc()
{
    NetMgr::Inst().Proc();
    Now::Update();
    return 0;
}

int NetThread::Tick()
{
    return 0;
}

int NetThread::Tick1S()
{
    return NetMgr::Inst().Tick1S();
}

int NetThread::Tick20S()
{
    return NetMgr::Inst().Tick20S();
}