#include "usermodel.hpp"
#include "db.h"
#include <iostream>
using namespace std;

//User的增加方法
bool UserModel::insert(User &user)
{
    //组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "insert into User(name,password,state) values('%s','%s','%s');",
            user.getName().c_str(), user.getPassWord().c_str(), user.getState().c_str());
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            //获取插入成功的用户数据生产的主键id
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}
//User的查询方法
User UserModel::query(int id)
{
    char sql[1024] = {0};
    sprintf(sql, "select * from User where id = %d;", id);
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setNmae(row[1]);
                user.setPassWord(row[2]);
                user.setState(row[3]);
                mysql_free_result(res);
                return user;
            }
        }
    }
    return User();
}

//User的更新方法
bool UserModel::updateState(User &user)
{
    char sql[1024] = {0};
    sprintf(sql, "update User set state='%s' where id=%d;", user.getState().c_str(), user.getId());
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            return true;
        }
    }
    return false;
}
//UserState状态重置
void UserModel::resetState()
{
    char sql[1024] = "update User set state='offline' where state='online';";
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}