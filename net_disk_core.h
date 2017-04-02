#include<iostream>  
#include "mysql_driver.h"  
#include "mysql_connection.h"  
#include <cppconn/driver.h>  
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h> 
#include<string.h>
#include <fstream>
using namespace std;  

class DataBuf : public std::streambuf
{
public:
	DataBuf(char* d, size_t s)
	{
		setg(d, d, d+s);
	}
};

#define login_URL "/login"
#define register_URL "/register"
#define filelist_URL "/filelist"
#define modify_password__URL "/modify/password"

bool GetMySQLConnection(const std::string & strConStr,const std::string & strUsrName,const std::string & strPwd);
void CloseMySQLConnection();
std::string ProNetDiskRequest(const std::string & strBodyData,const std::string & strUrl);
std::string ProRegisterRequest(const std::string & strBodyData);
std::string ProLoginRequest(const std::string & strBodyData);
std::string ProGetFileListRequest(const std::string & strBodyData);
std::string ProModifyPwdRequest(const std::string & strBodyData);
