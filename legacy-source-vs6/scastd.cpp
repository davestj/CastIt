#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <afxinet.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifdef _WIN32
#include <afx.h>
#include <winsock.h>
#include <time.h>
#else
#include <sys/socket.h>
#include <signal.h>
#endif
#include <libxml/nanohttp.h>
#include <libxml/tree.h>
#include <libxml/parser.h>

#include <curl/curl.h>
#include <curl/easy.h>

#include "DB.h"

#include "common.h"
#include "log.h"


MYSQL_ROW    row;

int    paused = 0;
int    exiting = 0;

CString URLize(CString data)
{
    
    CString tempString;
    
    tempString = data;
    
    tempString.Replace("%", "%25");
    tempString.Replace(";", "%3B");
    tempString.Replace("/", "%2F");
    tempString.Replace("?", "%3F");
    tempString.Replace(":", "%3A");
    tempString.Replace("@", "%40");
    tempString.Replace("&", "%26");
    tempString.Replace("=", "%3D");
    tempString.Replace("+", "%2B");
    tempString.Replace(" ", "%20");
    tempString.Replace("\"", "%22");
    tempString.Replace("#", "%23");
    
    tempString.Replace("<", "%3C");
    tempString.Replace(">", "%3E");
    tempString.Replace("!", "%21");
    tempString.Replace("*", "%2A");
    tempString.Replace("'", "%27");
    tempString.Replace("(", "%28");
    tempString.Replace(")", "%29");
    tempString.Replace(",", "%2C");
    tempString.Replace("\r", "%0D");
    tempString.Replace("\n", "%0A");
    
    
    return tempString;
    
}

CString URLizeint(int data)
{
    CString    tempString;
    
    char    buf[25] = "";
    
    sprintf(buf, "%d", data);
    
    tempString = buf;
    
    return tempString;
}

void parseWebdata(xmlNodePtr cur)
{
    
    while (cur != NULL) {
        fprintf(stdout, "Webdata: %s\n", cur->name);
        cur = cur->next;
    }
    
}
void replaceQuote(char *buf) {
    for (char *p1 = buf;p1 < buf+strlen(buf); p1++) {
        if (*p1 == '\'') {
            *p1 = ' ';
        }
    }
}
void trimRight(char *buf) {
    for (char *p1 = buf + strlen(buf)-1;p1 > buf; p1--) {
        if (*p1 != ' ') {
            break;
        }
        else {
            *p1 = '\000';
        }
    }
}


MemoryStruct    Result;

size_t
URLCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
    register int realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)data;
    
    mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory) {
        memcpy(&(mem->memory[mem->size]), ptr, realsize);
        mem->size += realsize;
        mem->memory[mem->size] = 0;
    }
    return realsize;
}

