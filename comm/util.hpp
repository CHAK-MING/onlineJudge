#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <atomic>
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <algorithm>
#include <string_view>

namespace ns_util
{
    const std::string temp_path = "./temp/";

    class StringUtil
	{
    public:
		// str: 要切分的字符串
		// target: 切分的结果
		// sep: 切分的分隔符
		static void SplitString(const std::string &str, std::vector<std::string> *target, const std::string &sep)
		{
			size_t start = 0;
			size_t end = str.find(sep);

			while (end != std::string::npos)
			{
				target->push_back(str.substr(start, end - start));
				start = end + sep.size();
				end = str.find(sep, start);
			}

			if (start < str.size())
			{
				target->push_back(str.substr(start));
			}
		}
	};

    class PathUtil
    {
    public:
        static std::string AddSuffix(const std::string &file_name, const std::string &suffix)
        {
            std::string path_name = temp_path;
            path_name += file_name;
            path_name += suffix;
            return path_name;
        }

        // 编译时需要有的临时文件
        // 构建源文件路径+后缀的完整文件名
        static std::string Src(const std::string &file_name)
        {
            return AddSuffix(file_name, ".cpp");
        }

        // 构建可执行程序的完整路径+后缀名
        static std::string Exe(const std::string &file_name)
        {
            return AddSuffix(file_name, ".exe");
        }

        // 编译时报错
        static std::string CompilerError(const std::string &file_name)
        {
            return AddSuffix(file_name, ".compile_error");
        }

        // 运行时需要有的临时文件
        // 标准输入
        static std::string Stdin(const std::string &file_name)
        {
            return AddSuffix(file_name, ".stdin");
        }
        // 标准输出
        static std::string Stdout(const std::string &file_name)
        {
            return AddSuffix(file_name, ".stdout");
        }
        // 构建该程序对于的错误文件完整路径+后缀名
        static std::string Stderr(const std::string &file_name)
        {
            return AddSuffix(file_name, ".stderr");
        }
    };

    class TimeUtil
    {
    public:
        // 获得秒时间戳
        static std::string GetTimeStamp()
        {
            timeval _time;
            gettimeofday(&_time, nullptr);
            return std::to_string(_time.tv_sec);
        }
        // 获得毫秒时间戳
        static std::string GetTimeMs()
        {
            timeval _time;
            gettimeofday(&_time, nullptr);
            return std::to_string(_time.tv_sec * 1000 + _time.tv_usec / 1000);
        }
    };

    class FileUtil
    {
    public:
        static bool IsFileExists(const std::string &path_name)
        {
            struct stat st;
            return (stat(path_name.c_str(), &st) == 0);
        }

        static std::string UniqFileName()
        {
            // 毫秒级时间戳+原子性递增唯一值
            static std::atomic_uint id(0);
            id++;
            std::string ms = TimeUtil::GetTimeMs();
            std::string uniq_id = std::to_string(id);

            return ms + "_" + uniq_id;
        }

        static bool WriteFile(const std::string &target, const std::string &content)
        {
            std::ofstream out(target);
            if (!out)
            {
                return false;
            }
            out.write(content.c_str(), content.size());
            out.close();
            return true;
        }

        /**
         * target: 目标文件
         * content: 内容
         * keep: 是否在读取文件的每一行时保留\n
         */
        static bool ReadFile(const std::string &target, std::string *content, bool keep = false)
        {
            content->clear();
            std::ifstream in(target);
            if (!in)
            {
                return false;
            }
            std::string line;
            // getline不保存行分割符
            // getline重载了bool类型强转
            while (std::getline(in, line))
            {
                (*content) += line;
                (*content) += (keep ? "\n" : "");
            }
            in.close();
            return true;
        }
    };

    

} // namespace ns_util
