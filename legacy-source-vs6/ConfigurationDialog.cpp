// ConfigurationDialog.cpp : implementation file
//

#include "stdafx.h"
#include "CastIt.h"
#include "CastItDlg.h"
#include "ConfigurationDialog.h"
#include "common.h"
#include "log.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



/////////////////////////////////////////////////////////////////////////////
// CConfigurationDialog dialog


CConfigurationDialog::CConfigurationDialog(CWnd* pParent /*=NULL*/)
	: CDialog(CConfigurationDialog::IDD, pParent)
{
	//{{AFX_DATA_INIT(CConfigurationDialog)
	m_Database = _T("");
	m_Hostname = _T("");
	m_Password = _T("");
	m_TablePrefix = _T("");
	m_Username = _T("");
	m_MysqlFlag = FALSE;
	m_Aggregate = 0;
	m_DeleteSongHistoryInterval = 0;
	m_PollInterval = 0;
	m_URL = _T("");
	m_CallURL = FALSE;
	m_TemplateIn = _T("");
	m_TemplateOut = _T("");
	m_GenerateTemplate = FALSE;
	//}}AFX_DATA_INIT


}


void CConfigurationDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CConfigurationDialog)
	DDX_Control(pDX, IDC_DELETE_SONGHISTORY_INTERVAL, m_DeleteSongCtl);
	DDX_Control(pDX, IDC_AGGREGATE, m_AggregateCtl);
	DDX_Control(pDX, IDC_CREATE_TABLES, m_CreatCtl);
	DDX_Control(pDX, IDC_VERIFY_CONNECTION, m_VerifyCtl);
	DDX_Control(pDX, IDC_MY_USERNAME, m_UsernameCtl);
	DDX_Control(pDX, IDC_MY_TABLEPREFIX, m_TablePrefixCtl);
	DDX_Control(pDX, IDC_MY_PASSWORD, m_PasswordCtl);
	DDX_Control(pDX, IDC_MY_HOSTNAME, m_HostnameCtl);
	DDX_Control(pDX, IDC_MY_DATABASE, m_DatabaseCtl);
	DDX_Text(pDX, IDC_MY_DATABASE, m_Database);
	DDX_Text(pDX, IDC_MY_HOSTNAME, m_Hostname);
	DDX_Text(pDX, IDC_MY_PASSWORD, m_Password);
	DDX_Text(pDX, IDC_MY_TABLEPREFIX, m_TablePrefix);
	DDX_Text(pDX, IDC_MY_USERNAME, m_Username);
	DDX_Check(pDX, IDC_MYSQL_FLAG, m_MysqlFlag);
	DDX_Text(pDX, IDC_AGGREGATE, m_Aggregate);
	DDX_Text(pDX, IDC_DELETE_SONGHISTORY_INTERVAL, m_DeleteSongHistoryInterval);
	DDX_Text(pDX, IDC_POLLINTERVAL, m_PollInterval);
	DDX_Text(pDX, IDC_URL, m_URL);
	DDX_Check(pDX, IDC_CALLURL, m_CallURL);
	DDX_Text(pDX, IDC_TEMPLATEIN, m_TemplateIn);
	DDX_Text(pDX, IDC_TEMPLATEOUT, m_TemplateOut);
	DDX_Check(pDX, IDC_GENERATETEMPLATE, m_GenerateTemplate);
//	DDX_Check(pDX, IDC_UPDATE_CC, m_UpdateCC);
	DDX_Control(pDX, IDC_VIEWHELP1, m_btnHelp1);
	DDX_Control(pDX, IDC_VIEWHELP2, m_btnHelp2);
	DDX_Control(pDX, IDC_VIEWHELP3, m_btnHelp3);
	DDX_Control(pDX, IDC_VIEWHELP4, m_btnHelp4);
	DDX_Control(pDX, IDC_VIEWHELP5, m_btnHelp5);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConfigurationDialog, CDialog)
	//{{AFX_MSG_MAP(CConfigurationDialog)
	ON_BN_CLICKED(IDC_MYSQL_FLAG, OnMysqlFlag)
	ON_BN_CLICKED(IDC_VERIFY_CONNECTION, OnVerifyConnection)
	ON_BN_CLICKED(IDC_CREATE_TABLES, OnCreateTables)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_BROWSE1, OnBrowse1)
	ON_BN_CLICKED(IDC_BROWSE2, OnBrowse2)
	ON_BN_CLICKED(IDC_VIEWCGI, OnViewcgi)
	ON_BN_CLICKED(IDC_FTPINFO, OnFtpinfo)
	ON_BN_CLICKED(IDC_VIEWHELP1, OnViewHelpTip1)
	ON_BN_CLICKED(IDC_VIEWHELP2, OnViewHelpTip2)
	ON_BN_CLICKED(IDC_VIEWHELP3, OnViewHelpTip3)
    ON_BN_CLICKED(IDC_VIEWHELP4, OnViewHelpTip4)
	ON_BN_CLICKED(IDC_VIEWHELP5, OnViewHelpTip5)



