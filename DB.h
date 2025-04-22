#ifndef _DB_
#define _DB_
#include "mysql.h"


class DB {
public:
	MYSQL   mySQL;
        MYSQL_RES       *pResult;
        MYSQL_ROW       row;
		CString	username;
		CString	password;
		CString	hostname;
		CString	dbname;
	long	numFields;
	long	numRows;
	int	sqlTimings;
	long	numFetches;
	double	startTime;
	double	endTime;
	char	tempQuery[2046];
	char	errorMessage[255];

	int Connect();
	void Disconnect();
	char *getErrorMessage();
	int Query(char *);
	void EndQuery();
	MYSQL_ROW Fetch();
	DB();
	~DB();
};
#endif
