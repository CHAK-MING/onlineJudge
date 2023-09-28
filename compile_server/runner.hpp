#pragma once

#include "../comm/util.hpp"
#include "../comm/log.hpp"

#include <iostream>
#include <string>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <fcntl.h>

namespace ns_runner
{
    using namespace ns_util;
    using namespace ns_log;

    class Runner
    {
    public:
        Runner() {}
        ~Runner() {}

        // 提供设置进程占用资源大小的接口
        static void SetProcLimit(int cpu_limit, int mem_limit)
        {
            rlimit cpu_rlimit;
            rlimit mem_rlimit;
            
            cpu_rlimit.rlim_cur = cpu_limit;
            cpu_rlimit.rlim_max = RLIM_INFINITY;
            mem_rlimit.rlim_cur = mem_limit * 1024; // 转换为kb
            mem_rlimit.rlim_max = RLIM_INFINITY;

            setrlimit(RLIMIT_CPU, &cpu_rlimit);
            setrlimit(RLIMIT_AS, &mem_rlimit);
        }

        /**
         * 返回值 > 0：程序异常，退出时收到了信号，返回值就是对应的信号编号
         * 返回值 == 0： 正常运行完毕的，结果保存到了对应的临时文件
         * 返回值 < 0：内部错误
         * 
         * cpu_limit: 该程序运行时，可以使用最大的CPU资源上限
         * mem_limit: 内存限制
        */

        // 指明文件名即可，不需要带路径和带后缀
        static int Run(const std::string &file_name, int cpu_limit, int mem_limit)
        {
            /**************************
             * 程序运行：
             * 1. 代码跑完，结果正确
             * 2. 代码跑完，结果不正确
             * 3. 代码没跑完，出现异常
             * 我们只考虑是否正确运行完毕
             */
            std::string _execute = PathUtil::Exe(file_name);
            std::string _stdin = PathUtil::Stdin(file_name);
            std::string _stdout = PathUtil::Stdout(file_name);
            std::string _stderr = PathUtil::Stderr(file_name);

            umask(0);
            int _stdin_fd = open(_stdin.c_str(), O_CREAT | O_RDONLY, 0644);
            int _stdout_fd = open(_stdout.c_str(), O_CREAT | O_WRONLY, 0644);
            int _stderr_fd = open(_stderr.c_str(), O_CREAT | O_WRONLY, 0644);

            if(_stdin_fd < 0 || _stdout_fd < 0 || _stderr_fd < 0)
            {
                LOG(ERROR) << "运行时打开标准文件失败" << "\n";
                return -1; // 代表打开文件失败
            }


            pid_t pid = fork();
            if (pid < 0)
            {
                LOG(ERROR) << "运行时创建子进程失败" << "\n";
                close(_stdin_fd);
                close(_stdout_fd);
                close(_stderr_fd);
                return -2; // 代表创建子进程失败
            }
            else if (pid == 0)
            {
                dup2(_stdin_fd,0);
                dup2(_stdout_fd,1);
                dup2(_stderr_fd,2);

                SetProcLimit(cpu_limit, mem_limit); // 设置资源限制

                execl(_execute.c_str(),_execute.c_str(),nullptr);
                exit(1);
            }   
            else
            {
                close(_stdin_fd);
                close(_stdout_fd);
                close(_stderr_fd);
                int status = 0;
                waitpid(pid, &status, 0);
                LOG(INFO) << "运行完毕，info: " << (status & 0x7F) << "\n";
                // 程序运行异常，一定是因为收到了信号
                return status & 0x7F;
            }
        }
    };
}