#include <iostream>
#include <signal.h>
#include "../comm/httplib.h"
#include "oj_control.hpp"

using namespace httplib;
using namespace ns_control;

static Control* ctrl_ptr = nullptr;

void Recovery(int sign)
{
    ctrl_ptr->RecoveryMachine();
}

int main()
{
    signal(SIGQUIT, Recovery);
    // 用户请求的服务路由功能
    Server svr;
    Control ctrl;
    ctrl_ptr = &ctrl;
    
    // 获取所有的题目列表
    svr.Get("/all_questions",[&ctrl](const Request& req,Response& resp)
    {
        // 返回一张包含所有题目的html网页
        std::string html;
        ctrl.AllQuestions(&html);
        resp.set_content(html,"text/html; charset=utf-8");
    });

    // 用户要根据题目编号获取题目的内容
    // \d+正则表达式，匹配一个或多个位的数字
    svr.Get(R"(/question/(\d+))",[&ctrl](const Request& req,Response& resp)
    {
        std::string number = req.matches[1];
        std::string html;
        ctrl.SingleQuestion(number, &html);
        resp.set_content(html, "text/html; charset=utf-8");
    });

    // 用户提交代码，使用我们的判题功能(1.每道题的测试用例 2.compile_and_run)
    svr.Post(R"(/judge/(\d+))",[&ctrl](const Request& req,Response& resp)
    {
        std::string number = req.matches[1];
        std::string result_json;
        ctrl.Judge(number, req.body, &result_json);
        resp.set_content(result_json, "application/json; charset=utf-8");
    });
    

    svr.set_base_dir("./wwwroot");
    svr.listen("0.0.0.0", 8080);

    return 0;
}
