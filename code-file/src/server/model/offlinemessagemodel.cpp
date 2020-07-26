#include "offlinemessagemodel.hpp"

//插入离线消息
void OffLineMessageModle::insert(int userid, string message)
{
    char sql[1024];
    sprintf(sql, "insert into OfflineMessage values(%d,'%s');", userid, message.c_str());
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}
//删除离线消息
void OffLineMessageModle::remove(int userid)
{
    char sql[1024];
    sprintf(sql, "delete from OfflineMessage where userid=%d;", userid);
    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
}
//查询离线消息
vector<string> OffLineMessageModle::query(int userid)
{
    char sql[1024];
    sprintf(sql, "select message from OfflineMessage where userid=%d", userid);
    MySQL mysql;
    vector<string> message;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            { //将离线消息放入至message容器中
                message.push_back(row[0]);
            }
            mysql_free_result(res);
            return message;
        }
    }
    return message;
}