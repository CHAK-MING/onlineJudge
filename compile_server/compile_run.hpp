#pragma once

#include "compile.hpp"
#include "runner.hpp"
#include "../comm/util.hpp"
#include "../comm/log.hpp"
#include <jsoncpp/json/json.h>
#include <signal.h>

namespace ns_compile_and_run
{
    using namespace ns_log;
    using namespace ns_util;
    using namespace ns_compiler;
    using namespace ns_runner;
    class CompileAndRun
    {
    public:
        // code > 0 : 收到信号导致异常
        // code < 0 : 非运行报错
        // code == 0 : 运行正常
        // 待完善
        static std::string CodeToDesc(int code, const std::string &file_name)
        {
            std::string desc;
            switch (code)
            {
            case 0:
                desc = "编译运行成功";
                break;
            case -1:
                desc = "提交的代码为空";
                break;
            case -2:
                desc = "未知错误";
                break;
            case -3:
                FileUtil::ReadFile(PathUtil::CompilerError(file_name), &desc, true);
                break;
            case SIGABRT: // 6
                desc = "内存超过范围";
                break;
            case SIGXCPU: // 24
                desc = "CPU使用超时";
                break;
            case SIGFPE: // 8
                desc = "浮点数溢出";
                break;
            default:
                desc = "未知: " + std::to_string(code);
                break;
            }
            return desc;
        }

        static void RemoveTempFile(const std::string& file_name)
        {
            std::string _src = PathUtil::Src(file_name);
            if(FileUtil::IsFileExists(_src)) unlink(_src.c_str());

            std::string _compiler_error = PathUtil::CompilerError(file_name);
            if(FileUtil::IsFileExists(_compiler_error)) unlink(_compiler_error.c_str());

            std::string _execute = PathUtil::Exe(file_name);
            if(FileUtil::IsFileExists(_execute)) unlink(_execute.c_str());

            std::string _stdin = PathUtil::Stdin(file_name);
            if(FileUtil::IsFileExists(_stdin)) unlink(_stdin.c_str());

            std::string _stdout = PathUtil::Stdout(file_name);
            if(FileUtil::IsFileExists(_stdout)) unlink(_stdout.c_str());
            
            std::string _stderr = PathUtil::Stderr(file_name);
            if(FileUtil::IsFileExists(_stderr)) unlink(_stderr.c_str());
        }

        /***
         * 输入：
         * code：用户提交的代码
         * input: 用户给自己提交的代码对应的输入 不做处理
         * cpu_limit: 代码的CPU时间限制
         * mem_limit: 代码的内存限制
         *
         * 输出：
         * 必填
         * status：状态码
         * reason：请求结果
         * 选填
         * stdout：我的程序运行完的结果
         * stderr：我的程序运行完的错误结果
         *
         */
        static void Start(const std::string &in_json, std::string *out_json)
        {
            Json::Value in_value;
            Json::Reader reader;
            reader.parse(in_json, in_value);

            std::string code = in_value["code"].asString();
            std::string input = in_value["input"].asString();
            int cpu_limit = in_value["cpu_limit"].asInt();
            int mem_limit = in_value["mem_limit"].asInt();

            Json::Value out_value;
            int status_code = 0;
            int run_result = 0;

            // 毫秒级时间戳+原子性递增唯一值
            std::string file_name = FileUtil::UniqFileName(); // 获取具有唯一性的名称

            if (code.size() == 0)
            {
                status_code = -1; // 代码为空
                goto END;
            }

            if (!FileUtil::WriteFile(PathUtil::Src(file_name), code)) // 形成临时src文件
            {
                status_code = -2; // 未知错误
                goto END;
            }

            if (!Compiler::Compile(file_name))
            {
                status_code = -3; // 编译失败
                goto END;
            }

            run_result = Runner::Run(file_name, cpu_limit, mem_limit);
            if (run_result < 0)
            {
                status_code = -2; // 未知错误
            }
            else if (run_result == 0)
            {
                // 运行成功
                status_code = 0;
            }
            else
            {
                status_code = run_result; // 运行出错
            }
        END:
            out_value["status"] = status_code;
            out_value["reason"] = CodeToDesc(status_code, file_name);
            if (status_code == 0)
            {
                // 整个过程全部成功
                std::string _stdout;
                std::string _stderr;
                FileUtil::ReadFile(PathUtil::Stdout(file_name), &_stdout, true);
                FileUtil::ReadFile(PathUtil::Stderr(file_name), &_stderr, true);
                out_value["stdout"] = _stdout;
                out_value["stderr"] = _stderr;
            }
            // 序列化过程
            Json::StyledWriter writer;
            *out_json = writer.write(out_value);

            // 清理所有的临时文件
            RemoveTempFile(file_name);
        }
    };
}