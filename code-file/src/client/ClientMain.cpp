#include "json.hpp"
#include <iostream>
#include <chrono>
#include <memory>
#include <vector>
#include <string>
#include <thread>
#include <ctime>
using namespace std;
using json = nlohmann::json;

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "public.hpp"
#include "user.hpp"
#include "group.hpp"
#include "usermodel.hpp"

//当前用户信息
User g_currentUser;
//当前用户朋友列表
vector<User> g_currentUserFriendList;
//当前用户群组列表信息
vector<Group> g_currentGroupUserList;
//是否登录在线
bool isLogin;
//显示当前用户数据信息
void showCurrentUserData();
//接受数据线程函数
void readTaskHandler(int clientfd);
//得到目前时间
string getCurrentTime();
//聊天主菜单
void mainMenu(int clientfd);

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cout << "Command invalid! example: ./ChatClient 127.0.0.1 6000" << endl;
        exit(-1);
    }
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        cerr << "Cocket create error" << endl;
        exit(-1);
    }

    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));
    //绑定server信息
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    { //连接服务器
        cerr << "Connect server error" << endl;
        exit(-1);
    }

    for (;;)
    {
        cout << "========================================================" << endl;
        cout << "1.Login" << endl;
        cout << "2.Register" << endl;
        cout << "3.Quit" << endl;
        cout << "========================================================" << endl;
        cout << "Choice:";
        int choice = 0;
        cin >> choice;
        cin.get();

        switch (choice)
        {
        case 1: //Login业务
        {
            int id = 0;
            char pwd[50] = {0};
            cout << "UserID:";
            cin >> id;
            cin.get();
            cout << "User Password:";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            string request = js.dump();
            //发送登录请求消息
            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "Send login msg error:" << request << endl;
            }
            else
            {
                char buffer[1024] = {0};
                //接受登录反馈信息
                len = recv(clientfd, buffer, 1024, 0);
                if (-1 == len)
                {
                    cerr << "Recv login response error" << endl;
                }
                else
                {
                    json responsejs = json::parse(buffer);
                    if (0 != responsejs["errno"].get<int>())
                    {
                        cerr << responsejs["errmsg"].get<string>() << endl;
                    }
                    else
                    {
                        g_currentUser.setId(responsejs["id"].get<int>());
                        g_currentUser.setNmae(responsejs["name"].get<string>());

                        if (responsejs.contains("friends"))
                        { //获取用户朋友信息
                            g_currentUserFriendList.clear();

                            vector<string> vec = responsejs["friends"];
                            for (string &str : vec)
                            {
                                json js = json::parse(str);
                                User user;
                                user.setId(js["id"].get<int>());
                                user.setNmae(js["name"].get<string>());
                                user.setState(js["state"].get<string>());
                                g_currentUserFriendList.push_back(user);
                            }
                        }

                        if (responsejs.contains("groups"))
                        { //获取用户群组信息
                            g_currentGroupUserList.clear();

                            vector<string> groupVec = responsejs["groups"];
                            for (string &groupstr : groupVec)
                            {
                                json grpjs = json::parse(groupstr);
                                Group group;
                                group.setId(grpjs["id"].get<int>());
                                group.setGroupName(grpjs["groupname"].get<string>());
                                group.setGroupDesc(grpjs["groupdesc"].get<string>());

                                vector<string> userVec = grpjs["users"];
                                for (string &userstr : userVec)
                                {
                                    json js = json::parse(userstr);
                                    GroupUser groupuser;
                                    groupuser.setId(js["id"].get<int>());
                                    groupuser.setNmae(js["name"].get<string>());
                                    groupuser.setState(js["state"].get<string>());
                                    groupuser.setGroupRole(js["role"].get<string>());
                                    group.getGroupUsers().push_back(groupuser);
                                }
                                g_currentGroupUserList.push_back(group);
                            }
                        }
                        showCurrentUserData();

                        if (responsejs.contains("offlinemsg"))
                        { //获取离线消息
                            vector<string> vec = responsejs["offlinemsg"];
                            for (string &str : vec)
                            {
                                json js = json::parse(str);
                                if (ONE_CHAT_MSG == js["msgid"].get<int>())
                                {
                                    cout << js["time"].get<string>() << " [" << js["id"].get<int>() << "]" << js["name"].get<string>()
                                         << " said:" << js["msg"].get<string>() << endl;
                                }
                                else if (GROUP_CHAT_MSG == js["msgid"].get<int>())
                                {
                                    cout << "-群消息-"
                                         << "[" << js["groupid"].get<int>() << "-" << js["groupname"].get<string>() << "] " << js["time"].get<string>() << "\n"
                                         << "[" << js["id"].get<int>() << "]" << js["name"].get<string>()
                                         << " Said:" << js["msg"].get<string>() << endl;
                                    continue;
                                }
                            }
                        }
                        static int resTaskNum = 0;
                        if (resTaskNum == 0)
                        { //创建单独的线程用来接收消息
                            std::thread readTask(readTaskHandler, clientfd);
                            readTask.detach();
                        }

                        isLogin = true;
                        mainMenu(clientfd);
                    }
                }
            }
        }
        break;
        case 2: //注册业务
        {
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "username:";
            cin.getline(name, 50);
            cout << "userpassword:";
            cin.getline(pwd, 50);
            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send reg msg error:" << request << endl;
            }
            else
            {
                char buffer[1024] = {0};
                int len = recv(clientfd, buffer, 1024, 0);
                if (-1 == len)
                {
                    cerr << "recv reg response error" << endl;
                }
                else
                {
                    json responsejs = json::parse(buffer);
                    if (0 != responsejs["errno"].get<int>())
                    {
                        cerr << name << "is already exist,register error!" << endl;
                    }
                    else
                    {
                        cout << name << " register success,userid is" << responsejs["id"].get<int>()
                             << ",do not forget it!" << endl;
                    }
                }
            }
        }
        break;
        case 3:
            close(clientfd);
            exit(0);
            break;
        default:
            break;
        }
    }
    return 0;
}

