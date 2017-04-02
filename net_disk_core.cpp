#include"net_disk_core.h"
sql::Connection *con = NULL;  
bool GetMySQLConnection(const std::string & strConStr /*="tcp://127.0.0.1:3306"*/,const std::string & strUsrName,const std::string & strPwd)
{
	try
	{
		sql::mysql::MySQL_Driver *driver;  
		driver = sql::mysql::get_mysql_driver_instance();  
		con = driver->connect(strConStr.c_str(),strUsrName.c_str(),strPwd.c_str());  
		con->setSchema("net_disk");
	
	}
	catch (sql::SQLException &e) {  
        cout << "ERROR: " << e.what();  
        cout << " (MySQL error code: " << e.getErrorCode();  
        cout << ", SQLState: " << e.getSQLState() << ")" << endl;  
		return false;
  }
  catch (std::runtime_error &e) {  
        cout << "ERROR: " << e.what() << endl;  
        return false;  
    }  
	catch(...)
	{
		printf("GetMySQLConnection failed!\n");
		CloseMySQLConnection();
		return false;
	}
	printf("connect to mysql:%s success!\n",strConStr.c_str());
	return true;
}
void CloseMySQLConnection()
{
	if(con!=NULL)
	{
		delete con;
		con = NULL;
	}
}
std::string ProNetDiskRequest(const std::string & strBodyData,const std::string & strUrl)
{
	printf("post request url:%s,post data:%s\n",strUrl.c_str(),strBodyData.c_str());
	if(strcasecmp(strUrl.c_str(),login_URL)==0)
	{
		return ProLoginRequest(strBodyData);
	}
	return "";
}
std::string ProRegisterRequest(const std::string & strBodyData)
{
	printf("ProRegisterRequest paramter:%s\n",strBodyData.c_str());
	return "{code:0,data:[]}";
}
std::string ProLoginRequest(const std::string & strBodyData)
{
	printf("ProLoginRequest paramter:%s\n",strBodyData.c_str());
	return "{code:1,data:\"account not exist!\"}";
}
std::string ProGetFileListRequest(const std::string & strBodyData)
{
	printf("ProGetFileListRequest paramter:%s\n",strBodyData.c_str());
	return "{code:0,data:[1.txt, 2.exe,3.cpp]}";
}
std::string ProModifyPwdRequest(const std::string & strBodyData)
{
	printf("ProModifyPwdRequest paramter:%s\n",strBodyData.c_str());
	return "{code:0,data:[]}";
}