//	ON_BN_CLICKED(IDC_CC_CHECK, OnCcCheck)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConfigurationDialog message handlers

void CConfigurationDialog::OnMysqlFlag()
{
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	if (m_MysqlFlag) {
		m_UsernameCtl.EnableWindow(TRUE);
		m_PasswordCtl.EnableWindow(TRUE);
		m_HostnameCtl.EnableWindow(TRUE);
		m_DatabaseCtl.EnableWindow(TRUE);
		m_DeleteSongCtl.EnableWindow(TRUE);
		m_AggregateCtl.EnableWindow(TRUE);
		m_TablePrefixCtl.EnableWindow(TRUE);
		m_VerifyCtl.EnableWindow(TRUE);
		m_CreatCtl.EnableWindow(TRUE);
	}
	else {
		m_UsernameCtl.EnableWindow(FALSE);
		m_PasswordCtl.EnableWindow(FALSE);
		m_HostnameCtl.EnableWindow(FALSE);
		m_DatabaseCtl.EnableWindow(FALSE);
		m_DeleteSongCtl.EnableWindow(FALSE);
		m_AggregateCtl.EnableWindow(FALSE);
		m_TablePrefixCtl.EnableWindow(FALSE);
		m_VerifyCtl.EnableWindow(FALSE);
		m_CreatCtl.EnableWindow(FALSE);
	}
}

void CConfigurationDialog::OnVerifyConnection()
{
	CWaitCursor	wait;
	// TODO: Add your control notification handler code here
	UpdateData(TRUE);
	database.username = m_Username;
	database.password = m_Password;
	database.hostname = m_Hostname;
	database.dbname = m_Database;

	if (database.Connect()) {
		MessageBox("Successful connection", NULL, MB_ICONINFORMATION);
        LogMessage(LOG_INFO,"MySQL Connection Successfull");
	}
	else {
		char	buf[255];
		if (!strncmp(database.getErrorMessage(), "Unknown database", strlen("Unknown database"))) {
			sprintf(buf, "Connectivity validated, but database %s doesn't seem to exist, do you want it created ?", m_Database);
			if (MessageBox(buf, NULL, MB_YESNO) == IDYES) {
				database.dbname = "";
				if (!database.Connect()) {
					sprintf(buf, "Sorry, but having major problems doing that..(%s)", database.getErrorMessage());
				  LogMessage(LOG_INFO, buf);
					MessageBox(buf, NULL, MB_ICONERROR);
				}
				else {
					sprintf(buf, "create database %s", m_Database);
					LogMessage(LOG_INFO, buf);
					if (!database.Query(buf)) {
						sprintf(buf, "Sorry, but having major problems doing that..(%s)", database.getErrorMessage());
					LogMessage(LOG_INFO, buf);
					}
					else {
						sprintf(buf, "Database created..(%s)", m_Database);
						LogMessage(LOG_INFO, buf);
					}
					database.dbname = m_Database;
					MessageBox(buf, NULL, MB_ICONINFORMATION);
				}

			}
		}
		else {
			sprintf(buf, "Error verifying connection (%s)", database.getErrorMessage());
			LogMessage(LOG_INFO, buf);
			MessageBox(buf, NULL, MB_ICONERROR);
		}
	}
	database.Disconnect();
}