void showCurrentUserData()
{
    cout << "============================Login Infomation============================" << endl;
    cout << "--------------------当前登录用户ID：" << g_currentUser.getId() << " 用户名：" << g_currentUser.getName() << "---------------------" << endl;
    cout << "好友列表：" << endl;
    for (auto &user : g_currentUserFriendList)
    {
        cout << "-" << user.getId() << ":" << user.getName() << endl;
    }
    cout << "群组列表：" << endl;
    for (auto &group : g_currentGroupUserList)
    {
        cout << "群ID:" << group.getId() << " 名称:" << group.getGroupName() << " 简介:\"" << group.getGroupDesc() << "\"" << endl;
    }
    cout << "======================================================================== " << endl;
}

// "help" command handler
void help(int fd = 0, string str = "");
// "chat" command handler
void chat(int, string);
// "addfriend" command handler
void addfriend(int, string);
// "creategroup" command handler
void creategroup(int, string);
// "addgroup" command handler
void addgroup(int, string);
// "groupchat" command handler
void groupchat(int, string);
// "loginout" command handler
void loginout(int, string);

//用户支持的帮助列表
unordered_map<string, string> _commandMap = {
    {"help", "显示所有支持的命令，格式：help"},
    {"chat", "一对一聊天，格式：chat:friendid:[messsage]"},
    {"addfriend", "添加好友，格式：addfriend:[friendid]"},
    {"creategroup", "创建群组，格式：creategroup:[groupname]:[groupdesc]"},
    {"addgroup", "加入群组，格式：addgroup:[groupid]"},
    {"groupchat", "群聊天，格式：groupchat:[groupid]:[message]"},
    {"loginout", "退出登录,格式：loginout"}};
//命令对应的处理函数列表
unordered_map<string, function<void(int, string)>> _commandHandler = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"loginout", loginout}};

