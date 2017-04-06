#include"net_disk_core.h"
std::string ProLoginRequest(const std::string & strBodyData)
{
	printf("ProLoginRequest paramter:%s\n",strBodyData.c_str());
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
