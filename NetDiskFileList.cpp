#include"net_disk_core.h"
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
