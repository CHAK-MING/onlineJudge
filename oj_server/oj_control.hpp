#pragma once

#include <iostream>
#include <string>
#include <algorithm>
#include <cstdint>
#include <mutex>
#include <cassert>
#include <fstream>

#include <jsoncpp/json/json.h>

#include "../comm/httplib.h"
#include "oj_model.hpp"
#include "oj_view.hpp"

namespace ns_control
{
    using namespace ns_log;
    using namespace ns_util;
    using namespace ns_model;
    using namespace ns_view;
    using namespace httplib;

    const std::string service_machine = "./conf/service_machine.conf";

    // 提供服务的主机
    struct Machine
    {
        Machine() 
            :ip(""), port(0), load(0), mtx(nullptr)
        {}
        ~Machine()
        {}

        // 提升主机负载
        void IncLoad()
        {
            if(mtx) mtx->lock();
            ++load;
            if(mtx) mtx->unlock();
        }

        // 减少主机负载
        void DecLoad()
        {
            if(mtx) mtx->lock();
            --load;
            if(mtx) mtx->unlock();
        }

        // 重置负载
        void RestLoad()
        {
            if(mtx) mtx->lock();
            load = 0;
            if(mtx) mtx->unlock();
        }

        // 获取主机负载
        uint64_t Load()
        {
            uint64_t _load = 0;
            if (mtx) mtx->lock();
            _load = load;
            if (mtx) mtx->unlock();
            return _load;
        }

        std::string ip;  // 编译服务的ip
        int16_t port;    // 编译服务的端口
        int64_t load;    // 编译服务的负载
        std::mutex *mtx; // mutex禁止拷贝的，使用指针
    };

    // 负载均衡模块
    class LoadBlance
    {
    public:
        LoadBlance()
        {
            assert(LoadConf(service_machine));
            LOG(INFO) << "加载 " << service_machine << " 成功! " << "\n";
        }

        ~LoadBlance() = default;

        bool LoadConf(const std::string& machine_conf)
        {
            std::ifstream in(machine_conf);
            if(!in.is_open())
            {
                LOG(FATAL) << "加载: " << machine_conf << " 失败! " << "\n";
                return false;
            }
            std::string line;
            while(std::getline(in, line))
            {
                std::vector<std::string> tokens;
                StringUtil::SplitString(line, &tokens, ":");
                if(tokens.size() != 2)
                {
                    LOG(WARNING) << " 切分 " << line << "失败！" << "\n";
                    continue;
                }
                Machine m;
                m.ip = tokens[0];
                m.port = stoi(tokens[1]);
                m.load = 0;
                m.mtx = new std::mutex();

                online.push_back(machines.size());
                machines.push_back(m);
            }

            in.close();
            return true;
        }

        bool SmartChoice(int *id, Machine** m)
        {
            // 1. 使用选择好的主机(更新主机的负载)
            // 2. 需要选择可能离线该主机
            mtx.lock();
            // 负载均衡算法
            // 1. 随机数法 + hash
            // 2. 轮询 + hash(首选)
            int online_num = online.size();
            if(online_num == 0)
            {
                mtx.unlock();
                LOG(FATAL) << " 所有的后端编译主机已经离线，请运维的同事尽快查看! " << "\n";
                return false;
            }
            // 找到负载最小的机器
            *id = online[0];
            *m = &machines[online[0]];
            uint64_t min_load = machines[online[0]].Load();
            for(int i = 1;i < online_num; ++i)
            {
                uint64_t curr_load = machines[online[i]].Load();
                if(min_load > curr_load)
                {
                    min_load = curr_load;
                    *id = online[i];
                    *m = &machines[online[i]];
                }
            }
            mtx.unlock();
            return true;
        }

        void OfflineMachine(int which)
        {
            mtx.lock();
            auto iter = find(online.begin(),online.end(),which);
            machines[which].RestLoad();
            online.erase(iter);
            offline.push_back(which);
            mtx.unlock();
        }
        
        void OnlineMachine()
        {
            // 统一上线
            mtx.lock();
            online.insert(online.end(),offline.begin(),offline.end());
            offline.clear();
            mtx.unlock();

            LOG(INFO) << "所有的主机又上线啦！" << "\n";
        }

