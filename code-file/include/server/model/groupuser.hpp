#ifndef GROUPUSER_H
#define GROUPUSER_H
#include "user.hpp"

class GroupUser : public User
{
public:
//
    void setGroupRole(string groupRole = "normal") { this->groupRole = groupRole; }
    string getGroupRole() { return groupRole; }

private:
    string groupRole; //群成员身份
};

#endif