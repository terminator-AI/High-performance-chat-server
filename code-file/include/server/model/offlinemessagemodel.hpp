#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H
#include"db.h"
#include<string>
#include<vector>
using namespace std;
class OffLineMessageModle
{
public:
    //插入离线消息
    void insert(int userid,string message);
    //删除离线消息
    void remove (int userid);
    //查询离线消息
    vector<string> query(int userid);
};

#endif