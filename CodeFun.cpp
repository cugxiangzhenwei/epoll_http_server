#include <iconv.h>
#include<string.h>
#include"CodeFun.h"
#include<stdio.h>
int code_convert(const char *from_charset,const char *to_charset,char *inbuf,size_t inlen,char *outbuf,size_t outlen)
{
	iconv_t cd;
	char **pin = &inbuf;
	char **pout = &outbuf;

	cd = iconv_open(to_charset,from_charset);
	if (cd==0)
	{
		printf("iconv_open failed!\n");
		return -1;
	}
	memset(outbuf,0,outlen);
	size_t iRev = iconv(cd,pin,&inlen,pout,&outlen);
	iconv_close(cd);
	if(iRev == (size_t)-1)
	{
		printf("code_convert 失败，返回:%s,iconv返回:%lu\n",outbuf,iRev);
		return -1;
	}
	//	printf("code_convert 成功，返回:%s,iconv返回:%lu\n",outbuf,iRev);
	return 0;
}

int u2g(char *inbuf,int inlen,char *outbuf,int outlen)
{
	return code_convert("utf-8","gbk",inbuf,inlen,outbuf,outlen);
}

int g2u(char *inbuf,size_t inlen,char *outbuf,size_t outlen)
{
	return code_convert("gbk","utf-8",inbuf,inlen,outbuf,outlen);
}

std::string utf82gbk(const std::string & strUTF8)
{
	int iLen = strUTF8.length();
	char * pOut = new char[iLen*4];
	u2g((char*)strUTF8.c_str(),iLen,pOut,iLen*4);
	std:: string strOut = pOut;
	delete []pOut;
	return strOut;
}
std::string gbk2utf8(const std::string & strGBK)
{
	int iLen = strGBK.length();
	char * pOut = new char[iLen*4];
	g2u((char*)strGBK.c_str(),iLen,pOut,iLen*4);
	std:: string strOut = pOut;
	printf("gbk2utf8 return %s\n",pOut);
	delete []pOut;
	return strOut;
}
