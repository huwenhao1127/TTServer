#include "BaseThread.h"
#include "Now.h"
#include "Logger.h"

void ThreadMainFunc(void* pCallBack)
{
    BaseThread* pThread = (BaseThread*)pCallBack;
    if (nullptr == pThread)
    {
        return;
    }

    prctl(PR_SET_NAME, pThread->m_szThreadName);
    pThread->m_bActive = true;
    pThread->m_oThreadID = std::this_thread::get_id();
    pThread->m_ullLastTickTime = Now::TimeStampMS();
    pThread->m_ullLastTickTime1S = Now::TimeStampMS();
    pThread->m_ullLastTickTime5S = Now::TimeStampMS();
    pThread->m_ullLastTickTime10S = Now::TimeStampMS();
    pThread->m_ullLastTickTime20S = Now::TimeStampMS();
    
    int iRet = pThread->Init();
    if (0 != iRet)
    {
        return;
    }

    pThread->Run();
}

BaseThread::BaseThread(const char* szName)
{
    m_szThreadName = szName;
    m_dwIdleSleepTm = BASE_THREAD_IDLE_SLEEP_TIME;
    m_dwTickInterval = BASE_THREAD_TICK_INTERVAL;
    m_bActive = false;
}

BaseThread::~BaseThread()
{
    if (nullptr != m_pThread)
    {
        Stop();
    }
}

int BaseThread::Start()
{
    m_pThread = new std::thread(ThreadMainFunc, this);
    if (nullptr == m_pThread)
    {
        return -1;
    }
    return 0;
}

int BaseThread::Stop()
{
    m_bActive = false;
    m_pThread->join();
    delete m_pThread;
    m_pThread = nullptr;
    return 0;
}

void BaseThread::Run()
{
    while (m_bActive)
    {
        Now::Update();
        
        int iProcCnt = Proc();

        if (Now::TimeStampMS() - m_ullLastTickTime > m_dwTickInterval)
        {
            Tick();
            m_ullLastTickTime = Now::TimeStampMS();
        }

        if (Now::TimeStampMS() - m_ullLastTickTime1S > 1000)
        {
            Tick1S();
            m_ullLastTickTime1S = Now::TimeStampMS();
        }

        if (Now::TimeStampMS() - m_ullLastTickTime5S > 5000)
        {
            Tick5S();
            m_ullLastTickTime5S = Now::TimeStampMS();
        }

        if (Now::TimeStampMS() - m_ullLastTickTime10S > 10000)
        {
            Tick10S();
            m_ullLastTickTime10S = Now::TimeStampMS();
        }

        if (Now::TimeStampMS() - m_ullLastTickTime20S > 20000)
        {
            Tick20S();
            m_ullLastTickTime20S = Now::TimeStampMS();
        }

        if (iProcCnt <= 0)
        {
            Idle();
        }
    }
}

