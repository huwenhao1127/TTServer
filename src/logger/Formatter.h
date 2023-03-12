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
#include <iostream>
#include <functional>
#include <map>
#include <vector>

namespace tts {

class BaseAppender;
class Logger;

#define DEFAULT_FORMATTER_PATTERN "[%d]|%L|%f:%l|%U|%t|%F|%m %n"

/**
 * @brief 格式化日志item
 * 
 * @param[out] osFmtItem   item输出流
 * @param[in] ptrLogger    所属日志器
 * @param[in] stRecord     日志记录
 * @param[in] sFormat      格式
 */
typedef std::function<void (std::ostream& osFmtItem, EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord, const std::string& sFormat)> ItemFmtFunc;

class Formatter
{
public:
    /**
     * sPattern模式说明：
     *  %m 消息
     *  %L 日志级别
     *  %c 日志名称
     *  %t 线程id
     *  %n 换行
     *  %d 时间 默认格式[年-月-日 小时:分钟:秒.毫秒]. 也支持传入strftime格式, 例如%d{%Y-%M-%D}
     *  %f 文件名
     *  %l 行号
     *  %T 制表符
     *  %F 协程id
     *  %N 线程名称
     *  %U 函数名
     * 
     *  {}内的内容为模式附加的字符串
     *  其它字符 自定义字符串对应%s
     * 
     *  默认格式 "[%d]|%L|%f:%l|%U|%t|%F| %m %n"
     **/
    Formatter(const std::string& sPattern = DEFAULT_FORMATTER_PATTERN)
        : m_sFormatter(sPattern)
    {
        Init();
    }
    ~Formatter() {}

    void Init();

public:
    /**
     * @brief 对日志记录格式化，输出字符串
     */
    std::string Format(EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord);

    /**
     * @brief 获取日志格式
     */
    const std::string& getPattern() const { return m_sFormatter;}

public:
    /**
     * @brief 日志内容
     */
    static void MessageItemFormat(
        std::ostream& osFmtItem, EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord, const std::string& sFormat = "");

    /**
     * @brief 日志级别
     */
    static void LevelItemFormat(
        std::ostream& osFmtItem, EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord, const std::string& sFormat = "");

    /**
     * @brief 日志名称
     */
    static void NameItemFormat(
        std::ostream& osFmtItem, EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord, const std::string& sFormat = "");

    /**
     * @brief 线程id
     */
    static void ThreadIdItemFormat(
        std::ostream& osFmtItem, EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord, const std::string& sFormat = "");

    /**
     * @brief 换行
     */
    static void NewLineItemFormat(
        std::ostream& osFmtItem, EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord, const std::string& sFormat  = "");

    /**
     * @brief 日期
     */
    static void DateTimeItemFormat(
        std::ostream& osFmtItem, EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord, const std::string& sFormat = "");

    /**
     * @brief 文件名
     */
    static void FilenameItemFormat(
        std::ostream& osFmtItem, EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord, const std::string& sFormat = "");

    /**
     * @brief 行号
     */
    static void LineNumItemFormat(
        std::ostream& osFmtItem, EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord, const std::string& sFormat = "");

    /**
     * @brief 制表符
     */
    static void TabItemFormat(
        std::ostream& osFmtItem, EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord, const std::string& sFormat = "");

    /**
     * @brief 协程id
     */
    static void FiberIdItemFormat(
        std::ostream& osFmtItem, EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord, const std::string& sFormat = "");

    /**
     * @brief 线程名
     */
    static void ThreadNameItemFormat(
        std::ostream& osFmtItem, EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord, const std::string& sFormat  = "");

    /**
     * @brief 函数名
     */
    static void FuncNameItemFormat(
        std::ostream& osFmtItem, EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord, const std::string& sFormat = "");
    
    /**
     * @brief 自定义字符串
     */
    static void StringItemFormat(
        std::ostream& osFmtItem, EnmLoggerLevel eLevel, Logger& stLogger, const STLogRecord& stRecord, const std::string& sFormat = "");

    /**
     * @brief 日志级别转字符串
     */
    static const char* LevelToString(EnmLoggerLevel eLevel);

private:
    /**
     * @brief 绑定模式与对应的fmt函数
     */
    void BindItemAndFmtFunc();

    /**
     * @brief 将Formatter字符串解析为模式
     */
    void ParseFormatter();
private:
    std::string m_sFormatter;         // 输出格式
    bool m_bError;                    // pattern错误
    std::map<char, ItemFmtFunc> m_mapItemFmt;   // 所有Item的Fmt函数
    std::vector<std::tuple<char, std::string>> m_vecItems;  // Formatter对应所有items, 0:模式，1:模式附加的字符串
};

}