void CConfigurationDialog::OnCreateTables()
{
	// TODO: Add your control notification handler code here


	char	sql1[1024] = "";
	char	sql2[1024] = "";
	char	sql3[1024] = "";
	char	sql4[1024] = "";
	char	sql5[1024] = "";
	CString	message = "";

	UpdateData(TRUE);

	database.username = m_Username;
	database.password = m_Password;
	database.hostname = m_Hostname;
	database.dbname = m_Database;

	sprintf(sql1, "CREATE TABLE IF NOT EXISTS %sshoutcast_serverinfo ( \
		serverURL varchar(255) default NULL,\
		currentlisteners int(11) default NULL,\
		peaklisteners int(11) default NULL,\
		maxlisteners int(11) default NULL,\
		averagetime int(11) default NULL,\
		streamhits int(11) default NULL,\
		bitrate int(11) default NULL,\
		time timestamp(14) NOT NULL)", m_TablePrefix);



	sprintf(sql2, "CREATE TABLE IF NOT EXISTS %ssonginfo (\
		serverURL varchar(255) default NULL, \
		songTitle varchar(255) default NULL, \
		currentlisteners int(11) default NULL,\
		time timestamp(14) NOT NULL)", m_TablePrefix);

	sprintf(sql3, "CREATE TABLE IF NOT EXISTS %sshoutcast_history ( \
		serverURL varchar(255) default NULL,\
		avgcurrentlisteners int(11) default NULL,\
		avgpeaklisteners int(11) default NULL,\
		avgmaxlisteners int(11) default NULL,\
		avgaveragetime int(11) default NULL,\
		avgstreamhits int(11) default NULL,\
		time timestamp(14) NOT NULL)", m_TablePrefix);

	sprintf(sql4, "CREATE TABLE IF NOT EXISTS %sicecast2_history ( \
		serverURL varchar(255) default NULL,\
		avgcurrentlisteners int(11) default NULL,\
		avgconnections int(11) default NULL,\
		time timestamp(14) NOT NULL)", m_TablePrefix);

	sprintf(sql5, "CREATE TABLE IF NOT EXISTS %sicecast2_serverinfo ( \
		server_name varchar(255) default NULL,\
		current_song varchar(255) default NULL,\
		server_url varchar(255) default NULL,\
		ice2_br int(11) default NULL,\
		ice2_ql int(11) default NULL,\
		ice2_chn int(11) default NULL,\
		ice2_sr int(11) default NULL,\
		ice2_cons int(11) default NULL,\
		ice2_cl int(11) default NULL,\
		ice2_strtype varchar(255) default NULL,\
		ice2_desc varchar(255) default NULL,\
		time timestamp(14) NOT NULL)", m_TablePrefix);

	if (!database.Connect()) {
        LogMessage(LOG_INFO, "Cannot connect to DB, check your settings");
		MessageBox("Cannot connect to DB, check your settings", NULL, MB_ICONERROR);
	}
	else {
		if (database.Query(sql1)) {
			message += "Table (" + m_TablePrefix + "shoutcast_serverinfo) created OK\r\n";
		}else{
            message += "Table (" + m_TablePrefix + "shoutcast_serverinfo) creation FAILED (" + database.getErrorMessage() + ")\r\n";
		}
		if (database.Query(sql2)) {
			message += "Table (" + m_TablePrefix + "songinfo) created OK\r\n";
		}else {
			message += "Table (" + m_TablePrefix + "songinfo) creation FAILED (" + database.getErrorMessage() + ")\r\n";
			}
		if (database.Query(sql3)) {
			message += "Table (" + m_TablePrefix + "shoutcast_history) created OK\r\n";
		}else{
			message += "Table (" + m_TablePrefix + "shoutcast_history) creation FAILED (" + database.getErrorMessage() + ")\r\n";
		}
		if (database.Query(sql4)) {
			message += "Table (" + m_TablePrefix + "icecast2_history) created OK\r\n";
		}else{
			message += "Table (" + m_TablePrefix + "icecast2_history) creation FAILED (" + database.getErrorMessage() + ")\r\n";
		}
		if (database.Query(sql5)) {
			message += "Table (" + m_TablePrefix + "icecast2_serverinfo) created OK\r\n";
		}else {
			message += "Table (" + m_TablePrefix + "icecast2_serverinfo) creation FAILED (" + database.getErrorMessage() + ")\r\n";
				}
			}

		MessageBox(message, NULL, MB_ICONINFORMATION);
	}


void CConfigurationDialog::OnClose()
{
	// TODO: Add your message handler code here and/or call default

	CDialog::OnClose();


}

BOOL CConfigurationDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	short	shBtnColor = 30;

	m_btnHelp1.SetIcon(IDI_HELP);
	m_btnHelp1.SetFlat(TRUE);
//	m_btnHelp1.OffsetColor(CButtonST::BTNST_COLOR_BK_IN, shBtnColor);


	m_btnHelp2.SetIcon(IDI_HELP);
	m_btnHelp2.SetFlat(TRUE);
//	m_btnHelp2.OffsetColor(CButtonST::BTNST_COLOR_BK_IN, shBtnColor);


	m_btnHelp3.SetIcon(IDI_HELP);
	m_btnHelp3.SetFlat(TRUE);
