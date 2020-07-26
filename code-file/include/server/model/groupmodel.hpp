#ifndef GROUPMODEL_H
#define GROUPMODEL_H
#include "group.hpp"
#include "db.h"
class GroupModel
{
public:
    //创建组
    bool createGroup(Group &group);
    //加入组
    bool addGroup(int userid, int groupid, string role = "normal");
    //查询用户所在组信息
    vector<Group> queryGroupInfo(int userid);
    //查询组其他成员信息,主要用于给其它成员群发信息
    vector<int> queryGroupMemberInfo(int userid, int groupid);

private:
    Group _group;
};

#endif