#include "ChatService.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>

//获取单例对象的接口函数
ChatService *ChatService::instance()
{
    static ChatService service;
    return &service;
}
//注册消息以及对应的handler回调操作
ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG, bind(&ChatService::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, bind(&ChatService::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGIN_OUT_MSG, bind(&ChatService::loginout, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, bind(&ChatService::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG, bind(&ChatService::addFriend, this, _1, _2, _3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG, bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({JOIN_GROUP_MSG, bind(&ChatService::joinGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, bind(&ChatService::groupChat, this, _1, _2, _3)});
    if (_redis.connect())
    {
        _redis.init_notify_handler(bind(&ChatService::readFromRedisMessage, this, _1, _2));
    }
}
//获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid)
{
    //记录错误日志，msgid没有对应的事件处理回调
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        //LOG_ERROR << "msgid:" << msgid << " can not find handler!";
        //返回一个默认处理器
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time) {
            LOG_ERROR << "msgid:" << msgid << " can not find handler!";
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}
//处理登录业务
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    string password = js["password"].get<string>();
    User user = _usermodel.query(id);
    if (password == user.getPassWord())
    {
        //登录成功，但已经在线
        if ("online" == user.getState())
        {
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 3;
            response["errmsg"] = "该用户已在线，不可重复登录";
            conn->send(response.dump());
        }
        else
        //登录成功，且不在线
        {
            //记录用户登录在线信息
            {
                //维护_connectionMap线程安全
                lock_guard<mutex> lock(_connectionMapMutex);
                _connectionMap.insert({id, conn});
            }

            //向redis订阅有关用户的信息
            _redis.subscribe(id);

            //更新用户在线状态
            user.setState("online");
            _usermodel.updateState(user);
            json response; //反馈信息
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();
            //查询离线消息
            vector<string> vec = _offlineMsgmodel.query(id);
            ;
            if (!vec.empty())
            { //将查询到的离线消息转化为json，并移除表中的对应id的离线消息
                response["offlinemsg"] = vec;
                _offlineMsgmodel.remove(id);
            }
            //查询好友信息，反馈给刚登录用户
            vector<User> userVec = _friednModel.queryFriend(id);
            if (!userVec.empty())
            {
                vector<string> vec2;
                for (auto &v : userVec)
                {
                    json js;
                    js["id"] = v.getId();
                    js["name"] = v.getName();
                    js["state"] = v.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            //查询用户的群组信息
            vector<Group> groupuserVec = _groupmodel.queryGroupInfo(id);
            if (!groupuserVec.empty())
            {
                vector<string> groupV;
                for (Group &group : groupuserVec)
                {
                    json grpjson;
                    grpjson["id"] = group.getId();
                    grpjson["groupname"] = group.getGroupName();
                    grpjson["groupdesc"] = group.getGroupDesc();
                    vector<string> userV;
                    for (GroupUser &user : group.getGroupUsers())
                    {
                        json js;
                        js["id"] = user.getId();
                        js["name"] = user.getName();
                        js["state"] = user.getState();
                        js["role"] = user.getGroupRole();
                        userV.push_back(js.dump());
                    }
                    grpjson["users"] = userV;
                    groupV.push_back(grpjson.dump());
                }
                response["groups"] = groupV;
            }
            conn->send(response.dump());
        }
    }
    else
    {
        //登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 2;
        response["errmsg"] = "账号错误或者密码错误...";
        conn->send(response.dump());
    }
}
//处理注册业务
void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"].get<string>();
    string password = js["password"].get<string>();
    User user;
    user.setNmae(name);
    user.setPassWord(password);
    bool state = _usermodel.insert(user);
    if (state)
    {
        //注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        //注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}
//客户端处理异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connectionMapMutex);
        for (auto it = _connectionMap.begin(); it != _connectionMap.end(); ++it)
        {
            if (it->second == conn)
            {
                user.setId(it->first);
                _connectionMap.erase(it);
                break;
            }
        }
    }
    //取消该用户订阅消息
    _redis.unsubscribe(user.getId());

    if (user.getId() != -1)
    { //更新用户状态信息
        user.setState("offline");
        _usermodel.updateState(user);
    }
}
//服务的处理异常退出
void ChatService::reset()
{
    _usermodel.resetState();
}
//一对一聊天业务
void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["toid"].get<int>();
    {
        lock_guard<mutex> lock(_connectionMapMutex);
        auto it = _connectionMap.find(toid);
        if (it != _connectionMap.end())
        {
            //对方用户在线,直接转发js消息
            it->second->send(js.dump());
            return;
        }
    }

    //如果当前服务器连接队列里没有该连接，去数据库中查找是否在线，然后发布订阅消息，通过消息队列推送给对应服务器上的用户
    User user = _usermodel.query(toid);
    if (user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
        return;
    }

    //对方用户不在线，将离线消息存储再离线消息库中
    _offlineMsgmodel.insert(toid, js.dump());
}
//添加好友业务
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();
    //将好友存储到表中去
    _friednModel.addFriend(userid, friendid);
}

//创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string groupName = js["groupname"].get<string>();
    string groupDesc = js["groupdesc"].get<string>();
    //创建群组
    Group group(userid, groupName, groupDesc);
    if (_groupmodel.createGroup(group))
    { //存储创建群组人员信息
        _groupmodel.addGroup(userid, group.getId(), "creator");
    }
}
//加入群组业务
void ChatService::joinGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupmodel.addGroup(userid, groupid);
}
//群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    string msg = js["msg"].get<string>();
    vector<int> groupMeberId = _groupmodel.queryGroupMemberInfo(userid, groupid);
    lock_guard<mutex> lock(_connectionMapMutex);
    for (auto &v : groupMeberId)
    {
        if (_connectionMap.find(v) != _connectionMap.end())
        { //在线直接转发
            _connectionMap[v]->send(js.dump());
        }
        else
        {
            //跨服务器通信
            User user = _usermodel.query(v);
            if (user.getState() == "online")
            {
                _redis.publish(v, js.dump());
            }
            else
            {
                //不在线存储离线消息
                _offlineMsgmodel.insert(v, js.dump());
            }
        }
    }
}

//用户退出登录业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connectionMapMutex);
        auto it = _connectionMap.find(userid);
        if (it != _connectionMap.end())
        {
            _connectionMap.erase(it);
        }
    }
    //取消订阅
    _redis.unsubscribe(userid);
    //更新状态
    User user(userid, "", "", "offline");
    _usermodel.updateState(user);
}

//推送订阅来的消息
void ChatService::readFromRedisMessage(int userid, string msg)
{
    lock_guard<mutex> lock(_connectionMapMutex);
    auto it = _connectionMap.find(userid);
    if (it != _connectionMap.end())
    {
        it->second->send(msg);
        return;
    }
    _offlineMsgmodel.insert(userid, msg);
}