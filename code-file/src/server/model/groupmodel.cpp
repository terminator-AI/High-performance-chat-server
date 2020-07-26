#include "groupmodel.hpp"
//创建组
bool GroupModel::createGroup(Group &group)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into AllGroup(groupname,groupdesc) values('%s','%s');",
            group.getGroupName().c_str(), group.getGroupDesc().c_str());
    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}
//加入组
bool GroupModel::addGroup(int userid, int groupid, string role)
{
    //组装SQL语句
    char sql[1024] = {0};
    sprintf(sql, "insert into GroupUser(groupid,userid,grouprole) values(%d,%d,'%s');",
            groupid, userid, role.c_str());
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
        return true;
    }
    return false;
}
//查询用户所在组信息
vector<Group> GroupModel::queryGroupInfo(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "select a.id,a.groupname,a.groupdesc from AllGroup a inner join GroupUser b on b.groupid=a.id where b.userid=%d;", userid);
    MySQL mysql;
    vector<Group> groupVec;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            { //组信息
                Group group;
                group.setId(atoi(row[0]));
                group.setGroupName(row[1]);
                group.setGroupDesc(row[2]);
                groupVec.push_back(group);
            }
            mysql_free_result(res);
        }
    }
    for (Group &group : groupVec)
    { //每个组成员信息
        sprintf(sql, "select a.id,a.name,a.state,b.grouprole from User a \
        inner join GroupUser b on b.userid = a.id where b.groupid = % d;",
                group.getId());
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                GroupUser groupuser;
                groupuser.setId(atoi(row[0]));
                groupuser.setNmae(row[1]);
                groupuser.setState(row[2]);
                groupuser.setGroupRole(row[3]);
                group.getGroupUsers().push_back(groupuser);
            }
            mysql_free_result(res);
        }
    }
    return groupVec;
}
//根据目标组id查询组其他成员信息,主要用于给其他成员群发消息
vector<int> GroupModel::queryGroupMemberInfo(int userid, int groupid)
{
    char sql[1024] = {0};
    sprintf(sql, "select userid from GroupUser where groupid = %d and userid != %d;", groupid, userid);
    MySQL mysql;
    vector<int> userid_s;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            { //将同组用户id放入userid_s中
                userid_s.push_back(atoi(row[0]));
            }
            mysql_free_result(res);
        }
    }
    return userid_s;
}