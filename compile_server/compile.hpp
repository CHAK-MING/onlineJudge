#pragma once

#include "../comm/util.hpp"
#include "../comm/log.hpp"

#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

namespace ns_compiler
{
    // 引入对应的命名空间
    using namespace ns_util;
    using namespace ns_log;

    class Compiler
    {
    public:
        Compiler()
        {}
        ~Compiler()
        {}

        // 返回值：编译成功(true)、编译失败(false)
        // 输入参数：编译的文件名
        // file_name: 1234
        // 1234 -> ./temp/1234.cpp 源文件
        // 1234 -> ./temp/1234.exe 可执行文件
        // 1234 -> ./temp/1234.stderr 错误文件
        static bool Compile(const std::string& file_name)
        {
            pid_t pid = fork();
            if(pid < 0)
            {
                LOG(ERROR) << "内部错误，创建子进程失败" << "\n";
                return false;
            }
            else if(pid == 0)
            {
                umask(0);
                int _stderr = open(PathUtil::CompilerError(file_name).c_str(), O_CREAT | O_WRONLY, 0644);
                if(_stderr < 0)
                {
                    LOG(WARNING) << "没有成功形成compile_error文件" << "\n";
                    exit(1);
                }
                // 重定向标准错误到_stderr
                dup2(_stderr, 2);

                // 子进程：调用编译器，完成对代码的编译工作
                execlp("g++", "g++", "-o", 
                    PathUtil::Exe(file_name).c_str(),PathUtil::Src(file_name).c_str(), // 目标文件 源文件
                    "-D", "COMPILER_ONLINE", // 去掉测试用例的头部内容
                    "-std=c++11", nullptr); // 编译版本
                LOG(ERROR) << "启动编译器g++失败，可能是参数错误" << "\n";
                exit(2);
            }
            else
            {
                // 父进程
                waitpid(pid, nullptr, 0);
                // 编译是否成功
                // 判断一个文件是否存在
                if(FileUtil::IsFileExists(PathUtil::Exe(file_name)))
                {
                    LOG(INFO) << PathUtil::Src(file_name) << " 编译成功!" << "\n";
                    return true;
                }
            }
            LOG(ERROR) << "编译失败，没有形成可执行程序" << "\n";
            return false;
        }

    };
}