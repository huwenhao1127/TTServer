#include "NetConnect.h"

NetConnect::NetConnect() : m_oKcp(*this)
{

}

NetConnect::~NetConnect() {}

int NetConnect::Send2NetWork(const char* szData, int iLen, bool bReliable)
{
    return 0;
}