char *URLCall(CString URL)
{
    CURL    *curl = NULL;
    
    curl = curl_easy_init();
    
    if (Result.memory) {
        free(Result.memory);
        Result.memory = NULL;
        Result.size = 0;
    }
    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, ".cookie.txt");
    curl_easy_setopt(curl, CURLOPT_COOKIEJAR, ".cookie.txt");
    curl_easy_setopt(curl, CURLOPT_URL, URL);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, URLCallback);
    curl_easy_setopt(curl, CURLOPT_USERAGENT,  "Mozilla/4.0 (compatible; CASTIT  ;)");
    curl_easy_setopt(curl, CURLOPT_FILE, (void *)&Result);
    
    int res = curl_easy_perform(curl);
    
    curl_easy_cleanup(curl);
    
    return Result.memory;
}
int getStats(CString serverURL, CString serverPassword, int serverType, CString serverMountpoint, serverData *pData, char *errorMessage)
{
    xmlDocPtr doc = NULL;
    
    int    ret = 0;
    char    *p1;
    xmlNodePtr cur;
    xmlNodePtr cur2;
    xmlNodePtr cur3;
    char    query[4096] = "";
    int    port = 0;
    int    sleeptime = 0;
    CString    fullURL;
    int srvcheck = 0;
    
    
    fullURL = serverURL;
    
    strcpy(pData->serverURL, serverURL);
    
    pData->serverType = serverType;
    if (serverType == 0) {
        CString tempString = "";
        
        tempString.Format("?pass=%s&mode=viewxml", serverPassword);
        fullURL += "/admin.cgi" + tempString;
    }
    if (serverType == 1) {
        fullURL.Replace("http://", "http://" + serverPassword + "@");
        fullURL += "/admin/stats.xml";
    }
    if (serverType == 2) {
        fullURL += "/status.xml";
    }
    
    char *buffer = URLCall(fullURL);
    
    if (buffer) {
        
        if (strstr(buffer, "<title>SHOUTcast Administrator</title>")) {
            
            sprintf(errorMessage, "Bad password\n(%s)", serverPassword);
            LogMessage(LOG_ERROR, errorMessage);
            return 0;
        }
        else {
            p1 = buffer;
            if (p1) {
                doc = xmlParseMemory(p1, strlen(p1));
                if (!doc) {
                    //commented out for nt service
                    sprintf(errorMessage, "Bad parse!", serverURL);
                    LogMessage(LOG_ERROR,"Bad xml parse");
                    return 0;
                }
                
                cur = xmlDocGetRootElement(doc);
                if (cur == NULL) {
                    //commented out for nt service
                    sprintf(errorMessage, "Empty Document!", serverURL);
                    LogMessage(LOG_ERROR,"Empty Document");
                    xmlFreeDoc(doc);
                    return 0;
                }
				else {//parse xml stats from servers
                    memset(pData->songTitle, '\000', sizeof(pData->songTitle));
                    cur = cur->xmlChildrenNode;
                    while (cur != NULL) {
						// This is an Icecast 2 XML doc
                        if (!xmlStrcmp(cur->name, (const xmlChar *) "source")) {
                            if (!xmlStrcmp(xmlGetProp(cur, (const xmlChar *) "mount"), (const xmlChar *)(LPCSTR)serverMountpoint)) {
                                cur2 = cur->xmlChildrenNode;
                                while (cur2 != NULL) {
                                    
                                    if (!xmlStrcmp(cur2->name, (const xmlChar *)"server_type")) {
                                        memset(pData->icecast2_stream_type, '\000', sizeof(pData->icecast2_stream_type));
                                        char *p1 = (char *)xmlNodeListGetString(doc, cur2->xmlChildrenNode, 1);
                                        if (p1) {
                                            strcpy(pData->icecast2_stream_type, p1);
                                        }
                                    }
                                    
                                    if (!xmlStrcmp(cur2->name, (const xmlChar *)"ice-bitrate")) {
                                        pData->icecast2_bitrate = atoi((char *)xmlNodeListGetString(doc, cur2->xmlChildrenNode,1));
                                    }
                                    
                                    if (!xmlStrcmp(cur2->name, (const xmlChar *)"bitrate")) {
                                        pData->icecast2_bitrate = atoi((char *)xmlNodeListGetString(doc, cur2->xmlChildrenNode,1));
                                    }
                                    
                                    if (!xmlStrcmp(cur2->name, (const xmlChar *)"ice-quality")) {
                                        pData->icecast2_quality = atoi((char *)xmlNodeListGetString(doc, cur2->xmlChildrenNode,1));
                                    }
                                    if (!xmlStrcmp(cur2->name, (const xmlChar *)"ice-channels")) {
                                        pData->icecast2_channels = atoi((char *)xmlNodeListGetString(doc, cur2->xmlChildrenNode,1));
                                    }
                                    if (!xmlStrcmp(cur2->name, (const xmlChar *)"ice-samplerate")) {
                                        pData->icecast2_samplerate = atoi((char *)xmlNodeListGetString(doc, cur2->xmlChildrenNode,1));
                                    }
                                    
                                    if (!xmlStrcmp(cur2->name, (const xmlChar *)"connections")) {
                                        pData->icecast2_connections = atoi((char *)xmlNodeListGetString(doc, cur2->xmlChildrenNode,1));
                                    }
                                    if (!xmlStrcmp(cur2->name, (const xmlChar *)"listeners")) {
                                        pData->icecast2_listeners = atoi((char *)xmlNodeListGetString(doc, cur2->xmlChildrenNode,1));
                                    }
                                    if (!xmlStrcmp(cur2->name, (const xmlChar *)"listener_peak")) {
                                        pData->icecast2_peak_listeners = atoi((char *)xmlNodeListGetString(doc, cur2->xmlChildrenNode,1));
                                    }
                                    
                                    
                                    
                                    
                                    if (!xmlStrcmp(cur2->name, (const xmlChar *)"server_description")) {
                                        memset(pData->icecast2_serverdescription, '\000', sizeof(pData->icecast2_serverdescription));
                                        char *p1 = (char *)xmlNodeListGetString(doc, cur2->xmlChildrenNode, 1);
                                        if (p1) {
                                            strcpy(pData->icecast2_serverdescription, p1);
                                        }
                                    }
                                    if (!xmlStrcmp(cur2->name, (const xmlChar *)"artist")) {
                                        char *p1 = (char *)xmlNodeListGetString(doc, cur2->xmlChildrenNode, 1);
                                        if (p1) {
                                            if (strlen(p1) > 0) {
                                                strcat(pData->songTitle, p1);
                                                strcat(pData->songTitle, " - ");
                                            }
                                        }
                                    }
                                    if (!xmlStrcmp(cur2->name, (const xmlChar *)"genre")) {
                                        char *p1 = (char *)xmlNodeListGetString(doc, cur2->xmlChildrenNode, 1);
                                        if (p1) {
                                            if (strlen(p1) > 0) {
                                                strcpy(pData->serverGenre, p1);
                                            }
                                        }
                                    }
                                    if (!xmlStrcmp(cur2->name, (const xmlChar *)"server_name")) {
                                        char *p1 = (char *)xmlNodeListGetString(doc, cur2->xmlChildrenNode, 1);
                                        if (p1) {
                                            if (strlen(p1) > 0) {
                                                strcpy(pData->streamName, p1);
                                            }
                                        }
                                    }
                                    if (!xmlStrcmp(cur2->name, (const xmlChar *)"title")) {
                                        char *p1 = (char *)xmlNodeListGetString(doc, cur2->xmlChildrenNode, 1);
                                        if (p1) {
                                            if (strlen(p1) > 0) {
                                                strcat(pData->songTitle, p1);
                                            }
                                        }
                                    }
                                    cur2 = cur2->next;
                                }
                            }
                        }
                        if (!xmlStrcmp(cur->name, (const xmlChar *) "CURRENTLISTENERS")) {
                            pData->currentListeners = atoi((char *)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));
                        }
                        if (!xmlStrcmp(cur->name, (const xmlChar *) "PEAKLISTENERS")) {
                            pData->peakListeners = atoi((char *)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));
                        }
                        if (!xmlStrcmp(cur->name, (const xmlChar *) "MAXLISTENERS")) {
                            pData->maxListeners = atoi((char *)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));
                        }
                        if (!xmlStrcmp(cur->name, (const xmlChar *) "REPORTEDLISTENERS")) {
                            pData->reportedListeners = atoi((char *)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));
                        }
                        if (!xmlStrcmp(cur->name, (const xmlChar *) "AVERAGETIME")) {
                            pData->avgTime = atoi((char *)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));
                        }
                        if (!xmlStrcmp(cur->name, (const xmlChar *) "WEBHITS")) {
                            pData->webHits = atoi((char *)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));
                        }
                        if (!xmlStrcmp(cur->name, (const xmlChar *) "STREAMHITS")) {
                            pData->streamHits = atoi((char *)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));
                        }
                        if (!xmlStrcmp(cur->name, (const xmlChar *) "BITRATE")) {
                            pData->bitrate = atoi((char *)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1));
                        }
                        
                        
                        if (!xmlStrcmp(cur->name, (const xmlChar *) "SERVERTITLE")) {
                            memset(pData->streamName, '\000', sizeof(pData->streamName));
                            char *p1 = (char *)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
                            if (p1) {
                                strcpy(pData->streamName, p1);
                            }
                        }
                        
                        if (!xmlStrcmp(cur->name, (const xmlChar *) "SERVERGENRE")) {
                            memset(pData->serverGenre, '\000', sizeof(pData->serverGenre));
                            char *p1 = (char *)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
                            if (p1) {
                                strcpy(pData->serverGenre, p1);
                            }
                        }
                        
                        if (!xmlStrcmp(cur->name, (const xmlChar *) "SONGTITLE")) {
                            memset(pData->songTitle, '\000', sizeof(pData->songTitle));
                            char *p1 = (char *)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
                            if (p1) {
                                strcpy(pData->songTitle, p1);
                            }
                        }
                        if (!xmlStrcmp(cur->name, (const xmlChar *) "CONTENT")) {
                            memset(pData->sc_content_type, '\000', sizeof(pData->sc_content_type));
                            char *p1 = (char *)xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
                            if (p1) {
                                strcpy(pData->sc_content_type, p1);
                            }
                        }
						// This is an Steamcast XML doc
                        if (!xmlStrcmp(cur->name, (const xmlChar *) "sources")) {
                            cur2 = cur->xmlChildrenNode;
                            while (cur2 != NULL) {
                                if (!xmlStrcmp(cur2->name, (const xmlChar *)"source")) {
									bool ourSource = false;
									cur3 = cur2->xmlChildrenNode;
									while (cur3 != NULL) {
				                        if (!xmlStrcmp(cur3->name, (const xmlChar *) "mount")) {
											char *p1 = (char *)xmlNodeListGetString(doc, cur3->xmlChildrenNode, 1);
											if (p1) {
												if (!strcmp(p1, (LPCSTR)serverMountpoint)) {
													ourSource = true;
													break;
												}
											}
										}
				                        cur3 = cur3->next;
									}
									if (ourSource) {
										cur3 = cur2->xmlChildrenNode;
										while (cur3 != NULL) {
											if (!xmlStrcmp(cur3->name, (const xmlChar *) "mount")) {
												char *p1 = (char *)xmlNodeListGetString(doc, cur3->xmlChildrenNode, 1);
												if (p1) {
												}
											}
											if (!xmlStrcmp(cur3->name, (const xmlChar *) "status")) {
												char *p1 = (char *)xmlNodeListGetString(doc, cur3->xmlChildrenNode, 1);
												if (p1) {
												}
											}
											if (!xmlStrcmp(cur3->name, (const xmlChar *) "name")) {
					                            memset(pData->streamName, '\000', sizeof(pData->streamName));
												char *p1 = (char *)xmlNodeListGetString(doc, cur3->xmlChildrenNode, 1);
												if (p1) {
					                                strcpy(pData->streamName, p1);
												}
											}
											if (!xmlStrcmp(cur3->name, (const xmlChar *) "description")) {
		                                        memset(pData->icecast2_serverdescription, '\000', sizeof(pData->icecast2_serverdescription));
												char *p1 = (char *)xmlNodeListGetString(doc, cur3->xmlChildrenNode, 1);
												if (p1) {
													strcpy(pData->icecast2_serverdescription, p1);
												}
											}
											if (!xmlStrcmp(cur3->name, (const xmlChar *) "genre")) {
					                            memset(pData->serverGenre, '\000', sizeof(pData->serverGenre));
												char *p1 = (char *)xmlNodeListGetString(doc, cur3->xmlChildrenNode, 1);
												if (p1) {
													strcpy(pData->serverGenre, p1);
												}
											}
											if (!xmlStrcmp(cur3->name, (const xmlChar *) "url")) {
					                            memset(pData->serverURL, '\000', sizeof(pData->serverURL));
												char *p1 = (char *)xmlNodeListGetString(doc, cur3->xmlChildrenNode, 1);
												if (p1) {
													strcpy(pData->serverURL, p1);
												}
											}
											if (!xmlStrcmp(cur3->name, (const xmlChar *) "bitrate")) {
												char *p1 = (char *)xmlNodeListGetString(doc, cur3->xmlChildrenNode, 1);
												if (p1) {
													pData->bitrate = atoi((char *)xmlNodeListGetString(doc, cur3->xmlChildrenNode, 1));
												}
											}
											if (!xmlStrcmp(cur3->name, (const xmlChar *) "meta_song")) {
					                            memset(pData->songTitle, '\000', sizeof(pData->songTitle));
												char *p1 = (char *)xmlNodeListGetString(doc, cur3->xmlChildrenNode, 1);
												if (p1) {
													strcpy(pData->songTitle, p1);
												}
											}
											if (!xmlStrcmp(cur3->name, (const xmlChar *) "meta_url")) {
												char *p1 = (char *)xmlNodeListGetString(doc, cur3->xmlChildrenNode, 1);
												if (p1) {
												}
											}
											if (!xmlStrcmp(cur3->name, (const xmlChar *) "nodes")) {
												char *p1 = (char *)xmlNodeListGetString(doc, cur3->xmlChildrenNode, 1);
												if (p1) {
												}
											}
											if (!xmlStrcmp(cur3->name, (const xmlChar *) "unique_nodes")) {
												char *p1 = (char *)xmlNodeListGetString(doc, cur3->xmlChildrenNode, 1);
												if (p1) {
													pData->currentListeners = atoi((char *)xmlNodeListGetString(doc, cur3->xmlChildrenNode, 1));
												}
											}
											if (!xmlStrcmp(cur3->name, (const xmlChar *) "max_nodes")) {
												char *p1 = (char *)xmlNodeListGetString(doc, cur3->xmlChildrenNode, 1);
												if (p1) {
													pData->maxListeners = atoi((char *)xmlNodeListGetString(doc, cur3->xmlChildrenNode, 1));
												}
											}
											if (!xmlStrcmp(cur3->name, (const xmlChar *) "average_connect_time")) {
												char *p1 = (char *)xmlNodeListGetString(doc, cur3->xmlChildrenNode, 1);
												if (p1) {
													pData->avgTime = atoi((char *)xmlNodeListGetString(doc, cur3->xmlChildrenNode, 1));
												}
											}
											if (!xmlStrcmp(cur3->name, (const xmlChar *) "tuneins")) {
												char *p1 = (char *)xmlNodeListGetString(doc, cur3->xmlChildrenNode, 1);
												if (p1) {
													pData->streamHits = atoi((char *)xmlNodeListGetString(doc, cur3->xmlChildrenNode, 1));
												}
											}
											if (!xmlStrcmp(cur3->name, (const xmlChar *) "connect_time")) {
												char *p1 = (char *)xmlNodeListGetString(doc, cur3->xmlChildrenNode, 1);
												if (p1) {
												}
											}
											if (!xmlStrcmp(cur3->name, (const xmlChar *) "bytes_recv")) {
												char *p1 = (char *)xmlNodeListGetString(doc, cur3->xmlChildrenNode, 1);
												if (p1) {
												}
											}
											if (!xmlStrcmp(cur3->name, (const xmlChar *) "bytes_sent")) {
												char *p1 = (char *)xmlNodeListGetString(doc, cur3->xmlChildrenNode, 1);
												if (p1) {
												}
											}
											if (!xmlStrcmp(cur3->name, (const xmlChar *) "content_type")) {
					                            memset(pData->sc_content_type, '\000', sizeof(pData->sc_content_type));
												char *p1 = (char *)xmlNodeListGetString(doc, cur3->xmlChildrenNode, 1);
												if (p1) {
													strcpy(pData->sc_content_type, p1);
												}
											}
											cur3 = cur3->next;
										}
									}
                                }
		                        cur2 = cur2->next;
							}
						}
                        cur = cur->next;
                    }
					
					    
						

                }//end parsing xml stats from servers
                if (strlen(pData->songTitle) > 0) {
                    trimRight(pData->songTitle);
                    replaceQuote(pData->songTitle);
                }
            }
            else {
                
                sprintf(errorMessage, "Bad data from %s\n", serverURL);
                LogMessage(LOG_ERROR,errorMessage);
                return 0;
            }
        }
    }
    //LOG AND SEND ERROR MESSAGES HERE IF SERVERS ARE DOWN
    else {
        sprintf(errorMessage, "SERVER %s IS DOWN\n", serverURL);
        LogMessage(LOG_ERROR,errorMessage);
        srvcheck = 1;
        
    }
    //END SENDING ERROR MESSAGES
    if (doc) {
        xmlFreeDoc(doc);
    }
    return 1;
}

