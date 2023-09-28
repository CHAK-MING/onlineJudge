#pragma once

#include "util.hpp"

#include <iostream>
#include <string>
#include <sstream>

namespace ns_log
{
    using namespace ns_util;
    
    // 日志等级
    enum
    {
        INFO,
        DEBUG,
        WARNING,
        ERROR,
        FATAL
    };

    inline std::ostream& Log(const std::string & level,const std::string& file_name, int line)
    {
        // 创建输出流
        static std::ostringstream oss;
        
        // 添加日志等级
        oss << "[" << level << "]";

        // 添加报错文件名称
        oss << "[" << file_name << "]";

        // 添加报错行
        oss << "[" << line << "]";

        // 添加日志时间戳
        oss << "[" << TimeUtil::GetTimeStamp() << "]";

        return oss;
    }

    inline void OutputLog(const std::ostringstream& oss)
    {
        std::string log = oss.str();
        std::cout << log << std::flush;

        std::ofstream log_file("log.txt", std::ios::app);
        if(log_file)
        {
            log_file << log << std::endl;
            log_file.close();
        }
    }

    #define LOG(level) Log(#level, __FILE__, __LINE__)
}