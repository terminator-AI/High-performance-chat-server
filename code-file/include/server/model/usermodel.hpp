#ifndef USERMODEL_H
#define USERMODEL_H
#include "user.hpp"
#include <string>

class UserModel
{
public:
    //User的增加方法
    bool insert(User &user);
    //User的查询方法
    User query(int id);
    //User的登录状态更新方法
    bool updateState(User &user);
    //UserState状态重置
    void resetState();
};
#endif