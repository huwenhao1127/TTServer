#pragma once
#include "Singleton.h"
#include "NetConnect.h"
#include "Socket.h"
#include <unordered_map>
#include <unordered_set>

typedef std::unordered_map<uint64_t, NetConnect*>  NetConnMap;           // 网络连接map(key:client addr, value:连接对象)
typedef std::unordered_map<uint32_t, NetConnect*>  NetClosedConnMap;     // 待回收的连接
typedef std::unordered_set<uint32_t> NetConnIDSet;                       // 已分配的id

class NetConnectMgr : public Singleton<NetConnectMgr>
{
public:
    /**
     * @brief 初始化
     * 
     * @return int 
     */
    int Init();

    /**
     * @brief tick1s
     * 
     */
    void Tick1S();

    /**
     * @brief 主循环
     * 
     * @return int 
     */
    int Proc();

    /**
     * @brief 将连接移动到待回收map
     * 
     * @param poConn 
     */
    void MoveToClosedMap(NetConnect *poConn);

    /**
     * @brief 获取链接对象
     * 
     * @param ullConnID 
     * @return NetConnect* 
     */
    NetConnect* GetConnByID(uint32_t ullConnID);
    NetConnect* GetConnByAddr(const sockaddr_in& stAddr);

    /**
     * @brief 设置/更新链接索引
     * 
     * @param poConn     链接对象
     * @param pstOldAddr 旧地址
     * @return int 
     */
    int SetConnAddr(NetConnect *poConn, const sockaddr_in *pstOldAddr = nullptr);

    /**
     * @brief 创建新链接对象
     * 
     * @param poNetWork 
     * @param stClientAddr 
     * @param szCookie 
     * @param stEncyptData 
     * @return NetConnect* 
     */
    NetConnect* CreateNewConn(NetWork *poNetWork, const sockaddr_in& stClientAddr, const char *szCookie, const STEncyptData& stEncyptData);
private:
    /**
     * @brief 处理待删除的连接
     * 
     */
    void ProcClosedConn();

    /**
     * @brief 回收链接对象
     * 
     * @param poConn 
     * @return int 
     */
    int FreeConn(NetConnect* poConn);

    /**
     * @brief 分配连接ID
     * 
     * @param ulID [out] 分配的ID
     * @return -1 分配失败 
     */
    int AllocID(uint32_t& ulID);

private:
    NetConnMap          m_mapConn;              // 连接map
    NetClosedConnMap    m_mapClosedConn;        // 已关闭待回收的连接
    NetConnIDSet        m_setExistID;           // 已经使用的ID
    uint32_t            m_ulCurID;              // 当前ID
    time_t              m_tLastUpdate;          // 上次update时间ms
};
