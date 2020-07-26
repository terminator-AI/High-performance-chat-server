#include "friendmodel.hpp"

//添加好友方法
void FriendModel::addFriend(int userid, int friendid)
{
    //组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "insert into Friend values(%d,%d);", userid, friendid);
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}
//查询好友方法
vector<User> FriendModel::queryFriend(int userid)
{
    char sql[1024] = {0};

    sprintf(sql, "select a.id,a.name,a.state from User a inner join Friend b on b.friendid=a.id where b.userid=%d;", userid);
    MySQL mysql;
    vector<User> userVec;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setNmae(row[1]);
                user.setState(row[2]);
                userVec.push_back(user);
            }
            mysql_free_result(res);
            return userVec;
        }
    }
    return userVec;
}
