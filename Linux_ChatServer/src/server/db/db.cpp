#include "db.h"
#include "Linux_mymuduo/Logger.h"

// 数据库配置信息
static string server = "127.0.0.1";
static string user = "root";
static string password = "123456";
static string dbname = "chat";

// 初始化数据库连接
MySQL::MySQL(){
    conn_ = mysql_init(nullptr);
}

// 释放数据库连接资源
MySQL::~MySQL(){
    if (conn_ != nullptr)
        mysql_close(conn_);
}

// 连接数据库
bool MySQL::connect(){
    MYSQL *p = mysql_real_connect(conn_, server.c_str(), user.c_str(),password.c_str(), dbname.c_str(), 3306, nullptr, 0);
    if (p != nullptr){
        // C和C++代码默认的编码字符是ASCII，如果不设置，从MySQL上拉下来的中文显示？
        mysql_query(conn_, "set names gbk");
        LOG_INFO("connect mysql success! \n");
    }
    else{
        LOG_INFO("connect mysql fail! \n");
    }

    return p;
}

// 更新操作
bool MySQL::update(string sql){
    if (mysql_query(conn_, sql.c_str())){
        LOG_INFO("%s : %d : %s 更新失败! \n",__FILE__,__LINE__,sql.c_str());
        return false;
    }

    return true;
}

// 查询操作
MYSQL_RES *MySQL::query(string sql){
    if (mysql_query(conn_, sql.c_str())){
        LOG_INFO("%s : %d : %s 查询失败! \n",__FILE__,__LINE__,sql.c_str());
        return nullptr;
    }
    
    return mysql_use_result(conn_);
}

// 获取连接
MYSQL* MySQL::getConnection(){
    return conn_;
}
