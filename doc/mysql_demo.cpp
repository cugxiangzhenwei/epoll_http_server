#include<iostream>  
#include "mysql_driver.h"  
#include "mysql_connection.h"  
#include <cppconn/driver.h>  
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h> 
#include<string.h>
#include <fstream>
using namespace std;  

class DataBuf : public std::streambuf
{
public:
	DataBuf(char* d, size_t s)
	{
		setg(d, d, d+s);
	}
};
 
void RunConnectMySQL()  
{  
    sql::mysql::MySQL_Driver *driver;  
    sql::Connection *con;  
  
    driver = sql::mysql::get_mysql_driver_instance();  
    con = driver->connect("tcp://127.0.0.1:3306", "root", "1005150782");  
    con->setSchema("Cug");
{
	sql::Statement *stmt = con->createStatement();
	stmt->execute("SET NAMES GBK");
	stmt->execute("create table if not exists PhotoBook(\
				  ID INT(9) NOT NULL AUTO_INCREMENT,\
				  NAME VARCHAR(255),\
				  SEX CHAR(4) DEFAULT 'm',\
				  AGE INT(3) DEFAULT 0,\
				  PHONE_NUM CHAR(11) NOT NULL,\
				  LOCATION  VARCHAR(255) DEFAULT NULL,\
				  RELATION  VARCHAR(20) DEFAULT NULL,\
				  PHOTO   MEDIUMBLOB,\
				  PRIMARY KEY(ID)\
				  )");
	delete stmt;  
	sql::PreparedStatement *pstmt = NULL;
	pstmt = con->prepareStatement("INSERT INTO PhotoBook(NAME,SEX,AGE,PHONE_NUM,LOCATION,RELATION,PHOTO) VALUES (?,?,?,?,?,?,?)");
	pstmt->setString(1, "xiangzhenwei");
	pstmt->setString(2, "man");
	pstmt->setInt(3, 29);
	pstmt->setString(4,"13488751151");
	pstmt->setString(5,"北京市昌平区");
	pstmt->setString(6,"本人");
	char pData[20]= "9876543210";
	int iLen = strlen(pData);
	if (iLen >0)
	{
		DataBuf buffer(pData,iLen);
		std::istream s(&buffer);
		pstmt->setBlob(7, &s);
		int iRev = pstmt->executeUpdate();
		printf("executeUpdate set blob data return %d\n",iRev);
	}
	else
	{
		pstmt->setBlob(7, NULL);
		int iRev = pstmt->executeUpdate();
		printf("executeUpdate set blob to null eturn %d\n",iRev);
	}
	delete pstmt;
}
{
	sql::Statement *stmt = NULL;
	stmt = con->createStatement();
	char strSQl[1024];
	sprintf(strSQl,"UPDATE PhotoBook SET NAME='%s', LOCATION='%s', RELATION='%s' \
		WHERE ID = 1","zhenwei","BeiJing ChangPing","myself");
	bool bFaild = stmt->execute(strSQl);
	printf("execute return %d\n",bFaild);
	delete stmt;
}
{
	printf("query begin call..\n");
	sql::PreparedStatement *pstmt = NULL;
	pstmt = con->prepareStatement("SELECT * FROM PhotoBook ORDER BY ID ASC");
	sql::ResultSet *m_pres;
	m_pres = pstmt->executeQuery();
	m_pres->beforeFirst();
	while(m_pres->next())
	{
   		std::string m_strName = m_pres->getString("NAME").c_str();
		std::string 	m_strSex = m_pres->getString("SEX").c_str();
		int  m_iAge = m_pres->getInt("AGE");
		std::string 	m_strPhoneNumber = m_pres->getString("PHONE_NUM").c_str();
		std::string 	m_strLocation = m_pres->getString("LOCATION").c_str();
		std::string 	m_strRelation = m_pres->getString("RELATION").c_str();
		std::istream  * pstream = m_pres->getBlob("PHOTO");
		if (pstream==NULL) return;
		istreambuf_iterator<char> beg(*pstream),eos;                                                 
		string  data(beg,eos);
		printf("name=%s,sex=%s,age=%d,phontnum=%s,location=%s,relation=%s,photo=%s\n",m_strName.c_str(),
		m_strSex.c_str(),m_iAge,m_strPhoneNumber.c_str(),m_strLocation.c_str(),m_strRelation.c_str(),
		data.c_str());
	}
	delete pstmt;
}
delete con;  
}  
  
int main(void)  
{  
    RunConnectMySQL();  
    return 0;  
}  

