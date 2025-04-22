#ifdef WIN32
#include <afx.h>
#include <winsock2.h>
#endif
#include <mysql.h>
#include <stdio.h>
#include <stdlib.h>
#include "DB.h"
#include <string.h>
#ifdef WIN32
#include <sys/timeb.h>
#include <time.h>
#else
#include <sys/time.h>
#endif

double  DBGetCurrentTime() {
        double  thetime;
        struct timeval  tv;
#ifdef _WIN32
		time_t ltime;

		thetime = time(&ltime);
#else
        gettimeofday(&tv, NULL);
        thetime = (double)tv.tv_sec + (double)((double)tv.tv_usec / (double)1000000);
#endif
   
        return thetime;
}

DB::DB() {
	pResult = 0;
	sqlTimings = 0;
	memset(tempQuery, '\000', sizeof(tempQuery));
}
DB::~DB() {
	if (pResult) {
		mysql_free_result(pResult);
	}
}
char *DB::getErrorMessage() {
	return mysql_error(&mySQL);
}
int DB::Connect() {
    
	mysql_init(&mySQL);


    mysql_options(&mySQL,MYSQL_READ_DEFAULT_GROUP,"scastd");


    if (!mysql_real_connect(&mySQL,hostname,username,password,dbname,0,NULL,0)) {
       return(0);
    }
	return 1;
}
int DB::Query(char *query) {
	// double	startTime = 0.0;
	// double	endTime = 0.0;

	numFetches = 0;
	strcpy(tempQuery, query);
	if (sqlTimings) {
		startTime = DBGetCurrentTime();
	}
	if (pResult) {
		mysql_free_result(pResult);
	}
        if (mysql_query(&mySQL, query) != 0) {
                fprintf(stderr, "Misformed query (%s)\n", query);
                fprintf(stderr, "Error: %s\n", mysql_error(&mySQL));
                return(0);
        }

        pResult = mysql_store_result(&mySQL);
        if (pResult) {
		numFields = mysql_num_fields(pResult);
		numRows = mysql_num_rows(pResult);
        }
	if (sqlTimings) {
		endTime = DBGetCurrentTime();
	}

	if (sqlTimings) {
		fprintf(stdout, "Query %f secs - SQL: %s\n", endTime - startTime, query);
	}
	return(1);

}
MYSQL_ROW DB::Fetch() {
	if (pResult) {
		numFetches++;
		return mysql_fetch_row(pResult);
	}
	else {
		return 0;
	}
	return 0;
}

void DB::EndQuery() {
	if (sqlTimings) {
		endTime = DBGetCurrentTime();
	}
	if (sqlTimings) {
		fprintf(stdout, "Total Time Query (%d fetches) %f secs - SQL: %s\n", numFetches, endTime - startTime, tempQuery);
	}
}
void DB::Disconnect() {
	mysql_close(&mySQL);
	return;
}
