
#include "NetMgr.h"

int main()
{
    int iRet = NetMgr::Inst().Init();
    if (0 == iRet)
    {
        NetMgr::Inst().Start();
    }
    while (1)
    {
        usleep(1000);
    }
    return 0;
}