        // for test
        void ShowMachines()
        {
            mtx.lock();
            std::cout << "当前在线主机列表: "; 
            for(auto& id : online)
            {
                std::cout << id << " ";
            }
            std::cout << std::endl;

            std::cout << "当前离线主机列表: "; 
            for(auto& id : offline)
            {
                std::cout << id << " ";
            }
            std::cout << std::endl;
            mtx.unlock();
        }

    private:
        std::vector<Machine> machines; // 可以给我们提供编译服务的主机
        std::vector<int> online;       // 所有在线主机的id
        std::vector<int> offline;      // 所有离线主机的id
        std::mutex mtx;                // 保证LoadBlance的数据安全
    };

    class Control
    {
    public:
        Control() {}
        ~Control() {}

        void RecoveryMachine()
        {
            load_blance.OnlineMachine();
        }

        // 根据题目数据构建网页
        // html: 输出的网页
        bool AllQuestions(std::string* html)
        {
            bool ret = true;
            std::vector<Question> all;
            if(model.GetAllQuestions(&all))
            {
                sort(all.begin(),all.end(),[](const Question& q1,const Question& q2)
                {
                    return std::stoi(q1.number) < std::stoi(q2.number);
                });
                // 获取题目信息成功，将所有的题目数据构建成 
                view.AllQuestionExpandHtml(all, html);
            }
            else
            {
                *html = "获取题目失败，形参题目列表失败";
                ret = false;
            }
            return ret;
        }

        bool SingleQuestion(const std::string& number, std::string* html)
        {
            bool ret = true;
            Question q;
            if(model.GetOneQuestion(number, &q))
            {
                // 获取指定题目信息成功，将题目数据构建成网页
                view.SingleQuestionExpandHtml(q, html);
            }
            else
            {
                *html = "指定题目: " + number + " 不存在";
                ret = false;
            }
            return ret;
        }


        // code:
        // input: 
        void Judge(const std::string& number, const std::string& in_json, std::string* out_json)
        {
            // 0. 根据题目编号，拿到对应题目的细节
            Question q;
            model.GetOneQuestion(number, &q);

            // 1. in_json反序列化，获得题目id和源代码还有input数据
            Json::Reader reader;
            Json::Value in_value;
            reader.parse(in_json, in_value);

            
            // 2. 重新拼接用户代码+测试用例代码，形成新的代码
            std::string code = in_value["code"].asString();
            Json::Value compile_value;
            compile_value["input"] = in_value["input"].asString();
            compile_value["code"] = code + q.tail; // 用户代码 + 测试用例代码
            compile_value["cpu_limit"] = q.cpu_limit;
            compile_value["mem_limit"] = q.mem_limit;
            Json::FastWriter writer;
            std::string compile_string = writer.write(compile_value);

            // 3. 选择负载最低的主机
            // 规则： 一直选择，直到主机可用，否则，就是全部挂掉
            while(true)
            {
                int id = 0;
                Machine* m = nullptr;
                if(!load_blance.SmartChoice(&id, &m))
                {
                    break;
                }
                LOG(INFO) << "选择主机成功 主机ID: " << id << "详情: " << m->ip << ":" << m->port << "\n";

                // 4. 然后发起http请求，得到结果
                Client cli(m->ip, m->port);
                m->IncLoad();
                if(auto res = cli.Post("/compile_and_run", compile_string, "application/json;charset=utf-8"))
                {
                    // 5. 将结果赋值给out_json
                    if(res->status == 200)
                    {
                        LOG(INFO) << "请求编译和运行服务成功..." << "\n";
                        *out_json = res->body;
                        break;
                    }
                    m->DecLoad();
                }
                else
                {
                    // 请求失败
                    LOG(ERROR) << "当前请求的主机挂掉 主机ID: " << id << "详情: " << m->ip << ":" << m->port << "\n";
                    load_blance.OfflineMachine(id);
                    load_blance.ShowMachines(); // 仅仅是为了调试
                }
            }
        }

    private:
        Model model;            // 提供与数据交互的model
        View view;              // 提供网页渲染功能
        LoadBlance load_blance; // 核心负载均衡器
    };
}