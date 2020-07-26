#ifndef GROUP_H
#define GROUP_H
#include "groupuser.hpp"
#include <string>
#include <vector>
using namespace std;
class Group
{
public:
    Group(int id = -1, string groupName = "", string groupDesc = "")
    {
        this->id = id;
        this->groupName = groupName;
        this->groupDesc = groupDesc;
    }
    void setId(int id) { this->id = id; }
    void setGroupName(string groupName) { this->groupName = groupName; }
    void setGroupDesc(string groupDesc) { this->groupDesc = groupDesc; }
    int getId() { return id; }
    string getGroupName() { return groupName; }
    string getGroupDesc() { return groupDesc; }
    vector<GroupUser>& getGroupUsers(){return this->_groupUser;}

private:
    int id;
    string groupName;
    string groupDesc;
    vector<GroupUser> _groupUser;
};

#endif