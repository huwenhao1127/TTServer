#include "RingQueue.h"
#include "BaseThread.h"
#include <iostream>
#include <random>
#include <mutex>

static RingQueue oQueue(50);
std::mutex mtx;

class ProducerThread : public BaseThread
{
public:
    ProducerThread(const char* szName) : BaseThread(szName) {}
    virtual ~ProducerThread() {}
private:
     int Tick()
    {
        int iRand = std::rand();
        oQueue.Push((char*)&iRand, sizeof(iRand));
        std::cout << "producer: " << iRand << std::endl;
        return 0;
    }
};

class ComsumerThread : public BaseThread
{
public:
    ComsumerThread(const char* szName)  : BaseThread(szName) {}
    virtual ~ComsumerThread() {}
private:
    int Tick1S()
    {
        char szRes[4];
        oQueue.Pop(szRes);
        std::cout << "consumer: " << (*(int*)szRes) << std::endl;
        return 0;
    }
};

int main()
{
    ProducerThread tPorducer("producer");
    tPorducer.Start();
    ComsumerThread tComsumer("comsumer");
    tComsumer.Start();
    tPorducer.Join();
    tComsumer.Join();
    return 0;
}