#include"redis_api.h"
#include<iostream>
#include<assert.h>
RedisAPI * RedisAPI::Instance()
{
	static RedisAPI redis;
	return &redis;
}
RedisAPI::RedisAPI()
{
	pRedisContext = NULL;
}
bool  RedisAPI::connect(const char * pszHost,int iport,int iTimeoutMillSeconds)
{
	int iSeconds = iTimeoutMillSeconds /1000;
	int iMileSeconds = iTimeoutMillSeconds - iSeconds *1000; 
	struct timeval timeout = {iSeconds, iMileSeconds};    //2s的超时时间	
	pRedisContext = (redisContext*)redisConnectWithTimeout(pszHost, iport, timeout);
	if ( (NULL == pRedisContext) || (pRedisContext->err) )
    {
        if (pRedisContext)
        {
            std::cout << "redis connect error:" << pRedisContext->errstr <<",host:" <<pszHost << ",port:"<< iport << std::endl;
        }
        else
        {
            std::cout << "connect error: can't allocate redis context." << std::endl;
        }
		pRedisContext = NULL;
        return false;
    }
	return true;
}
void  RedisAPI::set_string(const char * pszKey,const std::string & value)
{
	if(pRedisContext == NULL)
	{
		printf("redis not connected!\n");
		return;
	}
	 assert(pszKey!=NULL);
	 redisCommand(this->pRedisContext, "SET %s %s", pszKey, value.c_str());  
}
std::string RedisAPI::get_string(const char * pszKey)
{
	printf("RedisAPI::get_string(%s) begin call...\n",pszKey);
	if(pRedisContext == NULL)
	{
		printf("redis not connected!\n");
		return "";
	}
	assert(pszKey!=NULL);
	redisReply * reply = (redisReply*)redisCommand(this->pRedisContext, "GET %s", pszKey);  
	if(!reply)
	{
		printf("can't get data from key %s\n",pszKey);
		return "";
	}
	printf("%s key-value: reply->str=%p\n",pszKey,reply->str);
    std::string str = reply->str!=NULL ? reply->str : "";  
	freeReplyObject(reply);  
	return str;
}