int deleteOldHistory(DB *db, CString tablePrefix, CString serverURL, serverData *pData, int age)
{
    
    char    query[4096] = "";
    long    totalcurrentlisteners = 0;
    long    totalpeaklisteners = 0;
    long    totalmaxlisteners = 0;
    long    totalaveragetime = 0;
    long    totalstreamhits = 0;
    
    double    avgcurrentlisteners = 0.0;
    double    avgpeaklisteners = 0.0;
    double    avgmaxlisteners = 0.0;
    double    avgaveragetime = 0.0;
    double    avgstreamhits = 0.0;
    
    db->Connect();
    
    
    sprintf(query, "delete from %ssonginfo where time < NOW() - INTERVAL %d HOUR and serverURL = '%s'", tablePrefix, age, serverURL);
    if (!db->Query(query)) {
        char    buf[1024] = "";
        sprintf(buf, "MySQL Error (%s)", db->getErrorMessage());
        LogMessage(LOG_ERROR, buf);
    }
    db->Disconnect();
    return 0;
    
}
int aggregateHistory(DB *db, CString tablePrefix, CString serverURL, serverData *pData, int age)
{
    
    char    query[4096] = "";
    long    totalcurrentlisteners = 0;
    long    totalconnections = 0;
    long    totalpeaklisteners = 0;
    long    totalmaxlisteners = 0;
    long    totalaveragetime = 0;
    long    totalstreamhits = 0;
    
    double    avgcurrentlisteners = 0.0;
    double  avgconnections = 0.0;
    double    avgpeaklisteners = 0.0;
    double    avgmaxlisteners = 0.0;
    double    avgaveragetime = 0.0;
    double    avgstreamhits = 0.0;
    
    db->Connect();
    
    //aggregate shoutcast history (trouble area)
    if(pData->serverType == 0){
        sprintf(query, "select currentlisteners, peaklisteners, maxlisteners, averagetime, streamhits from %sshoutcast_serverinfo where time < NOW() - INTERVAL %d HOUR and serverURL = '%s'", tablePrefix, age, serverURL);
        db->Query(query);
        int loop = 1;
        int    numrows = 0;
        while (loop) {
            row = db->Fetch();
            if (row) {
                totalcurrentlisteners += atoi(row[0]);
                totalpeaklisteners += atoi(row[1]);
                totalmaxlisteners += atoi(row[2]);
                totalaveragetime += atoi(row[3]);
                totalstreamhits += atoi(row[4]);
                numrows++;
                }else {
                loop = 0;
            }
        }
        
        if (numrows > 0) {
            avgcurrentlisteners = totalcurrentlisteners / numrows;
            avgpeaklisteners = totalpeaklisteners / numrows;
            avgmaxlisteners = totalmaxlisteners / numrows;
            avgaveragetime = totalaveragetime / numrows;
            avgstreamhits = totalstreamhits / numrows;
            sprintf(query, "insert into %sshoutcast_history (serverURL, avgcurrentlisteners, avgpeaklisteners, avgmaxlisteners, avgaveragetime, avgstreamhits, time) values('%s', %d, %d, %d, %d, %d, NULL)", tablePrefix, serverURL, (int)avgcurrentlisteners, (int)avgpeaklisteners, (int)avgmaxlisteners, (int)avgaveragetime, (int)avgstreamhits);
            if (!db->Query(query)) {
                char    buf[1024] = "";
                
                sprintf(buf, "MySQL Error (%s)", db->getErrorMessage());
                LogMessage(LOG_ERROR, buf);
                }else {
                
                sprintf(query, "delete from %sshoutcast_serverinfo where time < NOW() - INTERVAL %d HOUR and serverURL = '%s'", tablePrefix, age, serverURL);
                if (!db->Query(query)) {
                    char    buf[1024] = "";
                    
                    sprintf(buf, "MySQL Error (%s)", db->getErrorMessage());
                    LogMessage(LOG_ERROR, buf);
                }
            }
        }
    }
    
    //aggeragate icecast2 history (trouble area)
    if(pData->serverType == 1){
        sprintf(query, "select ice2_cl, ice2_cons from %sicecast2_serverinfo where time < NOW() - INTERVAL %d HOUR and server_url = '%s'", tablePrefix, age, serverURL);
        db->Query(query);
        int loop = 1;
        int    numrows = 0;
        while (loop) {
            row = db->Fetch();
            if (row) {
                totalcurrentlisteners += atoi(row[0]);
                totalconnections += atoi(row[1]);
                numrows++;
            }
            else {
                loop = 0;
            }
        }
        
        if (numrows > 0) {
            avgcurrentlisteners = totalcurrentlisteners / numrows;
            avgconnections = totalconnections / numrows;
            sprintf(query, "insert into %sicecast2_history (serverURL, avgcurrentlisteners, avgconnections, time) values('%s', %d, %d, NULL)", tablePrefix, serverURL, (int)avgcurrentlisteners, (int)avgconnections);
            if (!db->Query(query)) {
                char    buf[1024] = "";
                
                sprintf(buf, "MySQL Error (%s)", db->getErrorMessage());
                LogMessage(LOG_ERROR, buf);
            }
            else {
                
                sprintf(query, "delete from %sicecast2_serverinfo where time < NOW() - INTERVAL %d HOUR and server_url = '%s'", tablePrefix, age, serverURL);
                if (!db->Query(query)) {
                    char    buf[1024] = "";
                    
                    sprintf(buf, "MySQL Error (%s)", db->getErrorMessage());
                    LogMessage(LOG_ERROR, buf);
                }
            }
        }
    }
    db->Disconnect();
    return 0;
}
int insertIntoMySQL(DB *db, CString tablePrefix, CString serverURL, serverData *pData)
{
    char    query[4096] = "";
    int    insert_flag = 0;
    
    db->Connect();
    
    sprintf(query, "select songTitle from %ssonginfo where serverURL = '%s' order by time desc limit 1", tablePrefix, serverURL);
    db->Query(query);
    insert_flag = 0;
    row = db->Fetch();
    if (row) {
        if (!strcmp(pData->songTitle, row[0])) {
            insert_flag = 0;
        }
        else {
            insert_flag = 1;
        }
    }
    else {
        insert_flag = 1;
    }
    if (insert_flag) {
        if(pData->serverType == 0){
            sprintf(query, "insert into %ssonginfo values('%s', '%s', %d, NULL)", tablePrefix, serverURL, pData->songTitle, pData->currentListeners);
            if (!db->Query(query)) {
                char    buf[1024] = "";
                sprintf(buf, "MySQL Error (%s)", db->getErrorMessage());
                LogMessage(LOG_ERROR, buf);
            }
            
        }
        if(pData->serverType == 1){
            sprintf(query, "insert into %ssonginfo values('%s', '%s', %d, NULL)", tablePrefix, serverURL, pData->songTitle, pData->icecast2_listeners);
            if (!db->Query(query)) {
                char    buf[1024] = "";
                sprintf(buf, "MySQL Error (%s)", db->getErrorMessage());
                LogMessage(LOG_ERROR, buf);
            }
            
        }
        
    }
    if (pData->serverType == 0) {
        
        sprintf(query, "insert into %sshoutcast_serverinfo (serverURL, currentlisteners, peaklisteners, maxlisteners, averagetime, streamhits, bitrate, time) values('%s', %d, %d, %d, %d, %d, %d, NULL)", tablePrefix, serverURL, pData->currentListeners, pData->peakListeners, pData->maxListeners, pData->avgTime, pData->streamHits, pData->bitrate);
        
        if (!db->Query(query)) {
            char    buf[1024] = "";
            
            sprintf(buf, "MySQL Error (%s)", db->getErrorMessage());
            LogMessage(LOG_ERROR, buf);
        }
        
    }
    if (pData->serverType == 1) {
        sprintf(query, "insert into %sicecast2_serverinfo (server_name, current_song, server_url, ice2_br, ice2_ql, ice2_chn, ice2_sr, ice2_cons, ice2_cl, ice2_strtype, ice2_desc) values('%s', '%s', '%s', %d, %d, %d, %d, %d, %d, '%s', '%s')", tablePrefix, pData->streamName, pData->songTitle, serverURL, pData->icecast2_bitrate, pData->icecast2_quality, pData->icecast2_channels, pData->icecast2_samplerate, pData->icecast2_connections, pData->icecast2_listeners, pData->icecast2_stream_type, pData->icecast2_serverdescription);
        if(!db->Query(query)) {
            char    buf[1024] = "";
            sprintf(buf, "MySQL Error (%s)", db->getErrorMessage());
            LogMessage(LOG_ERROR, buf);
        }
        
    }
    db->Disconnect();
    return 1;
}
