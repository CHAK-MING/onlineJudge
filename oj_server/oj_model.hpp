#pragma once

#include "../comm/log.hpp"
#include "../comm/util.hpp"

#include <iostream>
#include <string>
#include <unordered_map>
#include <cassert>
#include <vector>

// 根据题目list文件，加载所有的题目信息到内存
// model: 主要用来和数据进行交互，对外提供和数据交互的接口
namespace ns_model
{
    using namespace ns_log;
    using namespace ns_util;

    struct Question
    {
        std::string number; // 题目的编号
        std::string title;  // 题目的标题
        std::string star;   // 题目的难度
        int cpu_limit;      // 题目的时间要求(s)
        int mem_limit;      // 题目的空间要求(kb)
        std::string desc;   // 题目的描述
        std::string header; // 题目预设给用户在线编辑器的代码
        std::string tail;   // 题目的测试用例
    };

    const std::string questions_list = "./questions/questions.list";
    const std::string questions_path = "./questions/";

    class Model
    {
    public:
        Model() 
        {
            assert(LoadQuestionList(questions_list));
        }
        ~Model() {}

        bool LoadQuestionList(const std::string& questions_list)
        {
            // 加载配置文件 questions/questions.list + 题目文件
            std::ifstream in(questions_list);
            if(!in.is_open())
            {
                LOG(FATAL) << " 加载题库失败，请检查是否存在题库文件" << "\n";
                return false;
            }
            std::string line;
            while(getline(in, line))
            {   
                std::vector<std::string> tokens;
                StringUtil::SplitString(line, &tokens, " ");
                if(tokens.size() != 5)
                {
                    LOG(WARNING) << "加载部分题目失败，请检查文件格式" << "\n";
                    continue;
                }
                Question q;
                q.number = tokens[0];
                q.title = tokens[1];
                q.star = tokens[2];
                q.cpu_limit = atoi(tokens[3].c_str());
                q.mem_limit = atoi(tokens[4].c_str());

                std::string path = questions_path;
                path += q.number + "/";

                FileUtil::ReadFile(path + "desc.txt",&(q.desc), true);
                FileUtil::ReadFile(path + "header.cpp", &(q.header), true);
                FileUtil::ReadFile(path + "tail.cpp", &(q.tail), true);

                questions.insert({q.number, q});
            }
            LOG(INFO) << "加载题库...成功" << "\n";
            in.close();
            return true;
        }

        bool GetAllQuestions(std::vector<Question>* out)
        {
            if(questions.size() == 0)
            {
                LOG(ERROR) << "用户获取题目失败！" << "\n"; 
                return false;
            }
            for(const auto& q : questions)
            {
                out->push_back(q.second);
            }
            return true;
        }

        bool GetOneQuestion(const std::string& number, Question* q)
        {
            const auto& iter = questions.find(number);
            if(iter == questions.end())
            {
                LOG(ERROR) << "用户获取题目失败，题目编号：" << number << "\n"; 
                return false;
            }
            (*q) = iter->second;
            return true;
        }

    private:
        std::unordered_map<std::string, Question> questions;
    };
}