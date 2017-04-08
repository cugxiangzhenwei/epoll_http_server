#ifndef NET_DISK_CORE_H
#define NET_DISK_CORE_H
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
#include<json/reader.h>
#include<json/writer.h>
using namespace std;  

extern sql::Connection *con;  

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
#define modify_password_URL "/modify/password"

typedef enum
{
	API_NOT_EXIST = -2, 			/*! api not exist */
	PARSE_JSON_INPUT_FAILED =-1, 	/*! parse input json failed */
	API_SUCCESS = 0, 				/*! success code */
	ACCOUNT_AREADY_EXIST,			/*! account aready exist! */			
	ACCOUNT_NOT_EXIST,				/*! account not exist!	*/
	ACCOUNT_PWD_NOT_MATCH,			/*! account pwasword not match!*/
	API_LACK_PARAM,					/*! api param lack or empty*/
	PWD_CONFIRM_NOT_MATCH,  	    /*! password is not equal to confirmpassword */
	DATABASE_ERROR					/*! database operator error!*/
}net_disk_code;

#define str_parse_json_failed	"{code:-1,msg:\"input json parse failed!\"}\n" 
#define str_api_not_exist		"{code:-2,msg:\"api not exist!\"}\n"

bool GetMySQLConnection(const std::string & strConStr,const std::string & strUsrName,const std::string & strPwd);
void CloseMySQLConnection();
std::string ProNetDiskRequest(const std::string & strBodyData,const std::string & strUrl);
std::string ProRegisterRequest(const std::string & strBodyData);
std::string ProLoginRequest(const std::string & strBodyData);
std::string ProGetFileListRequest(const std::string & strBodyData);
std::string ProModifyPwdRequest(const std::string & strBodyData);
bool parse_json_body(Json::Value & valueOut,const std::string & strBodyData);

extern std::string GetNewToken(uint64_t user_xid,const std::string & strDeviceId);
extern bool IsTokenValid(uint64_t user_xid,const std::string & strDeviceId,const std::string & strToken);

#endif // NET_DISK_CORE_H
