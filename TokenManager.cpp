#include<string>
#include<map>
#include<stdio.h>
using namespace std;
typedef unsigned long uint64_t;
typedef const char * deviceIdType;
typedef const char * tokenType;
typedef uint64_t  userXidType;
typedef uint64_t  timeStampType;
typedef map<deviceIdType,tokenType> mapDeviceTokenType;
typedef multimap<userXidType,mapDeviceTokenType> MapTokenType;
typedef map<tokenType,timeStampType> token_TimeValidMap;
static token_TimeValidMap g_token_timeValidMap;
static MapTokenType g_mapToken;
#define TIME_VALID_TOKEN	3600 // token is valid in an hour

std::string GetNewToken(uint64_t user_xid,const std::string & strDeviceId)
{
	char szStrToken[20];
	sprintf(szStrToken,"%08ld_%08ld",user_xid,time(NULL));
	std::string strToken = szStrToken;
	unsigned long tNow = time(NULL);
	g_token_timeValidMap[strToken.c_str()] = timeStampType(tNow);	// make new token ,record timestamp
	MapTokenType::iterator iter = g_mapToken.find(user_xid);
	if(iter ==g_mapToken.end())	
	{
		mapDeviceTokenType dTmap;
		dTmap[strDeviceId.c_str()] = strToken.c_str(); // bind token to  deviceid 
		g_mapToken.insert(make_pair(user_xid,dTmap)); // bind deviceid to indicdate user
	}
	else
	{
		mapDeviceTokenType & dtMap = iter->second; // get user device list
		dtMap[strDeviceId.c_str()] = strToken.c_str(); // add new device token or update old device token
	}
	return strToken;
}
bool IsTokenValid(uint64_t user_xid,const std::string & strDeviceId,const std::string & strToken)
{
	MapTokenType::iterator iter = g_mapToken.find(user_xid);
	if(iter ==g_mapToken.end())		
		return false;

	mapDeviceTokenType & dtMap = iter->second;
	mapDeviceTokenType::iterator iter2 = dtMap.find(strDeviceId.c_str());
	if(iter2 == dtMap.end())
	{
		if(dtMap.empty())
			g_mapToken.erase(iter); // user_xid has no device is login,erase from base token map
	
		return false;
	}
	
	if(iter2->second == strToken)
	{
		token_TimeValidMap::iterator iter3 = g_token_timeValidMap.find(iter2->second);
		if(iter3== g_token_timeValidMap.end())
		{
			dtMap.erase(iter2); // not found timestap, not valid ,erase this device from user token list 
			if(dtMap.empty()) // check whether user_xid device list is empty
				g_mapToken.erase(iter); // user_xid has no device is login,erase from base token map

			return false;
		}	
		else
		{
			uint64_t tNow = time(NULL);
			if(iter3->second + TIME_VALID_TOKEN > tNow)
			{
				g_token_timeValidMap.erase(iter3);
				dtMap.erase(iter2); // token is timeout ,erase from device token list
				if(dtMap.empty()) // check whether user_xid device list is empty
					g_mapToken.erase(iter); // user_xid has no device is login,erase from base token map

				return false;
			}
		}
		return true;
	}
	return false; // token 无效
}
