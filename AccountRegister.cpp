#include"net_disk_core.h"
std::string ProRegisterRequest(const std::string & strBodyData)
{
	//printf("ProRegisterRequest paramter:%s\n",strBodyData.c_str());
	Json::Value input;
	if(!parse_json_body(input,strBodyData))
		return str_parse_json_failed;  

	std::string strUserName = input["username"].isString() ? input["username"].asString():"";
	std::string strPwd = input["password"].isString() ? input["password"].asString():"";
	std::string strConfirmPwd = input["confirmpassword"].isString() ? input["confirmpassword"].asString():"";
	std::string strNickName = input["nickname"].isString() ? input["nickname"].asString():"";
	std::string strPhone = input["phone"].isString() ? input["phone"].asString():"";
	std::string strEmail = input["email"].isString() ? input["email"].asString():"";
	std::string strSex = input["sex"].isString() ? input["sex"].asString():"";
	Json::Value jout;
	if(strUserName.empty() || strPwd.empty() || strConfirmPwd.empty())
	{
		jout["code"]  = API_LACK_PARAM;
		jout["msg"] = "user name or password or confirmpassword param is empty!";
	}
	else if(strPwd != strConfirmPwd)
	{
		jout["code"] = PWD_CONFIRM_NOT_MATCH;
		jout["msg"] = "password and confirm pwssword is not match!";	
	}
	
	char sql[1024];
	sprintf(sql,"select user_xid,user_nickname from t_user where user_name='%s' limit 1;",strUserName.c_str());
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
				unsigned int user_xid =  pres->getInt64("user_xid");
				jout["data"]["user_xid"] = user_xid;
				strNickName  =  pres->getString("user_nickname");
				jout["data"]["user_nickname"] = strNickName.c_str();
			}
			jout["code"] = ACCOUNT_AREADY_EXIST;
			jout["msg"] = "account already exist!";
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
		try
		{
			delete pstmt;
			delete pres;
			sprintf(sql,"INSERT INTO t_user(user_name,user_nickname,user_pwd,user_sex,user_phone,user_mail,user_space) \
 VALUES ('%s','%s','%s','%s','%s','%s',%d)",
				strUserName.c_str(),
				strNickName.c_str(),
				strPwd.c_str(),
				strSex.c_str(),
				strPhone.c_str(),
				strEmail.c_str(),
				0);

			pstmt = con->prepareStatement(sql);
			pres = pstmt->executeQuery();
			delete pstmt;
			delete pres;
			sprintf(sql,"select user_xid,user_nickname from t_user where user_name='%s';",strUserName.c_str());
			pstmt = con->prepareStatement(sql);
			pres  = pstmt->executeQuery();
			pres->beforeFirst();
			if(pres->next())
			{
				unsigned int user_xid =  pres->getUInt("user_xid");
				jout["code"] = 0;
				jout["data"]["user_xid"] = user_xid;
				jout["data"]["user_nickname"] = pres->getString("user_nickname").c_str();
			}
			delete pstmt;
			delete pres;
		}
		catch(sql::SQLException & e)
		{
			jout["code"] = DATABASE_ERROR;
			jout["msg"] = e.what();
		}
	}
 	Json::FastWriter writer;
	std::string strOut = writer.write(jout);
	return strOut;
}