void help(int fd, string str)
{
    for (auto &v : _commandMap)
    {
        cout << v.first << "-\t" << v.second << endl;
    }
}
void chat(int fd, string str)
{
    json js;
    int idx = str.find(':');
    if (idx == -1)
    {
        cerr << "Format error..." << endl;
        return;
    }
    js["msgid"] = ONE_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = atoi(str.substr(0, idx).c_str());
    js["msg"] = str.substr(idx + 1, str.size() - idx);
    js["time"] = getCurrentTime();
    string sendstr = js.dump();
    if (-1 == send(fd, sendstr.c_str(), sendstr.size() + 1, 0))
    {
        cerr << "Send chat msg failed..." << endl;
    }
}
void addfriend(int fd, string str)
{
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = atoi(str.c_str());
    string sendstr = js.dump();
    if (-1 == send(fd, sendstr.c_str(), sendstr.size() + 1, 0))
    {
        cerr << "Send addfriend msg failed..." << endl;
    }
}
void creategroup(int fd, string str)
{
    json js;
    int idx = str.find(':');
    if (idx == -1)
    {
        cerr << "Format error..." << endl;
        return;
    }
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = str.substr(0, idx);
    js["groupdesc"] = str.substr(idx + 1, str.size() - idx);
    string sendstr = js.dump();
    if (-1 == send(fd, sendstr.c_str(), sendstr.size() + 1, 0))
    {
        cerr << "Send creategroup msg faild..." << endl;
    }
}
void addgroup(int fd, string str)
{
    json js;
    js["msgid"] = JOIN_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = atoi(str.c_str());
    string sendstr = js.dump();
    if (-1 == send(fd, sendstr.c_str(), sendstr.size() + 1, 0))
    {
        cerr << "Send addgroup msg faild..." << endl;
    }
}
void groupchat(int fd, string str)
{
    json js;
    int idx = str.find(':');
    if (idx == -1)
    {
        cerr << "Format error..." << endl;
        return;
    }
    int groupid = atoi(str.substr(0, idx).c_str());
    string groupname;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    for (auto &v : g_currentGroupUserList)
    {
        if (groupid == v.getId())
        {
            groupname = v.getGroupName();
        }
    }
    js["groupname"] = groupname;
    js["name"] = g_currentUser.getName();
    js["msg"] = str.substr(idx + 1, str.size() - idx);
    js["time"] = getCurrentTime();
    string sendstr = js.dump();
    if (-1 == send(fd, sendstr.c_str(), sendstr.size() + 1, 0))
    {
        cerr << "Send groupchat msg faild..." << endl;
    }
}
void loginout(int fd, string str)
{
    json js;
    js["msgid"] = LOGIN_OUT_MSG;
    js["id"] = g_currentUser.getId();
    string sendstr = js.dump();
    if (-1 == send(fd, sendstr.c_str(), sendstr.size() + 1, 0))
    {
        cerr << "Send addgroup msg faild..." << endl;
    }
    isLogin = false;
}
void mainMenu(int clientfd)
{
    help();
    char buffer[1024] = {0};
    while (isLogin)
    {
        cin.getline(buffer, 1024);
        string commandstr(buffer);
        string command;
        int idx = commandstr.find(':');
        if (idx == -1)
        {
            command = commandstr;
        }
        else
        {
            command = commandstr.substr(0, idx);
        }
        auto it = _commandHandler.find(command);
        if (it == _commandHandler.end())
        {
            continue;
        }
        it->second(clientfd, commandstr.substr(idx + 1, commandstr.size() - idx));
    }
}
void readTaskHandler(int clientfd)
{
    char buffer[1024] = {0};
    for (;;)
    {
        int len = recv(clientfd, buffer, 1024, 0);
        if (-1 == len || 0 == len)
        {
            // close(clientfd);
            // exit(-1);
            continue;
        }
        json js = json::parse(buffer);
        if (ONE_CHAT_MSG == js["msgid"].get<int>())
        {
            cout << js["time"].get<string>() << "\n"
                 << "[" << js["id"].get<int>() << "]" << js["name"].get<string>()
                 << " Said:" << js["msg"].get<string>() << endl;
        }

        if (GROUP_CHAT_MSG == js["msgid"].get<int>())
        {
            cout << "-群消息-"
                 << "[" << js["groupid"].get<int>() << "-" << js["groupname"].get<string>() << "] " << js["time"].get<string>() << "\n"
                 << "[" << js["id"].get<int>() << "]" << js["name"].get<string>()
                 << " Said:" << js["msg"].get<string>() << endl;
        }
    }
}
string getCurrentTime()
{
    char date[60] = {0};
    auto tt = chrono::system_clock::to_time_t(chrono::system_clock::now());
    struct tm *ptr = localtime(&tt);
    sprintf(date, "%d-%02d-%02d %02d:%02d:%02d",
            static_cast<int>(ptr->tm_year) + 1900, ptr->tm_mon, ptr->tm_mday, ptr->tm_hour, ptr->tm_min, ptr->tm_sec);
    return string(date);
}