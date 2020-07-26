#ifndef CHATSERVICE_H
#define CHATSERVICE_H
#include <muduo/net/TcpConnection.h>
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"
#include <unordered_map>
#include <functional>
#include <mutex>
#include "json.hpp"
#include "usermodel.hpp"

using json = nlohmann::json;
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;

//表示处理消息的事件回调方法类型
using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp)>;

//聊天服务器业务类
class ChatService
{
public:
    //获取单例对象的接口函数
    static ChatService *instance();
    //处理登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //获取消息对应的处理器
    MsgHandler getHandler(int msgid);
    //客户端处理异常退出
    void clientCloseException(const TcpConnectionPtr &conn);
    //服务的处理异常退出
    void reset();
    //一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //加入群组业务
    void joinGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //用户退出登录业务
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //推送订阅来的消息
    void readFromRedisMessage(int userid, string msg);

private:
    ChatService();
    //存储消息id和其对应的业务处理方法
    unordered_map<int, MsgHandler> _msgHandlerMap;
    //存储登录连接
    unordered_map<int, TcpConnectionPtr> _connectionMap;
    //维护_connectionMap线程安全
    mutex _connectionMapMutex;

    //数据库User表操作类对象
    UserModel _usermodel;
    //数据库offlineMessage表操作类对象
    OffLineMessageModle _offlineMsgmodel;
    //数据库friend表操作类对象
    FriendModel _friednModel;
    //群组数据库操作类对象
    GroupModel _groupmodel;

    //redis连接操作
    Redis _redis;
};

#endif