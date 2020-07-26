#ifndef PUBLIC_H
#define PUBLIC_H

//server和client的公共文件

enum EnMsgType
{
    LOGIN_MSG = 1, //登录消息
    LOGIN_MSG_ACK, //登录回应消息
    LOGIN_OUT_MSG, //退出登录消息
    REG_MSG,       //注册消息
    REG_MSG_ACK,   //注册回应消息

    ONE_CHAT_MSG,   //一对一聊天消息
    ADD_FRIEND_MSG, //添加好友消息

    CREATE_GROUP_MSG, //创建组消息
    JOIN_GROUP_MSG,   //加入组消息
    GROUP_CHAT_MSG    //组聊天消息
};

#endif