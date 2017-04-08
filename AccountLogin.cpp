#include"net_disk_core.h"
std::string ProLoginRequest(const std::string & strBodyData)
{
	printf("ProLoginRequest paramter:%s\n",strBodyData.c_str());
	Json::Value input;
	if(!parse_json_body(input,strBodyData))
		return str_parse_json_failed;  

	std::string strUserName = input["username"].isString() ? input["username"].asString():"";
	std::string strPwd = input["password"].isString() ? input["password"].asString():"";
	Json::Value jout;
 	Json::FastWriter writer;
	std::string strOut; 
	if(strUserName.empty() || strPwd.empty())
	{
		jout["code"] = API_LACK_PARAM;
		jout["msg"] = "username and password can't be empty!";
		strOut = writer.write(jout);
		return strOut;
	}
	char sql[1024];
	sprintf(sql,"select user_xid,user_nickname,user_pwd from t_user where user_name='%s' limit 1;",strUserName.c_str());
	sql::PreparedStatement *pstmt = con->prepareStatement(sql);
    sql::ResultSet * pres = pstmt->executeQuery();
	bool bExist = (pres!=NULL  && pres->rowsCount()==1);
	if(bExist)
	{
		try
		{
			pres->beforeFirst();
			while(pres->next())
			{
				std::string strpwdStore  =  pres->getString("user_pwd");
				if(strPwd == strpwdStore) // login success ,return token
				{
					jout["code"] = 0;
					unsigned long user_xid = pres->getInt64("user_xid"); 
					jout["data"]["user_nickname"] = pres->getString("user_nickname").c_str();
					jout["data"]["token"] = GetNewToken(user_xid,"deviceid_test");
					jout["data"]["user_xid"] = user_xid;
				}
				else
				{
					jout["code"] = ACCOUNT_PWD_NOT_MATCH;
					jout["msg"] = "account password is not match!";
				}
			}
		}
		catch(sql::SQLException & e)
		{
			jout["code"] = DATABASE_ERROR;
			jout["msg"] = e.what();
		}
		delete pstmt;
		delete pres;
	}
	else
	{
		jout["data"] = ACCOUNT_NOT_EXIST;
		jout["msg"] = "account not exist!";
	}
	strOut = writer.write(jout);
	return strOut;
}
