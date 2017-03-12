#include"UrlCode.h"
#include<assert.h>
#include<stdio.h>
#include<string.h>
void decode(const char* psrc, char* pdst) {

            for (; *psrc != 0; ++psrc) {

                if (*psrc == '%') {

                    int code;
                    if (sscanf(psrc+1, "%02x", &code) != 1) 
                        code = '?';
                    *pdst++ = code;
                    psrc += 2;
                }
                else {
                    *pdst++ = *psrc;
                }
            }
        }

  bool need_encoding(const char ch) {
            const unsigned char *p = (const unsigned char*)&ch;
            return (*p >  126 || *p == '&' || *p == ' ' || *p == '=' || *p == '%' || 
                *p == '.' || *p == '/' || *p == '+' || 
                *p == '`' || *p == '{' || *p == '}' || *p == '|' || *p == '[' ||
                *p == ']' || *p == '\"' || *p == '<' || *p == '>' || *p == '\\' ||
                *p == '^' || *p == ';' || *p == ':');
        }
 	 bool need_encoding(const char* pcsz) {
            for (; *pcsz != 0; ++pcsz) {
                if (need_encoding(*pcsz))
                    return true;
            }
            return false;
        }

std::string& encode(const char* psrc, std::string& str)	{
            str.resize(strlen(psrc) * 3 + 1, '\0');

            char* pdst = &(*str.begin());

            for (; *psrc != 0; ++psrc) {

                unsigned char* p = (unsigned char*)psrc;
                if (need_encoding(*p)) {

                    char a[3];
                    sprintf(a, "%02x", *p);
                    *pdst = '%';
                    *(pdst + 1) = a[0];
                    *(pdst + 2) = a[1];
                    pdst += 3;
                }
                else {
                    *pdst++ = *p;
                }
            }

            str.resize(pdst - &(*str.begin()));

            return str;
        }
std::string UrlEncode(const std::string& str)
{
	std::string strTmp = str;
	return encode(str.c_str(),strTmp);
}
std::string UrlDecode(const std::string& str)
{
	std::string strOut;
	strOut.resize(str.size());
	decode(str.c_str(),&strOut[0]);
	return strOut;
}

