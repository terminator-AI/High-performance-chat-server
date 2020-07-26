#ifndef FRIENDMODEL_H
#define FRIENDMODEL_H
#include "db.h"
#include "user.hpp"
#include <vector>
class FriendModel
{
public:
    //添加好友方法
    void addFriend(int userid, int friendid);
    //查询好友方法
    vector<User> queryFriend(int userid);

private:
};

#endif