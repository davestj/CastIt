#ifndef _COMMON_H
#define _COMMON_H
#include "DB.h"
typedef struct tagServerData {
	int serverType;
	int	currentListeners;
	int	peakListeners;
	int	maxListeners;
	int	reportedListeners;
	int	avgTime;
	int	webHits;
	int	streamHits;
	int	bitrate;
	int icecast2_bitrate;
	int icecast2_channels;
	int icecast2_quality;
	int icecast2_samplerate;
	int icecast2_connections;
	int icecast2_listeners;
	int icecast2_peak_listeners;
	char	icecast2_stream_type[1024];
	char	icecast2_serverdescription[1024];
	char    serverGenre[1024];
	char	songTitle[1024];
	char	streamName[1024];
	char	serverURL[1024];
	char sc_content_type[1024];


public:
	int scbandwidth;
	int sc_peakbw;
	int icecast2_bandwidth;
	int icecast2_peakbw;
	int sc_cur_bw_mbps;
	int sc_peak_bw_mbps;
	int icecast2_cur_bw_mbps;
	int icecast2_peak_bw_mbps;

} serverData;
struct MemoryStruct {
        char *memory;
        size_t size;
};

char *URLCall(CString URL);
int aggregateHistory(DB *db, CString tablePrefix, CString serverURL, serverData *pData, int age);
int deleteOldHistory(DB *db, CString tablePrefix, CString serverURL, serverData *pData, int age);
int getStats(CString serverURL, CString serverPassword, int serverType,CString serverMountpoint, serverData *pData, char *errorMessage);
int insertIntoMySQL(DB *db, CString tablePrefix, CString serverURL, serverData *pData);
CString URLize(CString data);
CString URLizeint(int data);
#endif
