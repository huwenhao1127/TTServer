
#include "BaseClient.h"

int main()
{
    BaseClient oClient(10001);
    oClient.Init();
    oClient.Connect("127.0.0.1", UDP_ADDR_PORT);
    return 0;
}