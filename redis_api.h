#ifndef REDIS_API_H
#define  REDIS_API_H
#include<string>
#include<hiredis/hiredis.h>
/*
*brief redis api class
*/ 

class RedisAPI
{
public:
	static	RedisAPI * Instance();
	bool connect(const char * pszHost,int iport = 6379,int iTimeoutMillSeconds=2000);
	void set_string(const char * pszKey,const std::string & strValue);
	std::string  get_string(const char * pszKey);
protected:
	RedisAPI();
private:
 redisContext *pRedisContext;
};
#endif //  REDIS_API_H