//	m_btnHelp3.OffsetColor(CButtonST::BTNST_COLOR_BK_IN, shBtnColor);


	m_btnHelp4.SetIcon(IDI_HELP);
	m_btnHelp4.SetFlat(TRUE);
//	m_btnHelp4.OffsetColor(CButtonST::BTNST_COLOR_BK_IN, shBtnColor);


	m_btnHelp5.SetIcon(IDI_HELP);
	m_btnHelp5.SetFlat(TRUE);
//	m_btnHelp5.OffsetColor(CButtonST::BTNST_COLOR_BK_IN, shBtnColor);

	// TODO: Add extra initialization here
	database.username = m_Username;
	database.password = m_Password;
	database.hostname = m_Hostname;
	database.dbname = m_Database;

	OnMysqlFlag();
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE


}

void CConfigurationDialog::OnBrowse1()
{
	// TODO: Add your control notification handler code here
	CFileDialog	fileDialog(TRUE,"TXT",NULL,OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,"HTML files (*.html)|*.html|PHP Files (*.php)|*.php|ASP Files (*.asp)|*asp|Perl Files (*.pl)|*.pl|CGI Files (*.cgi)|*.cgi|SSI Files (*.shtml)|*.shtml|Java Script (*.js)|*.js|XML Files (*.xml)|*.xml|All Files (*.*)|*.*||");

	if (fileDialog.DoModal() == IDOK) {
		m_TemplateIn = fileDialog.GetPathName();
	}
	UpdateData(FALSE);

}

void CConfigurationDialog::OnBrowse2()
{
	// TODO: Add your control notification handler code here
	CFileDialog	fileDialog(TRUE,"TXT",NULL,OFN_PATHMUSTEXIST,"HTML files (*.html)|*.html|PHP Files (*.php)|*.php|ASP Files (*.asp)|*.asp|Perl Files (*.pl)|*.pl|CGI Files (*.cgi)|*.cgi|SSI Files (*.shtml)|*.shtml|Java Script (*.js)|*.js|XML Files (*.xml)|*.xml|All Files (*.*)|*.*||");
	if (fileDialog.DoModal() == IDOK) {
		m_TemplateOut = fileDialog.GetPathName();
	}
	UpdateData(FALSE);


}

void CConfigurationDialog::OnViewcgi()
{
	// TODO: Add your control notification handler code here
	CString message = "";

	message += "The following parameters are taken from the xml interface of\n";
	message += "either the shoutcast or icecast2 servers.\n";
    message += "Note:The cgi parameters are also known as variables\n";
	message += "and are utilized as $st, $sn etc....\n";
	message += "st = Server Type\n";
	message += "sn = SHOUTcast/Icecast2 Server Name\n";
	message += "cs = SHOUTcast/Icecast2 Current Song\n";
	message += "shout_cl = SHOUTcast: current listeners\n";
	message += "shout_ml = SHOUTcast: max listeners\n";
	message += "shout_pl = SHOUTcast: peak listeners\n";
	message += "shout_at = SHOUTcast: average listen time\n";
    message += "sg = SHOUTcast: server genre\n";
	message += "shout_wh = SHOUTcast: web hits\n";
	message += "shout_sh = SHOUTcast: stream hits\n";
	message += "su = SHOUTcast/Icecast2: server URL\n";
	message += "shout_br = SHOUTcast: bitrate\n";
	message += "ice2_br = Icecast2: stream bitrate (mp3)\n";
	message += "ice2_ql = Icecast2: stream quality (ogg vorbis)\n";
	message += "ice2_chn = Icecast2: stream channels (ogg vorbis/mp3)\n";
	message += "ice2_sr = Icecast2: stream samplerate (ogg vorbis/mp3)\n";
	message += "ice2_cons = Icecast2: mount point connections\n";
	message += "ice2_cl = Icecast2: mount point listeners\n";
	message += "ice2_strtype = Icecast2: stream type\n";
	message += "ice2_yptouch = Icecast2: Last touch time to yp directory\n";
	message += "ice2_desc = Icecast2: mount point description\n";


	MessageBox(message, NULL, MB_ICONINFORMATION);
}



void CConfigurationDialog::OnFtpinfo()
{
	// TODO: Add your control notification handler code here
	if (ftpInfoDialog.DoModal() == IDOK) {
		;
	}
}


