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
	std::string strData;
	if(strcasecmp(strUrl.c_str(),login_URL)==0)
	{
		strData =  ProLoginRequest(strBodyData);
	}
	else if(strcasecmp(strUrl.c_str(),register_URL)==0)
	{
		strData =  ProRegisterRequest(strBodyData);
	}
	else if(strcasecmp(strUrl.c_str(),filelist_URL)==0)
	{
		strData = ProGetFileListRequest(strBodyData);
	}
	else if(strcasecmp(strUrl.c_str(),modify_password_URL)==0)
	{
		strData =  ProModifyPwdRequest(strBodyData);
	}
	else 
		strData = str_api_not_exist;

	printf("post request url:%s,post data:%s,return:%s\n",strUrl.c_str(),strBodyData.c_str(),strData.c_str());
	return strData;
}
std::string ProRegisterRequest(const std::string & strBodyData)
{
	//printf("ProRegisterRequest paramter:%s\n",strBodyData.c_str());
	Json::Value input;
	if(!parse_json_body(input,strBodyData))
		return str_parse_json_failed;  

	Json::Value jout;
	jout["code"] = ACCOUNT_AREADY_EXIST;
	jout["msg"] = "account already exist!";
 	Json::FastWriter writer;
	std::string strOut = writer.write(jout);
	return strOut;
}
std::string ProLoginRequest(const std::string & strBodyData)
{
	//printf("ProLoginRequest paramter:%s\n",strBodyData.c_str());
	Json::Value input;
	if(!parse_json_body(input,strBodyData))
		return str_parse_json_failed;  

	Json::Value jout;
	jout["code"] = 0;
	jout["data"] = Json::Value(Json::arrayValue);
 	Json::FastWriter writer;
	std::string strOut = writer.write(jout);
	return strOut;
}
std::string ProGetFileListRequest(const std::string & strBodyData)
{
	//printf("ProGetFileListRequest paramter:%s\n",strBodyData.c_str());
	Json::Value input;
	if(!parse_json_body(input,strBodyData))
		return str_parse_json_failed;  

	Json::Value jout;
	jout["code"] = 0;
	Json::Value data(Json::arrayValue);
	data.append("1.txt");
	data.append("2.exe");
	data.append("3.cpp");
	jout["data"] = data ;
 	Json::FastWriter writer;
	std::string strOut = writer.write(jout);
	return strOut;
}
std::string ProModifyPwdRequest(const std::string & strBodyData)
{
	//printf("ProModifyPwdRequest paramter:%s\n",strBodyData.c_str());
	Json::Value input;
	if(!parse_json_body(input,strBodyData))
		return str_parse_json_failed;  

	Json::Value jout;
	jout["code"] = 0;
	jout["data"] = Json::Value(Json::arrayValue);
 	Json::FastWriter writer;
	std::string strOut = writer.write(jout);
	return strOut;
}
bool parse_json_body(Json::Value & valueOut,const std::string & strBodyData)
{
	Json::Reader reader;
	Json::Value & obj = valueOut;
	try
	{
		if (!reader.parse(strBodyData, obj, false)) 
			return false;
		if (!obj.isObject()) 
			return false;
	}
	catch(...)
	{
		printf("failed to write json :%s\n",strBodyData.c_str());
		return false;
	}
	return true;
}
