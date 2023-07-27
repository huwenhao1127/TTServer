#include "RingQueue.h"
#include <iostream>

int main()
{
    RingQueue oQueue(50);
    oQueue.Debug();

    char szData1[58] = "test hello world...";

    for (int i = 0; i < 100; i++)
    {
        oQueue.Push(szData1, sizeof(szData1));
        oQueue.Debug();

        char* pOut = (char*)malloc(100);
        oQueue.Pop(pOut);
        oQueue.Debug();
        std::cout << (const char*)pOut << std::endl;
        free(pOut);
    }
    return 0;
}