/*
void CConfigurationDialog::OnCcCheck()
{
	// TODO: Add your control notification handler code here
	CString	CCURL = "http://yp.casterclub.com/touch.php?ypa=touch";
//	CCURL += "id=" + URLize(m_CC_User);
//	CCURL += "&userpass=" + URLize(m_CC_Password);
//	CCURL += "&cs_login=cs_auth";

	char *buffer = URLCall(CCURL);
	if (buffer) {
	MessageBox("Touch Message:\r\n" + CString(buffer), NULL, MB_ICONINFORMATION);

		}
	else {
		//commented out for nt service
	//	MessageBox("Internal error", NULL, MB_ICONERROR);
	}

}
*/
void CConfigurationDialog::OnViewHelpTip1()
{

			short	shBtnColor = 30;
	m_btnHelp1.SetIcon(IDI_HELP);
	m_btnHelp1.SetFlat(TRUE);
//	m_btnHelp1.OffsetColor(CButtonST::BTNST_COLOR_BK_IN, shBtnColor);


	CString message = "";

	message += "This is the help tip box.\n";
	message += "To view information for a paticular setting\n";
	message += "just click the ? icon.\n";


	MessageBox(message, NULL, MB_ICONINFORMATION);

}

void CConfigurationDialog::OnViewHelpTip2()
{

		short	shBtnColor = 30;
	m_btnHelp2.SetIcon(IDI_HELP);
	m_btnHelp2.SetFlat(TRUE);
//	m_btnHelp2.OffsetColor(CButtonST::BTNST_COLOR_BK_IN, shBtnColor);

		CString message = "";

	message += "Mysql Tip\n";
	message += "It is recomended that you use a local mysql server\n";
    message += "this will speed up the process of gathering stats.\n";
    message += "Tips:\n";
    message += " Number of hours to aggregate history will enable you\n";
    message += " to monitor peak times, good setting for this is 1 (every hour)\n";
    message += " Enabling this will also get avarage stats for the following\n";
	message += "  -Listeners\n";
	message += "  -Peaklisteners\n";
	message += "  -Maxlisteners\n";
	message += "  -Averagelistentime (in seconds)\n";
	message += "  -Average hits to stream\n";
	message += "This is CastIts most powerfull feature\n";
	message += "Delete song history will empty the mysql database table of songs\n";
	message += "If you want a to keep all songs then leave this to zero\n";

	MessageBox(message, NULL, MB_ICONINFORMATION);

}

void CConfigurationDialog::OnViewHelpTip3()
{
		short	shBtnColor = 30;
	m_btnHelp3.SetIcon(IDI_HELP);
	m_btnHelp3.SetFlat(TRUE);
//	m_btnHelp3.OffsetColor(CButtonST::BTNST_COLOR_BK_IN, shBtnColor);

			CString message = "";

	message += "Url tip\n";
	message += "URL i.e. http://www.myserver.com/mystatsfile.php\n";
    message += "Note: Parameters containing statistics will be passed to the URL specified\n";
    message += "Example Tips:\n";
	message += " http://mydomain/castit/castit_main.php\n";
	message += " http://mydomain.com/castit/castit_rrd/castit_rrd_update.php\n";
	message += " http://mydomain.com/castit/stats_data.xml\n";


	MessageBox(message, NULL, MB_ICONINFORMATION);

}

void CConfigurationDialog::OnViewHelpTip4()
{
			short	shBtnColor = 30;
	m_btnHelp4.SetIcon(IDI_HELP);
	m_btnHelp4.SetFlat(TRUE);
//	m_btnHelp4.OffsetColor(CButtonST::BTNST_COLOR_BK_IN, shBtnColor);

	CString message = "";

	message += "Interval tips\n";
	message += "Default time is 120 seconds to gather stats on all servers\n";
	message += "you may set the interval time to any length in seconds.\n";


	MessageBox(message, NULL, MB_ICONINFORMATION);

}


void CConfigurationDialog::OnViewHelpTip5()
{
	short	shBtnColor = 30;

	m_btnHelp5.SetIcon(IDI_HELP);
	m_btnHelp5.SetFlat(TRUE);
//	m_btnHelp5.OffsetColor(CButtonST::BTNST_COLOR_BK_IN, shBtnColor);

	CString message = "";

	message += "Ftp & Html Temalpate tips\n";
	message += "You may generate a stats page here and ftp the file\n";
	message += "to your webserver/website.\n";
    message += "Tips:\n";
	message += " you may also use a xml, asp, perl page to\n";
	message += " display stats.\n";

	MessageBox(message, NULL, MB_ICONINFORMATION);


}

