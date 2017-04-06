#include<string>
using namespace std;
int u2g(char *inbuf,int inlen,char *outbuf,int outlen);
int g2u(char *inbuf,size_t inlen,char *outbuf,size_t outlen);
std::string utf82gbk(const std::string & strUTF8);
std::string gbk2utf8(const std::string & strGBK);
