// CastItDlg.cpp : implementation file
//
#include <afxinet.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>



#include "stdafx.h"
#include "CastIt.h"
#include "CastItDlg.h"
#include "ResizableDialog.h"
#include "common.h"
#include "log.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define WM_ICON_NOTIFY  WM_APP+10
#define CASTITVERSION "9.0 build 556"
#define CURRENTTIME "(" __DATE__ "@" __TIME__ ")"

#define IDS_WEBADDR        _T("http://casterclub.com/index.php")

extern CCastItApp theApp;

char    gAppName[] = "CastIt";
char    gConfigFile[] = ".\\Castit.ini";
char    gBackupConfig[]= ".\\Castit.ini.bak";
extern MemoryStruct    Result;
/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CResizableDialog
{
    public:
    CAboutDlg();
    
    // Dialog Data
    //{{AFX_DATA(CAboutDlg)
            enum { IDD = IDD_ABOUTBOX };
            CString    m_Version;
            CHyperLink        m_HomePageLink;
    //}}AFX_DATA
    
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAboutDlg)
            protected:
            virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL
    
    // Implementation
    protected:
    //{{AFX_MSG(CAboutDlg)
            virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CResizableDialog(CAboutDlg::IDD)
{
    //{{AFX_DATA_INIT(CAboutDlg)
            m_Version = _T("");
    //}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAboutDlg)
            DDX_Text(pDX, IDC_VERSION, m_Version);
            DDX_Control(pDX, IDC_HOMEPAGELINK, m_HomePageLink);
    //}}AFX_DATA_MAP
    
    
}

BEGIN_MESSAGE_MAP(CAboutDlg, CResizableDialog)
//{{AFX_MSG_MAP(CAboutDlg)
        // No message handlers
//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BOOL CAboutDlg::OnInitDialog()
{
    CResizableDialog::OnInitDialog();
    
    m_HomePageLink.SetURL(_T("http://mediacast1.com"));
    
    
    
    
    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}
//end about dialog
/////////////////////////////////////////////////////////////////////////////
CCastItDlg    *gDialog;

/////////////////////////////////////////////////////////////////////////////
// CCastItDlg dialog



CCastItDlg::CCastItDlg(CWnd* pParent /*=NULL*/)
: CResizableDialog(CCastItDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CCastItDlg)
            // NOTE: the ClassWizard will add member initialization here
            
    //}}AFX_DATA_INIT
    // Note that LoadIcon does not require a subsequent DestroyIcon in Win32
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
    m_hIcon = AfxGetApp()->LoadIcon(IDI_CASTIT);
    
    m_SysTray.Create(this, WM_ICON_NOTIFY, "CastIt", AfxGetApp()->LoadIcon(MAKEINTRESOURCE(IDI_CASTIT)), IDR_POPUP);
    m_SysTray.RemoveIcon();
    
}

void CCastItDlg::DoDataExchange(CDataExchange* pDX)
{
    CResizableDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CCastItDlg)
            DDX_Control(pDX, IDC_GETSTATS, m_GetStatsCtrl);
            DDX_Control(pDX, IDC_SERVER_LIST, m_ServerList);
            DDX_Control(pDX, IDC_BWIDTH, m_BwidthList);
            DDX_Control(pDX, ID_FILE_EXIT, m_btnClose);
            DDX_Control(pDX, ID_FILE_CONFIGURATION, m_btnConfig);
            DDX_Control(pDX, IDC_ADDSERVER, m_btnAddServer);
            DDX_Control(pDX, IDC_UPDATENOW, m_btnUpdateCastit);
            DDX_Control(pDX, ID_MINIMIZE, m_btnSystray);
            DDX_Control(pDX, ID_CASTITHOME, m_btnCastitHome);
            DDX_Control(pDX, IDC_LOG, m_Log);
            DDX_Control(pDX, IDC_CLEAR_LOG, m_btnLog);
            DDX_Control(pDX, IDC_TAB, m_TabCtrl);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CCastItDlg, CResizableDialog)
//{{AFX_MSG_MAP(CCastItDlg)
        ON_WM_SYSCOMMAND()
        ON_WM_PAINT()
        ON_WM_QUERYDRAGICON()
        ON_WM_CLOSE()
        ON_WM_SIZE()
        ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
        ON_COMMAND(ID_FILE_CONFIGURATION, OnFileConfiguration)
        ON_BN_CLICKED(IDC_GETSTATS, OnGetstats)
        ON_WM_TIMER()
        ON_BN_CLICKED(IDC_UPDATENOW, OnUpdatenow)
        ON_COMMAND(ID_FILE_EXIT, OnFileExit)
        ON_NOTIFY(NM_RCLICK, IDC_SERVER_LIST, OnRclickServerList)
        ON_NOTIFY(NM_DBLCLK, IDC_SERVER_LIST, OnDblclkServerList)
        ON_BN_CLICKED(IDC_ADDSERVER, OnAddserver)
        ON_BN_CLICKED(IDC_CCINI, OnCcini)
        ON_COMMAND(IDC_CCINIBACKUP, OnCciniBackup)
        ON_BN_CLICKED(ID_MINIMIZE, OnMinimize)
        ON_COMMAND(ID_POPUP_RESTORE, OnPopupRestore)
        ON_MESSAGE(WM_ICON_NOTIFY, OnTrayNotification)
        ON_BN_CLICKED(IDC_CLEAR_LOG, OnClearLog)
//}}AFX_MSG_MAP

END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCastItDlg message handlers

LRESULT CCastItDlg::OnTrayNotification(WPARAM wParam, LPARAM lParam)
{
    // Delegate all the work back to the default
    // implementation in CSystemTray.
    return m_SysTray.OnTrayNotification(wParam, lParam);
}
BOOL CCastItDlg::OnInitDialog()
{
    CResizableDialog::OnInitDialog();
    
    
    // Add "About..." menu item to system menu.
    // TODO: Add extra initialization here
    config_read();
    
    ////////////////////////////////////////////////////////////////////////////////
    // STabCtrl initialization
    
    PSTR pszTabItems[] =
    {
        "Server List",
        "Bandwidth",
        "Log Window",
        NULL
    };
    
    TC_ITEM tcItem;
    
    for(INT t = 0;
    pszTabItems[t] != NULL;
    t++)
    {
        tcItem.mask = TCIF_TEXT;
        tcItem.pszText = pszTabItems[t];
        tcItem.cchTextMax = strlen(pszTabItems[t]);
        m_TabCtrl.InsertItem(t,&tcItem);
    }
    
    // attach controls to tabs pages.
    
    m_TabCtrl.AttachControlToTab(&m_ServerList,0);    // attach first static text to first page
    m_TabCtrl.AttachControlToTab(&m_BwidthList,1);    // attach second static text to second page
    m_TabCtrl.AttachControlToTab(&m_Log,2);    // attach second static text to third page
    ///    m_TabCtrl.InsertItem(&m_Log,1);    // attach second static text to second page
    
    // initialize tab to first page.
    m_TabCtrl.SetCurSel(0);
    
    
    //log info to edit window
    str.Format(_T("CASTIT version ",) CASTITVERSION CURRENTTIME " starting up...");
    m_Log.AppendString(str);
    
    
    // Button close
    short    shBtnColor = 30;
    m_btnClose.SetIcon(IDI_EXIT);
    m_btnClose.SetRounded(TRUE,TRUE);
    m_btnClose.SetTooltipText(_T("Exit Castit"));
    m_btnClose.SetColor(CButtonST::BTNST_COLOR_BK_IN, RGB(250,243,221));
    m_btnClose.SetBtnCursor(IDC_HAND);
    
    //Config Button
    m_btnConfig.SetIcon(IDI_CONFIG);
    m_btnConfig.SetRounded(TRUE);
    m_btnConfig.SetTooltipText(_T("Castit Configuration Settings"));
    m_btnConfig.SetColor(CButtonST::BTNST_COLOR_BK_IN, RGB(250,243,221));
    m_btnConfig.SetBtnCursor(IDC_HAND);
    
    //Add server button
    m_btnAddServer.SetIcon(IDI_ADD_SERVER);
    m_btnAddServer.SetRounded(TRUE);
    //    m_btnAddServer.SetMenu(IDR_MAINMENU, m_hWnd);
    m_btnAddServer.SetTooltipText(_T("Add SHOUTcast or Icecast2 Server"));
    m_btnAddServer.SetColor(CButtonST::BTNST_COLOR_BK_IN, RGB(250,243,221));
    m_btnAddServer.SetBtnCursor(IDC_HAND);
    //update castit button
    m_btnUpdateCastit.SetIcon(IDI_UPDATE_CASTIT);
    m_btnUpdateCastit.SetRounded(TRUE);
    m_btnUpdateCastit.SetTooltipText(_T("Update stats for all servers"));
    m_btnUpdateCastit.SetColor(CButtonST::BTNST_COLOR_BK_IN, RGB(250,243,221));
    m_btnUpdateCastit.SetBtnCursor(IDC_HAND);
    //hide to sys tray button
    m_btnSystray.SetIcon(IDI_SYS_TRAY);
    m_btnSystray.SetRounded(TRUE);
    m_btnSystray.SetTooltipText(_T("Minimize Castit to system tray"));
    m_btnSystray.SetColor(CButtonST::BTNST_COLOR_BK_IN, RGB(250,243,221));
    m_btnSystray.SetBtnCursor(IDC_HAND);
    //castit timer
    m_GetStatsCtrl.SetIcon(IDI_TIMER);
    m_GetStatsCtrl.SetRounded(TRUE);
    m_GetStatsCtrl.SetColor(CButtonST::BTNST_COLOR_BK_IN, RGB(250,243,221));
    
    //castit home
    m_btnCastitHome.SetIcon(IDI_HOME);
    m_btnCastitHome.SetURL(IDS_WEBADDR);
    m_btnCastitHome.SetColor(CButtonST::BTNST_COLOR_BK_IN, RGB(250,243,221));
    m_btnCastitHome.SetTooltipText(IDS_WEBADDR);
    m_btnCastitHome.SetRounded(TRUE);
    m_btnCastitHome.SetBtnCursor(IDC_HAND);
    m_btnCastitHome.DrawBorder(TRUE,TRUE);
    //log button
    m_btnLog.SetIcon(IDI_CLEAR_LOG,IDI_EXIT);
    m_btnLog.SetColor(CButtonST::BTNST_COLOR_BK_IN, RGB(250,243,221));
    m_btnLog.SetTooltipText(IDS_WEBADDR);
    m_btnLog.SetRounded(TRUE);
    m_btnLog.DrawBorder(TRUE,TRUE);
    /////////////////////////////////////////////////////////
    //END BUTTONS
    
    
    // IDM_ABOUTBOX must be in the system command range.
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);
    
    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != NULL)
    {
        CString strAboutMenu;
        strAboutMenu.LoadString(IDS_ABOUTBOX);
        if (!strAboutMenu.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
            
        }
    }
    
    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);            // Set big icon
    SetIcon(m_hIcon, TRUE);        // Set small icon
    
    
    //set server list colors
    m_ServerList.SetBkColor(RGB(255,255,255));
    m_ServerList.SetTextColor(RGB(0,0,0));
    m_ServerList.SetTextBkColor(RGB(239,235,222));
    
    //set bandwdith colors
    m_BwidthList.SetBkColor(RGB(255,255,255));
    m_BwidthList.SetTextColor(RGB(0,0,0));
    m_BwidthList.SetTextBkColor(RGB(239,235,222));
    
    int columnNum = 0;
    
    // Create 256 color image lists
    HIMAGELIST hList = ImageList_Create(16,16, ILC_COLOR8 |ILC_MASK , 8, 1);
    m_cImageListNormal.Attach(hList);
    
    HICON hIcon;
    //hIcon = (HICON)LoadImage(theApp, MAKEINTRESOURCE(IDI_SCAST), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    HICON hIcon2;
    //hIcon2 = (HICON)LoadImage(theApp, MAKEINTRESOURCE(IDI_ICECAST2), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    HICON hIcon3;
    
    
    hIcon = theApp.LoadIcon(IDI_SCAST);
    hIcon2 = theApp.LoadIcon(IDI_ICECAST2);
    hIcon3 = theApp.LoadIcon(IDI_STEAMCAST);
    
    
    // Load the large icons
    m_cImageListNormal.Add(hIcon);
    m_cImageListNormal.Add(hIcon2);
    m_cImageListNormal.Add(hIcon3);
    m_ServerList.SetImageList(&m_cImageListNormal, LVSIL_SMALL);
    //    m_ServerList.SetHeaderImageList(IDI_SCAST);
    
    m_BwidthList.SetImageList(&m_cImageListNormal, LVSIL_SMALL);
    //set the list view columns for the servers
    //0
    m_ServerList.InsertColumn(columnNum, _T("Server URL"), LVCFMT_LEFT, m_col1Width);
    columnNum++;
    //1
    m_ServerList.InsertColumn(columnNum, _T("Status Message"), LVCFMT_LEFT, m_col2Width);
    columnNum++;
    //2
    m_ServerList.InsertColumn(columnNum, _T("Server Name"), LVCFMT_LEFT, m_col3Width);
    columnNum++;
    //3
    m_ServerList.InsertColumn(columnNum, _T("Current Song"), LVCFMT_LEFT, m_col4Width);
    columnNum++;
    //4
    m_ServerList.InsertColumn(columnNum, _T("I2: Description"), LVCFMT_LEFT, m_col5Width);
    columnNum++;
    //5
    m_ServerList.InsertColumn(columnNum, _T("Current/Max/Peak Listeners"), LVCFMT_LEFT, m_col6Width);
    columnNum++;
    //6
    m_ServerList.InsertColumn(columnNum, _T("SC: Avg Time"), LVCFMT_LEFT, m_col7Width);
    columnNum++;
    //7
    m_ServerList.InsertColumn(columnNum, _T("SC: Web Hits"), LVCFMT_LEFT, m_col8Width);
    columnNum++;
    //8
    m_ServerList.InsertColumn(columnNum, _T("SC: Stream Hits"), LVCFMT_LEFT, m_col9Width);
    columnNum++;
    //9
    m_ServerList.InsertColumn(columnNum, _T("SC: Bitrate"), LVCFMT_LEFT, m_col10Width);
    columnNum++;
    //10
    m_ServerList.InsertColumn(columnNum, _T("SC: Reported Listeners"), LVCFMT_LEFT, m_col11Width);
    columnNum++;
    //11
    m_ServerList.InsertColumn(columnNum, _T("I2: bitrate/quality/channels/samplerate"), LVCFMT_LEFT, m_col12Width);
    columnNum++;
    //12
    m_ServerList.InsertColumn(columnNum, _T("I2: listeners/peak/connections"), LVCFMT_LEFT, m_col13Width);
    columnNum++;
    //13
    m_ServerList.InsertColumn(columnNum, _T("Stream Type"), LVCFMT_LEFT, m_col14Width);
    columnNum++;
    
    m_ServerList.SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_FLATSB|LVS_EX_TRACKSELECT|LVS_EX_CHECKBOXES);
    m_ServerList.EnableToolTips(TRUE);
    
    
    //set the list view columns for bandwidth
    m_BwidthList.InsertColumn(columnNum, _T("Station"), LVCFMT_LEFT, m_colBW1Width);
    columnNum++;
    m_BwidthList.InsertColumn(columnNum, _T("SHOUTcast Bandwidth"), LVCFMT_LEFT, m_colBW2Width);
    columnNum++;
    m_BwidthList.InsertColumn(columnNum, _T("Icecast2 Bandwidth"), LVCFMT_LEFT, m_colBW3Width);
    columnNum++;
    
    m_BwidthList.SetExtendedStyle(LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_FLATSB|LVS_EX_TRACKSELECT);
    
    
    for (int i=0;i<serverURLs.GetSize();i++) {
        
        if (serverTypes.GetAt(i) == 1) {
            addServerToList(serverURLs.GetAt(i) + "(" + serverMountpoints.GetAt(i) + ")", serverTypes.GetAt(i), serverChecks.GetAt(i));
            //run server checks
            if (serverChecks.GetAt(i) == 0){
                m_ServerList.SetCheck(i,FALSE);
                }else{
                m_ServerList.SetCheck(i,TRUE);
            }
            
            
            }else {
            addServerToList(serverURLs.GetAt(i), serverTypes.GetAt(i), serverChecks.GetAt(i));
            //run server checks
            if (serverChecks.GetAt(i) == 0){
                m_ServerList.SetCheck(i,FALSE);
                }else{
                m_ServerList.SetCheck(i,TRUE);
            }
            
            
            
        }
        
        
    }
    
    
    
    
    
    
    AddAnchor(IDC_SERVER_LIST, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_BWIDTH, TOP_LEFT, BOTTOM_RIGHT);
    AddAnchor(IDC_LOG, TOP_LEFT, BOTTOM_RIGHT);
    EnableSaveRestore(_T("CastIt"));
    
    m_GettingStats = 0;
    
    m_AggregateInfo = 0;
    
    gDialog = this;
    
    //add taskbar notifier
    m_wndTaskbarNotifier1.Create(this);
    m_wndTaskbarNotifier1.SetSkin(IDB_SKIN_MSN);
    m_wndTaskbarNotifier1.SetTextFont("Arial",90,TN_TEXT_NORMAL,TN_TEXT_UNDERLINE);
    m_wndTaskbarNotifier1.SetTextColor(RGB(0,0,0),RGB(0,0,200));
    m_wndTaskbarNotifier1.SetTextRect(CRect(10,40,m_wndTaskbarNotifier1.m_nSkinWidth-10,m_wndTaskbarNotifier1.m_nSkinHeight-25));
    
    
    
    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CCastItDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        char    currentDate[255] = "";
        
        _strdate(currentDate);
        dlgAbout.m_Version = CString("CastIt - Version " CASTITVERSION) + CString(" - ") + CString(currentDate);
        dlgAbout.DoModal();
    }
    else
    {
        CResizableDialog::OnSysCommand(nID, lParam);
    }
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CCastItDlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // device context for painting
        
        SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);
        
        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;
        
        // Draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CResizableDialog::OnPaint();
        
    }
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CCastItDlg::OnQueryDragIcon()
{
    return (HCURSOR) m_hIcon;
}

void CCastItDlg::config_read()
{
    char    buf3[255] = "";
    char    buf2[255] = "";
    char    buf[255] = "";
    CString    tempString;
    
    m_col1Width = GetPrivateProfileInt(gAppName, "col1Width", 125, gConfigFile);
    m_col2Width = GetPrivateProfileInt(gAppName, "col2Width", 150, gConfigFile);
    m_col3Width = GetPrivateProfileInt(gAppName, "col3Width", 275, gConfigFile);
    m_col4Width = GetPrivateProfileInt(gAppName, "col4Width", 100, gConfigFile);
    m_col5Width = GetPrivateProfileInt(gAppName, "col5Width", 25, gConfigFile);
    m_col6Width = GetPrivateProfileInt(gAppName, "col6Width", 50, gConfigFile);
    m_col7Width = GetPrivateProfileInt(gAppName, "col7Width", 75, gConfigFile);
    m_col8Width = GetPrivateProfileInt(gAppName, "col8Width", 75, gConfigFile);
    m_col9Width = GetPrivateProfileInt(gAppName, "col9Width", 75, gConfigFile);
    m_col10Width = GetPrivateProfileInt(gAppName, "col10Width", 75, gConfigFile);
    m_col11Width = GetPrivateProfileInt(gAppName, "col11Width", 75, gConfigFile);
    m_col12Width = GetPrivateProfileInt(gAppName, "col12Width", 75, gConfigFile);
    m_col13Width = GetPrivateProfileInt(gAppName, "col13Width", 75, gConfigFile);
    m_col14Width = GetPrivateProfileInt(gAppName, "col14Width", 75, gConfigFile);
    
    
    m_colBW1Width = GetPrivateProfileInt(gAppName, "colBW1Width", 125, gConfigFile);
    m_colBW2Width = GetPrivateProfileInt(gAppName, "colBW2Width", 125, gConfigFile);
    m_colBW3Width = GetPrivateProfileInt(gAppName, "colBW3Width", 125, gConfigFile);
    
    GetPrivateProfileString(gAppName, "mysqlUsername", "", buf, sizeof(buf), gConfigFile);
    configDialog.m_Username = buf;
    GetPrivateProfileString(gAppName, "mysqlPassword", "", buf, sizeof(buf), gConfigFile);
    configDialog.m_Password = buf;
    GetPrivateProfileString(gAppName, "mysqlDatabase", "", buf, sizeof(buf), gConfigFile);
    configDialog.m_Database = buf;
    GetPrivateProfileString(gAppName, "mysqlHostname", "", buf, sizeof(buf), gConfigFile);
    configDialog.m_Hostname = buf;
    GetPrivateProfileString(gAppName, "mysqlTableprefix", "", buf, sizeof(buf), gConfigFile);
    configDialog.m_TablePrefix = buf;
    configDialog.m_MysqlFlag = GetPrivateProfileInt(gAppName, "mysqlMysqFlag", 1, gConfigFile);
    
    configDialog.m_PollInterval = GetPrivateProfileInt(gAppName, "pollInterval", 120, gConfigFile);
    
    configDialog.m_Aggregate  = GetPrivateProfileInt(gAppName, "Aggregate", 0, gConfigFile);
    configDialog.m_DeleteSongHistoryInterval  = GetPrivateProfileInt(gAppName, "DeleteSongHistoryInterval", 0, gConfigFile);
    
    //iterate through server list and read ini data
    int    serverCount = GetPrivateProfileInt(gAppName, "serverCount", 0, gConfigFile);
    for (int i=0; i<serverCount; i++) {
        sprintf(buf2, "serverURL%d", i);
        GetPrivateProfileString(gAppName, buf2, "", buf, sizeof(buf), gConfigFile);
        tempString = buf;
        serverURLs.Add(tempString);
        
        sprintf(buf2, "serverPassword%d", i);
        GetPrivateProfileString(gAppName, buf2, "", buf, sizeof(buf), gConfigFile);
        tempString = buf;
        serverPasswords.Add(tempString);
        
        sprintf(buf2, "serverType%d", i);
        int serverType = GetPrivateProfileInt(gAppName, buf2, 0, gConfigFile);
        serverTypes.Add(serverType);
        
        sprintf(buf2, "serverMountpoint%d", i);
        GetPrivateProfileString(gAppName, buf2, "", buf, sizeof(buf), gConfigFile);
        tempString = buf;
        serverMountpoints.Add(tempString);
        
        
        
        sprintf(buf3, "serverCheck%d", i);
        int serverCheck = GetPrivateProfileInt(gAppName, buf3, 0, gConfigFile);
        serverChecks.Add(serverCheck);
        
        
    }//end iteration read
    
    GetPrivateProfileString(gAppName, "templateIn", "", buf, sizeof(buf), gConfigFile);
    configDialog.m_TemplateIn = buf;
    GetPrivateProfileString(gAppName, "templateOut", "", buf, sizeof(buf), gConfigFile);
    configDialog.m_TemplateOut = buf;
    configDialog.m_GenerateTemplate = GetPrivateProfileInt(gAppName, "generateTemplate", 0, gConfigFile);
    
    GetPrivateProfileString(gAppName, "callURL", "", buf, sizeof(buf), gConfigFile);
    configDialog.m_URL = buf;
    configDialog.m_CallURL = GetPrivateProfileInt(gAppName, "callURLFlag", 0, gConfigFile);
    
    GetPrivateProfileString(gAppName, "FTPHost", "", buf, sizeof(buf), gConfigFile);
    configDialog.ftpInfoDialog.m_FTPHost = buf;
    GetPrivateProfileString(gAppName, "FTPUser", "", buf, sizeof(buf), gConfigFile);
    configDialog.ftpInfoDialog.m_FTPUser = buf;
    GetPrivateProfileString(gAppName, "FTPPassword", "", buf, sizeof(buf), gConfigFile);
    configDialog.ftpInfoDialog.m_FTPPassword = buf;
    GetPrivateProfileString(gAppName, "FTPDestination", "", buf, sizeof(buf), gConfigFile);
    configDialog.ftpInfoDialog.m_FTPDestination = buf;
    configDialog.ftpInfoDialog.m_FTPTemplateFlag = GetPrivateProfileInt(gAppName, "FTPTemplateFlag", 0, gConfigFile);
    
    //    configDialog.m_UpdateCC = GetPrivateProfileInt(gAppName, "updateCC", 0, gConfigFile);
    //    GetPrivateProfileString(gAppName, "CC_User", "", buf, sizeof(buf), gConfigFile);
    //    configDialog.m_CC_User = buf;
    //    GetPrivateProfileString(gAppName, "CC_Password", "", buf, sizeof(buf), gConfigFile);
    //    configDialog.m_CC_Password = buf;
    
    
}

void CCastItDlg::config_write()
{
    char    buf[25] = "";
    
    m_col1Width = m_ServerList.GetColumnWidth(0);
    m_col2Width = m_ServerList.GetColumnWidth(1);
    m_col3Width = m_ServerList.GetColumnWidth(2);
    m_col4Width = m_ServerList.GetColumnWidth(3);
    m_col5Width = m_ServerList.GetColumnWidth(4);
    m_col6Width = m_ServerList.GetColumnWidth(5);
    m_col7Width = m_ServerList.GetColumnWidth(6);
    m_col8Width = m_ServerList.GetColumnWidth(7);
    m_col9Width = m_ServerList.GetColumnWidth(8);
    m_col10Width = m_ServerList.GetColumnWidth(9);
    m_col11Width = m_ServerList.GetColumnWidth(10);
    m_col12Width = m_ServerList.GetColumnWidth(11);
    m_col13Width = m_ServerList.GetColumnWidth(12);
    m_col14Width = m_ServerList.GetColumnWidth(13);
    
    
    m_colBW1Width = m_BwidthList.GetColumnWidth(0);
    m_colBW2Width = m_BwidthList.GetColumnWidth(1);
    m_colBW3Width = m_BwidthList.GetColumnWidth(2);
    
    sprintf(buf, "%d", m_colBW1Width);
    WritePrivateProfileString(gAppName, "colBW1Width", buf, gConfigFile);
    sprintf(buf, "%d", m_colBW2Width);
    WritePrivateProfileString(gAppName, "colBW2Width", buf, gConfigFile);
    sprintf(buf, "%d", m_colBW3Width);
    WritePrivateProfileString(gAppName, "colBW3Width", buf, gConfigFile);
    
    sprintf(buf, "%d", m_col1Width);
    WritePrivateProfileString(gAppName, "col1Width", buf, gConfigFile);
    sprintf(buf, "%d", m_col2Width);
    WritePrivateProfileString(gAppName, "col2Width", buf, gConfigFile);
    sprintf(buf, "%d", m_col3Width);
    WritePrivateProfileString(gAppName, "col3Width", buf, gConfigFile);
    sprintf(buf, "%d", m_col4Width);
    WritePrivateProfileString(gAppName, "col4Width", buf, gConfigFile);
    sprintf(buf, "%d", m_col5Width);
    WritePrivateProfileString(gAppName, "col5Width", buf, gConfigFile);
    sprintf(buf, "%d", m_col6Width);
    WritePrivateProfileString(gAppName, "col6Width", buf, gConfigFile);
    sprintf(buf, "%d", m_col7Width);
    WritePrivateProfileString(gAppName, "col7Width", buf, gConfigFile);
    sprintf(buf, "%d", m_col8Width);
    WritePrivateProfileString(gAppName, "col8Width", buf, gConfigFile);
    sprintf(buf, "%d", m_col9Width);
    WritePrivateProfileString(gAppName, "col9Width", buf, gConfigFile);
    sprintf(buf, "%d", m_col10Width);
    WritePrivateProfileString(gAppName, "col10Width", buf, gConfigFile);
    sprintf(buf, "%d", m_col11Width);
    WritePrivateProfileString(gAppName, "col11Width", buf, gConfigFile);
    sprintf(buf, "%d", m_col12Width);
    WritePrivateProfileString(gAppName, "col12Width", buf, gConfigFile);
    sprintf(buf, "%d", m_col13Width);
    WritePrivateProfileString(gAppName, "col13Width", buf, gConfigFile);
    sprintf(buf, "%d", m_col14Width);
    WritePrivateProfileString(gAppName, "col14Width", buf, gConfigFile);
    
    
    WritePrivateProfileString(gAppName, "templateIn", configDialog.m_TemplateIn, gConfigFile);
    WritePrivateProfileString(gAppName, "templateOut", configDialog.m_TemplateOut, gConfigFile);
    sprintf(buf, "%d", configDialog.m_GenerateTemplate);
    WritePrivateProfileString(gAppName, "generateTemplate", buf, gConfigFile);
    
    WritePrivateProfileString(gAppName, "callURL", configDialog.m_URL, gConfigFile);
    sprintf(buf, "%d", configDialog.m_CallURL);
    WritePrivateProfileString(gAppName, "callURLFlag", buf, gConfigFile);
    
    WritePrivateProfileString(gAppName, "FTPHost", configDialog.ftpInfoDialog.m_FTPHost, gConfigFile);
    WritePrivateProfileString(gAppName, "FTPUser", configDialog.ftpInfoDialog.m_FTPUser, gConfigFile);
    WritePrivateProfileString(gAppName, "FTPPassword", configDialog.ftpInfoDialog.m_FTPPassword, gConfigFile);
    WritePrivateProfileString(gAppName, "FTPDestination", configDialog.ftpInfoDialog.m_FTPDestination, gConfigFile);
    sprintf(buf, "%d", configDialog.ftpInfoDialog.m_FTPTemplateFlag);
    WritePrivateProfileString(gAppName, "FTPTemplateFlag", buf, gConfigFile);
    
    //    sprintf(buf, "%d", configDialog.m_UpdateCC);
    //    WritePrivateProfileString(gAppName, "updateCC", buf, gConfigFile);
    //    WritePrivateProfileString(gAppName, "CC_User", configDialog.m_CC_User, gConfigFile);
    //    WritePrivateProfileString(gAppName, "CC_Password", configDialog.m_CC_Password, gConfigFile);
    
    
    WritePrivateProfileString(gAppName, "mysqlUsername", configDialog.m_Username, gConfigFile);
    WritePrivateProfileString(gAppName, "mysqlPassword", configDialog.m_Password, gConfigFile);
    WritePrivateProfileString(gAppName, "mysqlDatabase", configDialog.m_Database, gConfigFile);
    WritePrivateProfileString(gAppName, "mysqlHostname", configDialog.m_Hostname, gConfigFile);
    WritePrivateProfileString(gAppName, "mysqlTableprefix", configDialog.m_TablePrefix, gConfigFile);
    sprintf(buf, "%d", configDialog.m_MysqlFlag);
    WritePrivateProfileString(gAppName, "mysqlMysqFlag", buf, gConfigFile);
    sprintf(buf, "%d", configDialog.m_PollInterval);
    WritePrivateProfileString(gAppName, "pollInterval", buf, gConfigFile);
    
    sprintf(buf, "%d", configDialog.m_Aggregate);
    WritePrivateProfileString(gAppName, "Aggregate", buf, gConfigFile);
    sprintf(buf, "%d", configDialog.m_DeleteSongHistoryInterval);
    WritePrivateProfileString(gAppName, "DeleteSongHistoryInterval", buf, gConfigFile);
    
    //iterate through server list and write data
    for (int i=0; i<serverURLs.GetSize();i++) {
        
        sprintf(buf, "serverURL%d", i);
        WritePrivateProfileString(gAppName, buf, serverURLs.GetAt(i), gConfigFile);
        
        sprintf(buf, "serverPassword%d", i);
        WritePrivateProfileString(gAppName, buf, serverPasswords.GetAt(i), gConfigFile);
        
        sprintf(buf, "serverType%d", i);
        char buf2[25] = "";
        sprintf(buf2, "%d", serverTypes.GetAt(i));
        WritePrivateProfileString(gAppName, buf, buf2, gConfigFile);
        
        sprintf(buf, "serverMountpoint%d", i);
        WritePrivateProfileString(gAppName, buf, serverMountpoints.GetAt(i), gConfigFile);
        
        
        //add for list view control of servers checked.
        BOOL IsChecked = m_ServerList.GetCheck(i);
        char buf3[25] = "";
        if (IsChecked) {
            sprintf(buf3, "serverCheck%d", i);
            WritePrivateProfileString(gAppName, buf3, "1", gConfigFile);
            }else{
            sprintf(buf3, "serverCheck%d", i);
            WritePrivateProfileString(gAppName, buf3, "0", gConfigFile);
        }
        
        
    }//end iteration for server list
    
    sprintf(buf, "%d", serverURLs.GetSize());
    WritePrivateProfileString(gAppName, "serverCount", buf, gConfigFile);
    
}

void CCastItDlg::OnClose()
{
    // TODO: Add your message handler code here and/or call default
    config_write();
    if (Result.memory) {
        free(Result.memory);
        Result.memory = NULL;
        Result.size = 0;
    }
    CResizableDialog::OnClose();
    
    
}

void CCastItDlg::OnFileConfiguration()
{
    // TODO: Add your command handler code here
    configDialog.DoModal();
}

void CCastItDlg::EditServerEntry(int i)
{
    addServerDialog.m_ServerURL = serverURLs.GetAt(i);
    addServerDialog.m_ServerPassword = serverPasswords.GetAt(i);
    addServerDialog.m_ServerType = serverTypes.GetAt(i);
    addServerDialog.m_Mountpoint = serverMountpoints.GetAt(i);
    addServerDialog.m_ServerCheck = serverChecks.GetAt(i);
    
    if (addServerDialog.DoModal() == IDOK) {
        serverURLs.SetAt(i, addServerDialog.m_ServerURL);
        serverPasswords.SetAt(i, addServerDialog.m_ServerPassword);
        serverTypes.SetAt(i, addServerDialog.m_ServerType);
        serverMountpoints.SetAt(i, addServerDialog.m_Mountpoint);
        serverChecks.SetAt(i, addServerDialog.m_ServerCheck);
        
        if (addServerDialog.m_ServerType == 1) {
            UpdateServerToList(i, addServerDialog.m_ServerURL + "(" + addServerDialog.m_Mountpoint + ")", addServerDialog.m_ServerType, addServerDialog.m_ServerCheck);
        }
        else {
            UpdateServerToList(i, addServerDialog.m_ServerURL, addServerDialog.m_ServerType, addServerDialog.m_ServerCheck);
        }
    }
}
void CCastItDlg::OnAdd()

{
    addServerDialog.m_ServerType = 0;
    addServerDialog.m_ServerCheck = 0;
    // TODO: Add your control notification handler code here
    if (addServerDialog.DoModal() == IDOK) {
        serverURLs.Add(addServerDialog.m_ServerURL);
        serverPasswords.Add(addServerDialog.m_ServerPassword);
        serverTypes.Add(addServerDialog.m_ServerType);
        serverMountpoints.Add(addServerDialog.m_Mountpoint);
        serverChecks.Add(addServerDialog.m_ServerCheck);
        
        if (addServerDialog.m_ServerType == 2) {
            addServerToList(addServerDialog.m_ServerURL + "(" + addServerDialog.m_Mountpoint + ")", addServerDialog.m_ServerType, addServerDialog.m_ServerCheck);
            //log info to edit window
            str.Format(_T("LOG:",) CURRENTTIME " added steamcast server");
            m_Log.AppendString(str);
        }
        if (addServerDialog.m_ServerType == 1) {
            addServerToList(addServerDialog.m_ServerURL + "(" + addServerDialog.m_Mountpoint + ")", addServerDialog.m_ServerType, addServerDialog.m_ServerCheck);
            //log info to edit window
            str.Format(_T("LOG:",) CURRENTTIME " added icecast2 server");
            m_Log.AppendString(str);
        }
        if (addServerDialog.m_ServerType == 0) {
            addServerToList(addServerDialog.m_ServerURL, addServerDialog.m_ServerType, addServerDialog.m_ServerCheck);
            //log info to edit window
            str.Format(_T("LOG:",) CURRENTTIME " added shoutcast server");
            m_Log.AppendString(str);
        }
        
    }
}

void CCastItDlg::UpdateServerToList(int i, CString serverURL, int serverType, int serverCheck)
{
    LVITEM    lvi;
    
    
    lvi.mask =  LVIF_TEXT | LVIF_IMAGE;
    lvi.iItem = i;
    lvi.iSubItem = 0;
    lvi.pszText = (LPTSTR)(LPCTSTR)(serverURL);
    //m_ServerList.SetCheck(1);
    if (serverType == 0) {
        lvi.iImage = 0;
    }
    if (serverType == 1) {
        lvi.iImage = 1;
    }
    if (serverType == 2) {
        lvi.iImage = 2;
    }
    
    /*
    //set server check
    if (serverCheck == 0){
        m_ServerList.SetCheck(0, FALSE);
    }
    if (serverCheck == 1){
        m_ServerList.SetCheck(0, TRUE);
    }
    
    */
    m_ServerList.SetItem(&lvi);
    
    
}
void CCastItDlg::addServerToList(CString serverURL, int serverType, int serverCheck)
{
    LVITEM    lvi;
    LVITEM    lvi2;
    //
    lvi.mask =  LVIF_TEXT | LVIF_IMAGE;
    lvi.iItem = m_ServerList.GetItemCount();
    lvi.iSubItem = 0;
    lvi.pszText = (LPTSTR)(LPCTSTR)(serverURL);
    if (serverType == 0) {
        lvi.iImage = 0;
    }
    if (serverType == 1) {
        lvi.iImage = 1;
    }
    if (serverType == 2) {
        lvi.iImage = 2;
    }
    
    m_ServerList.InsertItem(&lvi);
    
    
    //
    lvi2.mask =  LVIF_TEXT | LVIF_IMAGE;
    lvi2.iItem = m_BwidthList.GetItemCount();
    lvi2.iSubItem = 0;
    lvi2.pszText = (LPTSTR)(LPCTSTR)(serverURL);
    if (serverType == 0) {
        lvi2.iImage = 0;
    }
    if (serverType == 1) {
        lvi2.iImage = 1;
    }
    if (serverType == 2) {
        lvi2.iImage = 2;
    }
    
    m_BwidthList.InsertItem(&lvi);
    
    
    
    
    
    
}



void CCastItDlg::OnGetstats()
{
    static int nPct = 0;
    // TODO: Add your control notification handler code here
    if (m_GettingStats) {
        KillTimer(1);
        m_GettingStats = 0;
        m_GetStatsCtrl.SetWindowText("Restart");
        for (int i=0;i < serverURLs.GetSize(); i++) {
            //        m_ServerList.DeleteProgress(i, 1);
        }
    }
    else {
        m_TimerCounter = 0;
        m_GettingStats = 1;
        SetTimer(1, 1000, NULL);
        if (configDialog.m_Aggregate) {
            SetTimer(2, 1000*60*60*configDialog.m_Aggregate, NULL);
            
            
        }
    }
}


void CCastItDlg::OnDelete()
{
    // TODO: Add your control notification handler code here
    POSITION pos = m_ServerList.GetFirstSelectedItemPosition();
    if (pos != NULL) {
        int nItem = m_ServerList.GetNextSelectedItem(pos);
        serverURLs.RemoveAt(nItem);
        serverPasswords.RemoveAt(nItem);
        serverTypes.RemoveAt(nItem);
        serverMountpoints.RemoveAt(nItem);
        m_ServerList.DeleteItem(nItem);
    }
    
}

void CCastItDlg::OnTimer(UINT nIDEvent)
{
    // TODO: Add your message handler code here and/or call default
    static int nPct = 0;
    
    if (nIDEvent == 1) {
        
        if (m_TimerCounter >= configDialog.m_PollInterval*1000) {
            m_GetStatsCtrl.SetWindowText("Updating");
            //log info to edit window
            str.Format(_T("LOG:",) CURRENTTIME " updating stats");
            m_Log.AppendString(str);
            //            m_ServerList.SetProgress(0, 1);
            nPct = 0;
            getServerData();
            
        }
        else {
            m_TimerCounter += 1000;
            char buttonText[1024] = "";
            int secsLeft = configDialog.m_PollInterval - (m_TimerCounter/1000);
            memset(buttonText, '\000', sizeof(buttonText));
            sprintf(buttonText, "%d seconds", secsLeft);
            m_GetStatsCtrl.SetWindowText(buttonText);
        }
    }
    if (nIDEvent == 2) {
        m_AggregateInfo = 1;
        nPct += 10;
        //        m_ServerList.UpdateProgress(0, 1, nPct);
        
    }
    CResizableDialog::OnTimer(nIDEvent);
}

void CCastItDlg::UpdateListInfo(int index, serverData *pData)
{
    char    buffer1[255] = "";
    char    buffer2[255] = "";
    char    buffer3[255] = "";
    char    buffer4[255] = "";
    char    buffer5[255] = "";
    char    buffer6[255] = "";
    char    buffer7[255] = "";
    char    buffer8[255] = "";
    char    buffer9[255] = "";
    char    buffer10[255] = "";
    char    buffer11[255] = "";
    char    buffer12[255] = "";
    char    buffer13[255] = "";
    char    bwidthbuffer1[255] = "";
    char    bwidthbuffer2[255] = "";
    char    bwidthbuffer3[255] = "";
    char    bwidthbuffer4[255] = "";
    
    
    
    //setting correct stats info for servers
    CString    serverType = "Unknown";
    
    if (pData->serverType == 0) {
        serverType = "SHOUTcast";
    }
    if (pData->serverType == 1) {
        serverType = "Icecast2";
        
    }
    if (pData->serverType == 2) {
        serverType = "Steamcast";
    }
    
    
    //calculate bandwidth usages
    
    int scbufferbandwidth;
    int icebufferbandwidth;
    int scbufferpeak;
    int icebufferpeak;
    int sc_curmbps;
    int sc_peakmbps;
    int ice_curmbps;
    int ice_peakmbps;
    /*
    int sc_cur_bw_mbps;
    int sc_peak_bw_mbps;
    int icecast2_cur_bw_mbps;
    int icecast2_peak_bw_mbps;
    */
    
    //do shoutcast
    scbufferbandwidth = pData->currentListeners * pData->bitrate;
    pData->scbandwidth = scbufferbandwidth;
    
    scbufferpeak = pData->peakListeners * pData->bitrate;
    pData->sc_peakbw = scbufferpeak;
    
    //do icecast2
    icebufferbandwidth = pData->icecast2_listeners * pData->icecast2_bitrate;
    pData->icecast2_bandwidth = icebufferbandwidth;
    
    icebufferpeak = pData->icecast2_peak_listeners * pData->icecast2_bitrate;
    pData->icecast2_peakbw = icebufferpeak;
    
    //calculate mbps
    //sc
    sc_curmbps = pData->scbandwidth / 1024;
    pData->sc_cur_bw_mbps = sc_curmbps;
    
    sc_peakmbps = pData->sc_peakbw / 1024;
    pData->sc_peak_bw_mbps = sc_peakmbps;
    //ice
    ice_curmbps = pData->icecast2_bandwidth / 1024;
    pData->icecast2_cur_bw_mbps = ice_curmbps;
    
    ice_peakmbps = pData->icecast2_peakbw / 1024;
    pData->icecast2_peak_bw_mbps = ice_peakmbps;
    
    
    
    //set proper channel modes for icecast2
    CString    ice2_channels = "Unknown";
    if (pData->icecast2_channels == 0){
        ice2_channels = "mono";
    }
    if (pData->icecast2_channels == 1){
        ice2_channels = "stereo";
    }
    if (pData->icecast2_channels == 2){
        ice2_channels = "joint stereo";
    }
    
    
    LVITEM    lvi;
    LVITEM lvi2;
    
    lvi.mask =  LVIF_TEXT;
    lvi.iItem = index;
    lvi.iSubItem = 2;
    lvi.pszText = pData->streamName;
    m_ServerList.SetItem(&lvi);
    
    lvi2.mask =  LVIF_TEXT;
    lvi2.iItem = index;
    lvi2.iSubItem = 1;
    //    lvi2.pszText = pData->streamName;
    m_BwidthList.SetItem(&lvi2);
    
    
    if(serverType == "Icecast2"){
        lvi.iSubItem = 4;
        lvi.pszText = pData->icecast2_serverdescription;
        m_ServerList.SetItem(&lvi);
        
    }
    if(serverType == "SHOUTcast"){
        lvi.iSubItem = 4;
        lvi.pszText = "N/A";
        m_ServerList.SetItem(&lvi);
    }
    if(serverType == "Steamcast"){
        lvi.iSubItem = 4;
        lvi.pszText = pData->icecast2_serverdescription;
        m_ServerList.SetItem(&lvi);
    }
    
    memset(buffer1, '\000', sizeof(buffer1));
    memset(buffer2, '\000', sizeof(buffer2));
    memset(buffer3, '\000', sizeof(buffer3));
    memset(buffer4, '\000', sizeof(buffer4));
    memset(buffer5, '\000', sizeof(buffer5));
    memset(buffer6, '\000', sizeof(buffer6));
    memset(buffer7, '\000', sizeof(buffer7));
    memset(buffer8, '\000', sizeof(buffer8));
    memset(buffer9, '\000', sizeof(buffer9));
    memset(buffer10, '\000', sizeof(buffer10));
    memset(buffer11, '\000', sizeof(buffer11));
    memset(buffer12, '\000', sizeof(buffer12));
    memset(buffer13, '\000', sizeof(buffer13));
    memset(bwidthbuffer1, '\000', sizeof(bwidthbuffer1));
    memset(bwidthbuffer2, '\000', sizeof(bwidthbuffer2));
    memset(bwidthbuffer3, '\000', sizeof(bwidthbuffer3));
    memset(bwidthbuffer4, '\000', sizeof(bwidthbuffer4));
    
    if(serverType == "SHOUTcast"){
        sprintf(buffer2, "%d/%d/%d", pData->currentListeners, pData->maxListeners, pData->peakListeners);
        LogMessage(LOG_INFO, buffer2);
    }
    if(serverType == "Steamcast"){
        sprintf(buffer2, "%d/%d/%d", pData->currentListeners, pData->maxListeners, pData->peakListeners);
        LogMessage(LOG_INFO, buffer2);
    }
    if(serverType == "Icecast2"){
        sprintf(buffer2, "%d/NA/%d", pData->icecast2_listeners, pData->icecast2_peak_listeners);
    }
    
    
    if(serverType == "SHOUTcast"){
        sprintf(buffer3, "%d seconds", pData->avgTime);
        LogMessage(LOG_INFO, buffer3);
    }
    if(serverType == "Steamcast"){
        sprintf(buffer3, "%d seconds", pData->avgTime);
        LogMessage(LOG_INFO, buffer3);
    }
    if(serverType == "Icecast2"){
        sprintf(buffer3, "%s", "N/A");
    }
    
    
    if(serverType == "SHOUTcast"){
        sprintf(buffer4, "%d", pData->webHits);
        LogMessage(LOG_INFO, buffer4);
    }
    if(serverType == "Steamcast"){
        sprintf(buffer4, "%s", "N/A");
    }
    if(serverType == "Icecast2"){
        sprintf(buffer4, "%s", "N/A");
    }
    
    
    if(serverType == "SHOUTcast"){
        sprintf(buffer5, "%d", pData->streamHits);
        LogMessage(LOG_INFO, buffer5);
    }
    if(serverType == "Steamcast"){
        sprintf(buffer5, "%d", pData->streamHits);
        LogMessage(LOG_INFO, buffer5);
    }
    if(serverType == "Icecast2"){
        sprintf(buffer5, "%s", "N/A");
    }
    
    
    if(serverType == "SHOUTcast"){
        sprintf(buffer6, "%dkbps", pData->bitrate);
        LogMessage(LOG_INFO, buffer6);
    }
    if(serverType == "Icecast2"){
        sprintf(buffer6, "%s", "N/A");
    }
    if(serverType == "Steamcast"){
        sprintf(buffer6, "%dkbps", pData->bitrate);
        LogMessage(LOG_INFO, buffer6);
    }
    
    if(serverType == "SHOUTcast"){
        sprintf(buffer7, "%d", pData->reportedListeners);
        LogMessage(LOG_INFO, buffer7);
    }
    if(serverType == "Icecast2"){
        sprintf(buffer7, "%s", "N/A");
    }
    if(serverType == "Steamcast"){
        sprintf(buffer7, "%d", pData->reportedListeners);
        LogMessage(LOG_INFO, buffer7);
    }
    
    if(serverType == "Icecast2"){
        sprintf(buffer8, "[%dkbps] - [Quality %d] - [%s] - [%dkhz]", pData->icecast2_bitrate, pData->icecast2_quality, ice2_channels, pData->icecast2_samplerate);
        LogMessage(LOG_INFO, buffer8);
    }
    if(serverType == "SHOUTcast"){
        sprintf(buffer8, "%s", "N/A");
    }
    if(serverType == "Steamcast"){
        sprintf(buffer8, "%s", "N/A");
    }
    
    if(serverType == "Icecast2"){
        sprintf(buffer9, "%d/%d/%d", pData->icecast2_listeners,pData->icecast2_peak_listeners, pData->icecast2_connections);
        LogMessage(LOG_INFO, buffer9);
    }
    if(serverType == "SHOUTcast"){
        sprintf(buffer9, "%s", "N/A");
    }
    if(serverType == "Steamcast"){
        sprintf(buffer9, "%s", "N/A");
    }
    
    
    
    lvi.iSubItem = 3;
    lvi.pszText = pData->songTitle;
    m_ServerList.SetItem(&lvi);
    
    lvi.iSubItem = 5;
    lvi.pszText = buffer2;
    m_ServerList.SetItem(&lvi);
    
    lvi.iSubItem = 6;
    lvi.pszText = buffer3;
    m_ServerList.SetItem(&lvi);
    
    lvi.iSubItem = 7;
    lvi.pszText = buffer4;
    m_ServerList.SetItem(&lvi);
    
    lvi.iSubItem = 8;
    lvi.pszText = buffer5;
    m_ServerList.SetItem(&lvi);
    
    lvi.iSubItem = 9;
    lvi.pszText = buffer6;
    m_ServerList.SetItem(&lvi);
    
    lvi.iSubItem = 10;
    lvi.pszText = buffer7;
    m_ServerList.SetItem(&lvi);
    
    lvi.iSubItem = 11;
    lvi.pszText = buffer8;
    m_ServerList.SetItem(&lvi);
    
    lvi.iSubItem = 12;
    lvi.pszText = buffer9;
    m_ServerList.SetItem(&lvi);
    
    if(serverType == "Icecast2"){
        lvi.iSubItem = 13;
        lvi.pszText = pData->icecast2_stream_type;
        m_ServerList.SetItem(&lvi);
    }
    if(serverType == "SHOUTcast"){
        lvi.iSubItem = 13;
        lvi.pszText = pData->sc_content_type;
        m_ServerList.SetItem(&lvi);
    }
    if(serverType == "Steamcast"){
        lvi.iSubItem = 13;
        lvi.pszText = pData->sc_content_type;
        m_ServerList.SetItem(&lvi);
    }
    
    sprintf(bwidthbuffer1, "Current {%d kbps}-{%d mbps} | Peak {%d kbps}-{%d mbps}", pData->scbandwidth, pData->sc_cur_bw_mbps, pData->sc_peakbw, pData->sc_peak_bw_mbps);
    sprintf(bwidthbuffer2, "Current {%d kbps}-{%d mbps} | Peak {%d kbps}-{%d mbps}", pData->icecast2_bandwidth, pData->icecast2_cur_bw_mbps, pData->icecast2_peakbw, pData->icecast2_peak_bw_mbps);
    
    
    //shoutcast and icecast2 and steamcast bandwidth
    if(serverType == "SHOUTcast"){
        lvi2.iSubItem = 1;
        lvi2.pszText = bwidthbuffer1;
        m_BwidthList.SetItem(&lvi2);
        
        lvi2.iSubItem = 2;
        lvi2.pszText = "N/A";
        m_BwidthList.SetItem(&lvi2);
        
    }
    if(serverType == "Steamcast"){
        lvi2.iSubItem = 1;
        lvi2.pszText = bwidthbuffer1;
        m_BwidthList.SetItem(&lvi2);
        
        lvi2.iSubItem = 2;
        lvi2.pszText = "N/A";
        m_BwidthList.SetItem(&lvi2);
        
    }
    if(serverType == "Icecast2"){
        lvi2.iSubItem = 2;
        lvi2.pszText = bwidthbuffer2;
        m_BwidthList.SetItem(&lvi2);
        
        lvi2.iSubItem = 1;
        lvi2.pszText = "N/A";
        m_BwidthList.SetItem(&lvi2);
    }
    
    
}
void CCastItDlg::UpdateStatusInfo(int index, char *pData)
{
    
    LVITEM    lvi;
    
    lvi.mask =  LVIF_TEXT;
    lvi.iItem = index;
    lvi.iSubItem = 1;
    lvi.pszText = pData;
    m_ServerList.SetItem(&lvi);
    
    
    
}

void CCastItDlg::OnUpdatenow()
{
    // TODO: Add your control notification handler code here
    getServerData();
    
}

void CCastItDlg::getServerData()
{
    
    
    
    CWaitCursor    waitCursor;
    //only part of CXlist control, commented out for now
    //    static int nPct = 0;
    serverData sData;
    char    errorMessage[1024];
    
    if (configDialog.m_GenerateTemplate) {
        ReadTemplateIntoMemory();
    }
    
    
    
    
    
    for (int i=0;i < serverURLs.GetSize(); i++) {
        BOOL bChecked = m_ServerList.GetCheck(i);
        if (bChecked) {
            m_TimerCounter = 0;
            m_ServerList.SetSelectionMark(i);
            m_ServerList.UpdateWindow();
            
            
            //only part of CXlist control, commented out for now
            //        m_ServerList.SetProgress(i, 1);
            //        nPct = 0;
            //        nPct += 100;
            //        m_ServerList.UpdateProgress(i, 1, nPct);
            
            
            UpdateStatusInfo(i, "getting stats");
            m_ServerList.Update(i);
            memset(&sData, '\000', sizeof(sData));
            memset(errorMessage, '\000', sizeof(errorMessage));
            
            
            
            if (!getStats(serverURLs.GetAt(i), serverPasswords.GetAt(i), serverTypes.GetAt(i), serverMountpoints.GetAt(i), &sData, errorMessage)) {
                //log info to edit window
                str.Format(_T(errorMessage));
                m_Log.AppendString(str);
                LogMessage(LOG_ERROR,errorMessage);
                
                //pop up window from systray
                m_wndTaskbarNotifier1.Show(errorMessage,800,3000,500,3);
                UpdateStatusInfo(i, errorMessage);
                m_ServerList.Update(i);
            }
            else {
                str.Format(_T("LOG:",) CURRENTTIME " updated stats successfully");
                m_Log.AppendString(str);
                UpdateStatusInfo(i, "stats retrieved");
                //pop up window from systray
                //MessageBox(errorMessage, NULL, MB_ICONINFORMATION);
                //m_wndTaskbarNotifier1.Show(serverURLs.GetAt(i),800,3000,500,3);
                
                UpdateListInfo(i, &sData);
                if (configDialog.m_CallURL) {
                    callURL(&sData);
                }
                if (configDialog.m_GenerateTemplate) {
                    SubstituteTagsInTemplate(&sData, i);
                }
                if (configDialog.m_MysqlFlag) {
                    configDialog.database.username = configDialog.m_Username;
                    configDialog.database.password = configDialog.m_Password;
                    configDialog.database.hostname = configDialog.m_Hostname;
                    configDialog.database.dbname = configDialog.m_Database;
                    
                    insertIntoMySQL(&(configDialog.database), configDialog.m_TablePrefix, serverURLs.GetAt(i) + serverMountpoints.GetAt(i), &sData);
                    if (m_AggregateInfo) {
                        if (configDialog.m_Aggregate > 0) {
                            aggregateHistory(&(configDialog.database), configDialog.m_TablePrefix, serverURLs.GetAt(i) + serverMountpoints.GetAt(i), &sData, configDialog.m_Aggregate);
                        }
                    }
                    if (configDialog.m_DeleteSongHistoryInterval > 0) {
                        deleteOldHistory(&(configDialog.database), configDialog.m_TablePrefix, serverURLs.GetAt(i) + serverMountpoints.GetAt(i), &sData, configDialog.m_DeleteSongHistoryInterval);
                    }
                }
                /*            if (configDialog.m_UpdateCC) {
                    CString serverType = "";
                    
                    CString    CCURL = "http://localhost/touch.php?ypa=addsrv";
                    //    CCURL += "id=" + URLize(configDialog.m_CC_User);
                    //    CCURL += "&userpass=" + URLize(configDialog.m_CC_Password);
                    //    CCURL += "&cs_login=cs_auth";
                    if (sData.serverType == 0) {
                        serverType = "Shoutcast";
                    }
                    if (sData.serverType == 1) {
                        serverType = "Icecast2";
                    }
                    
                    CCURL += "&st=" + URLize(serverType);
                    CCURL += "&sn=" + URLize(sData.streamName);
                    CCURL += "&cs=" + URLize(sData.songTitle);
                    CCURL += "&sg=" + URLize(sData.serverGenre);
                    CCURL += "&cl=" + URLizeint(sData.currentListeners);
                    CCURL += "&ml=" + URLizeint(sData.maxListeners);
                    CCURL += "&pl=" + URLizeint(sData.peakListeners);
                    CCURL += "&at=" + URLizeint(sData.avgTime);
                    CCURL += "&wh=" + URLizeint(sData.webHits);
                    CCURL += "&sh=" + URLizeint(sData.streamHits);
                    CCURL += "&br=" + URLizeint(sData.bitrate);
                    CCURL += "&su=" + URLize(sData.serverURL);
                    char *buffer = URLCall(CCURL);
                    if (buffer) {
                        LogMessage(LOG_INFO,"yp touch successfull");
                        //    MessageBox("Casterclub Yp Added Me", NULL, MB_ICONINFORMATION);
                        if (!strncmp(buffer, "ERROR", strlen("ERROR"))) {
                            //commented out for nt service
                            //MessageBox("CasterClub Error : " + CString(buffer), NULL, MB_ICONERROR);
                            LogMessage(LOG_ERROR,"ERROR contacting casterclub directory service");
                        }
                    }
                }
            }*/
        }
        if (configDialog.m_GenerateTemplate) {
            WriteTemplateOut();
        }
        if (configDialog.ftpInfoDialog.m_FTPTemplateFlag) {
            FTPFile();
        }
    }
    if (m_AggregateInfo) {
        m_AggregateInfo = 0;
    }
    
}



}//end bChecked
void CCastItDlg::OnFileExit()
{
    // TODO: Add your command handler code here
    SendMessage(WM_CLOSE, 0, 0);
    
}

void CCastItDlg::callURL(serverData *pData)
{
    CString    fullURL;
    CString    serverType = "Unknown";
    CString    thebuffer;
    
    CString parameters;
    
    
    if (configDialog.m_URL.GetLength() > 0) {
        if (pData->serverType == 0) {
            serverType = "Shoutcast";
        }
        if (pData->serverType == 1) {
            serverType = "Icecast2";
        }
        if (pData->serverType == 2) {
            serverType = "Steamcast";
        }
        
        
        parameters = "";
        parameters += "?st=" + URLize(serverType);
        parameters += "&sn=" + URLize(pData->streamName);
        parameters += "&cs=" + URLize(pData->songTitle);
        parameters += "&sg=" + URLize(pData->serverGenre);
        parameters += "&shout_cl=" + URLizeint(pData->currentListeners);
        parameters += "&shout_ml=" + URLizeint(pData->maxListeners);
        parameters += "&shout_pl=" + URLizeint(pData->peakListeners);
        parameters += "&shout_at=" + URLizeint(pData->avgTime);
        parameters += "&shout_wh=" + URLizeint(pData->webHits);
        parameters += "&shout_sh=" + URLizeint(pData->streamHits);
        parameters += "&shout_br=" + URLizeint(pData->bitrate);
        parameters += "&su=" + URLize(pData->serverURL);
        parameters += "&ice2_br=" + URLize(pData->icecast2_bitrate);
        parameters += "&ice2_ql=" + URLize(pData->icecast2_quality);
        parameters += "&ice2_chn=" + URLize(pData->icecast2_channels);
        parameters += "&ice2_sr=" + URLize(pData->icecast2_samplerate);
        parameters += "&ice2_cons=" + URLize(pData->icecast2_connections);
        parameters += "&ice2_cl=" + URLize(pData->icecast2_listeners);
        parameters += "&ice2_strtype=" + URLize(pData->icecast2_stream_type);
        parameters += "&ice2_desc=" + URLize(pData->icecast2_serverdescription);
        fullURL = fullURL + parameters;
        
        char *buffer = URLCall(fullURL);
        if (buffer) {
            if (strlen(buffer) == 0) {
                //commented out for nt service
                //    MessageBox("URL " + configDialog.m_URL + "Error parsing stats page", NULL, MB_ICONERROR);
                LogMessage(LOG_ERROR,"ERROR parsing stats page");
            }
        }
        else {
            //commented out for nt service
            LogMessage(LOG_ERROR, "ERROR calling URL");
            //MessageBox("Error calling URL " + fullURL, NULL, MB_ICONERROR);
        }
    }
}


int CCastItDlg::ReadTemplateIntoMemory()
{
    char    buffer[1024];
    FILE    *filep;
    
    CString tempString = "";
    
    m_TemplateMemory = "";
    
    filep = fopen(configDialog.m_TemplateIn, "r");
    if (!filep) {
        //commented out for nt service
        //    MessageBox("Cannot open file " + configDialog.m_TemplateIn, NULL, MB_ICONERROR);
        LogMessage(LOG_ERROR,"ERROR Cannot open template in file");
        return 0;
    }
    else {
        while (!feof(filep)) {
            memset(buffer, '\000', sizeof(buffer));
            int ret = fread(buffer, sizeof(buffer)-1, 1, filep);
            if (ret >= 0) {
                tempString = buffer;
                
                m_TemplateMemory = m_TemplateMemory + tempString;
            }
        }
        fclose(filep);
    }
    return 1;
}

int CCastItDlg::SubstituteTagsInTemplate(serverData *pData, int nItem)
{
    CString    serverType = "Unknown";
    
    
    char    keyword[255] = "";
    char    buf[255] = "";
    
    // Make it 1-based, not 0-based
    nItem++;
    
    if (pData->serverType == 0) {
        serverType = "Shoutcast";
    }
    if (pData->serverType == 1) {
        serverType = "Icecast2";
    }
    if (pData->serverType == 2) {
        serverType = "Steamcast";
    }
    
    sprintf(keyword, "+SERVER_TYPE_%d+", nItem);
    m_TemplateMemory.Replace(keyword, serverType);
    
    sprintf(keyword, "+SERVER_NAME_%d+", nItem);
    m_TemplateMemory.Replace(keyword, pData->streamName);
    
    sprintf(keyword, "+SERVER_GENRE_%d+", nItem);
    m_TemplateMemory.Replace(keyword, pData->serverGenre);
    
    sprintf(keyword, "+CURRENT_SONG_%d+", nItem);
    m_TemplateMemory.Replace(keyword, pData->songTitle);
    
    sprintf(keyword, "+SHOUTCAST_CURRENT_LISTENERS_%d+", nItem);
    sprintf(buf, "%d", pData->currentListeners);
    m_TemplateMemory.Replace(keyword, buf);
    
    sprintf(keyword, "+SHOUTCAST_MAX_LISTENERS_%d+", nItem);
    sprintf(buf, "%d", pData->maxListeners);
    m_TemplateMemory.Replace(keyword, buf);
    
    sprintf(keyword, "+SHOUTCAST_PEAK_LISTENERS_%d+", nItem);
    sprintf(buf, "%d", pData->peakListeners);
    m_TemplateMemory.Replace(keyword, buf);
    
    sprintf(keyword, "+SHOUTCAST_REPORTED_LISTENERS_%d+", nItem);
    sprintf(buf, "%d", pData->reportedListeners);
    m_TemplateMemory.Replace(keyword, buf);
    
    sprintf(keyword, "+SHOUTCAST_AVG_TIME_%d+", nItem);
    sprintf(buf, "%d", pData->avgTime);
    m_TemplateMemory.Replace(keyword, buf);
    
    sprintf(keyword, "+SHOUTCAST_WEB_HITS_%d+", nItem);
    sprintf(buf, "%d", pData->webHits);
    m_TemplateMemory.Replace(keyword, buf);
    
    sprintf(keyword, "+SHOUTCAST_STREAM_HITS_%d+", nItem);
    sprintf(buf, "%d", pData->streamHits);
    m_TemplateMemory.Replace(keyword, buf);
    
    sprintf(keyword, "+SHOUTCAST_STREAM_BITRATE_%d+", nItem);
    sprintf(buf, "%d", pData->bitrate);
    m_TemplateMemory.Replace(keyword, buf);
    
    
    sprintf(keyword, "+STREAM_URL_%d+", nItem);
    sprintf(buf, pData->serverURL);
    m_TemplateMemory.Replace(keyword, buf);
    
    //ICECAST2 TAGS
    sprintf(keyword, "+ICECAST2_BITRATE_%d+", nItem);
    sprintf(buf, "%d", pData->icecast2_bitrate);
    m_TemplateMemory.Replace(keyword, buf);
    
    sprintf(keyword, "+ICECAST2_QUALITY_%d+", nItem);
    sprintf(buf, "%d", pData->icecast2_quality);
    m_TemplateMemory.Replace(keyword, buf);
    
    sprintf(keyword, "+ICECAST2_CHANNELS_%d+", nItem);
    sprintf(buf, "%d", pData->icecast2_channels);
    m_TemplateMemory.Replace(keyword, buf);
    
    sprintf(keyword, "+ICECAST2_SAMPLERATE_%d+", nItem);
    sprintf(buf, "%d", pData->icecast2_samplerate);
    m_TemplateMemory.Replace(keyword, buf);
    
    sprintf(keyword, "+ICECAST2_CONNECTIONS_%d+", nItem);
    sprintf(buf, "%d", pData->icecast2_connections);
    m_TemplateMemory.Replace(keyword, buf);
    
    sprintf(keyword, "+ICECAST2_LISTENERS_%d+", nItem);
    sprintf(buf, "%d", pData->icecast2_listeners);
    m_TemplateMemory.Replace(keyword, buf);
    
    sprintf(keyword, "+ICECAST2_STREAMTYPE_%d+", nItem);
    sprintf(buf, pData->icecast2_stream_type);
    m_TemplateMemory.Replace(keyword, buf);
    
    sprintf(keyword, "+ICECAST2_SERVER_DESCRIPTIONS_%d+", nItem);
    sprintf(buf, pData->icecast2_serverdescription);
    m_TemplateMemory.Replace(keyword, buf);
    
    return 1;
}

int CCastItDlg::WriteTemplateOut()
{
    
    FILE    *filep;
    
    filep = fopen(configDialog.m_TemplateOut, "w");
    if (!filep) {
        //commented out for nt service
        //    MessageBox("Cannot open file " + configDialog.m_TemplateOut, NULL, MB_ICONERROR);
        LogMessage(LOG_ERROR,"ERROR Cannot open template out file");
    }
    else {
        fwrite((LPCSTR)m_TemplateMemory, m_TemplateMemory.GetLength(), 1, filep);
        fclose(filep);
    }
    return 1;
}

int CCastItDlg::FTPFile()
{
    char    buffer[255] = "";
    struct    _stat    stat_buf;
    CFtpConnection    *ftp;
    int    ret = 0;
    
    CInternetSession* inet = new CInternetSession("Mozilla/4.0 (compatible; CASTIT)",1, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, INTERNET_FLAG_DONT_CACHE);
    
    if (_stat(configDialog.m_TemplateOut, &stat_buf) != 0) {
        sprintf(buffer, "Cannot find FTP Local FILE (%s)", configDialog.m_TemplateOut);
        //commented out for nt service
        //    MessageBox(buffer, "Error", MB_ICONERROR);
        LogMessage(LOG_ERROR, buffer);
        return 0;
    }
    if (configDialog.ftpInfoDialog.m_FTPHost == "") {
        //commented out for nt service
        //    MessageBox("FTP Host is blank!", "Error", MB_ICONERROR);
        LogMessage(LOG_ERROR, "FTP Host is blank!");
        return 0;
    }
    if (configDialog.ftpInfoDialog.m_FTPUser == "") {
        //commented out for nt service
        //    MessageBox("FTP User is blank!", "Error", MB_ICONERROR);
        LogMessage(LOG_ERROR, "FTP User is blank!");
        return 0;
    }
    if (configDialog.ftpInfoDialog.m_FTPPassword == "") {
        //commented out for nt service
        //    MessageBox("FTP Password is blank!", "Error", MB_ICONERROR);
        LogMessage(LOG_ERROR, "FTP Password is blank!");
        return 0;
    }
    if (configDialog.ftpInfoDialog.m_FTPDestination == "") {
        //commented out for nt service
        //    MessageBox("FTP Destination is blank!", "Error", MB_ICONERROR);
        LogMessage(LOG_ERROR, "FTP Destination is blank!");
        return 0;
    }
    
    ftp = inet->GetFtpConnection(configDialog.ftpInfoDialog.m_FTPHost, configDialog.ftpInfoDialog.m_FTPUser, configDialog.ftpInfoDialog.m_FTPPassword, 21, TRUE);
    if (ftp != NULL) {
        ftp->PutFile(configDialog.m_TemplateOut, configDialog.ftpInfoDialog.m_FTPDestination, FTP_TRANSFER_TYPE_ASCII, 1);
        ftp->Close();
        ret = 1;
    }
    else {
        ret = 0;
    }
    
    inet->Close();
    delete inet;
    delete ftp;
    return ret;
}

#include "urlopen.h"

void CCastItDlg::OnAdmin()
{
    // TODO: Add your control notification handler code here
    POSITION pos = m_ServerList.GetFirstSelectedItemPosition();
    CString fullURL;
    if (pos != NULL) {
        int nItem = m_ServerList.GetNextSelectedItem(pos);
        CString serverURL = serverURLs.GetAt(nItem);
        CString serverPassword = serverPasswords.GetAt(nItem);
        
        CString serverMountpoint = serverMountpoints.GetAt(nItem);
        int serverType = serverTypes.GetAt(nItem);
        if (serverType == 2) {
            fullURL = serverURL + "/admin/";
        }
        if (serverType == 1) {
            fullURL = serverURL + "/admin/";
        }
        if (serverType == 0) {
            fullURL = serverURL;
        }
        CURL url;
        url.Open(_T(fullURL));
    }
}
void CCastItDlg::OnPlayStream()
{
    // TODO: Add your control notification handler code here
    POSITION pos = m_ServerList.GetFirstSelectedItemPosition();
    CString fullURL;
    if (pos != NULL) {
        int nItem = m_ServerList.GetNextSelectedItem(pos);
        CString serverURL = serverURLs.GetAt(nItem);
        CString serverPassword = serverPasswords.GetAt(nItem);
        
        CString serverMountpoint = serverMountpoints.GetAt(nItem);
        int serverType = serverTypes.GetAt(nItem);
        if (serverType == 2) {
            fullURL = serverURL + serverMountpoint;
        }
        if (serverType == 1) {
            fullURL = serverURL + serverMountpoint;
        }
        if (serverType == 0) {
            fullURL = serverURL;
        }
        FILE    *filep = NULL;
        filep = fopen("play.m3u", "w");
        if (filep == 0) {
            MessageBox("Unable to create play.m3u", NULL, MB_OK);
        }
        else {
            fprintf(filep, "%s\n", LPCSTR(fullURL));
            fclose(filep);
            fullURL = "file://play.m3u";
            ::ShellExecute(NULL, "open", "play.m3u", NULL, NULL, SW_SHOWNORMAL);
        }
        //            CURL    url;
        //            url.Open(fullURL, true);
    }
}

void CCastItDlg::OnRclickServerList(NMHDR* pNMHDR, LRESULT* pResult)
{
    // TODO: Add your control notification handler code here
    CMenu    menu;
    
    
    CPoint point;
    ::GetCursorPos(&point); //where is the mouse?
    
    DWORD dwSelectionMade;
    menu.LoadMenu(IDR_MENU1);
    CMenu *pmenuPopup = menu.GetSubMenu(0);
    dwSelectionMade = pmenuPopup->TrackPopupMenu( (TPM_LEFTALIGN|TPM_LEFTBUTTON|
    TPM_NONOTIFY|TPM_RETURNCMD),
    point.x, point.y, this
    );
    
    pmenuPopup->DestroyMenu();
    char    msg[255] ="";
    switch (dwSelectionMade) {
        case ID__INSERTNEWSERVERENTRY :
        OnAdd();
        break;
        case ID__DELETE :
        OnDelete();
        break;
        case ID_ADMINPAGE :
        OnAdmin();
        break;
        case ID_PLAYSTREAM :
        OnPlayStream();
        break;
        default :
        break;
    }
    
    *pResult = 0;
}

void CCastItDlg::OnDblclkServerList(NMHDR* pNMHDR, LRESULT* pResult)
{
    // TODO: Add your control notification handler code here
    //for some reason when this is dbl clicked on a non server entry
    //crash boom bang. so we disable it and show a message box
    //with further instruction.
    //MessageBox("Please right click on server entry to edit.", NULL, MB_ICONINFORMATION);
    
    LPNMLISTVIEW lpnmlv = (LPNMLISTVIEW)pNMHDR;
    EditServerEntry(lpnmlv->iItem);
    *pResult = NULL;
    
    
}

void CCastItDlg::OnAddserver()
{
    // TODO: Add your control notification handler code here
    OnAdd();
}



void CCastItDlg::OnCcini()
{
    // TODO: Add your control notification handler code here
    ShellExecute(NULL, "open", ".\\Castit.ini", NULL, NULL, SW_SHOWNORMAL);
}

void CCastItDlg::OnCciniBackup()
{
    
    char    buf[25] = "";
    
    m_col1Width = m_ServerList.GetColumnWidth(0);
    m_col2Width = m_ServerList.GetColumnWidth(1);
    m_col3Width = m_ServerList.GetColumnWidth(2);
    m_col4Width = m_ServerList.GetColumnWidth(3);
    m_col5Width = m_ServerList.GetColumnWidth(4);
    m_col6Width = m_ServerList.GetColumnWidth(5);
    m_col7Width = m_ServerList.GetColumnWidth(6);
    
    
    sprintf(buf, "%d", m_col1Width);
    WritePrivateProfileString(gAppName, "col1Width", buf, gBackupConfig);
    sprintf(buf, "%d", m_col2Width);
    WritePrivateProfileString(gAppName, "col2Width", buf, gBackupConfig);
    sprintf(buf, "%d", m_col3Width);
    WritePrivateProfileString(gAppName, "col3Width", buf, gBackupConfig);
    sprintf(buf, "%d", m_col4Width);
    WritePrivateProfileString(gAppName, "col4Width", buf, gBackupConfig);
    sprintf(buf, "%d", m_col5Width);
    WritePrivateProfileString(gAppName, "col5Width", buf, gBackupConfig);
    sprintf(buf, "%d", m_col6Width);
    WritePrivateProfileString(gAppName, "col6Width", buf, gBackupConfig);
    sprintf(buf, "%d", m_col7Width);
    WritePrivateProfileString(gAppName, "col7Width", buf, gBackupConfig);
    
    
    WritePrivateProfileString(gAppName, "mysqlUsername", configDialog.m_Username, gBackupConfig);
    WritePrivateProfileString(gAppName, "mysqlPassword", configDialog.m_Password, gBackupConfig);
    WritePrivateProfileString(gAppName, "mysqlDatabase", configDialog.m_Database, gBackupConfig);
    WritePrivateProfileString(gAppName, "mysqlHostname", configDialog.m_Hostname, gBackupConfig);
    WritePrivateProfileString(gAppName, "mysqlTableprefix", configDialog.m_TablePrefix, gBackupConfig);
    sprintf(buf, "%d", configDialog.m_MysqlFlag);
    WritePrivateProfileString(gAppName, "mysqlMysqFlag", buf, gBackupConfig);
    sprintf(buf, "%d", configDialog.m_PollInterval);
    WritePrivateProfileString(gAppName, "pollInterval", buf, gBackupConfig);
    
    sprintf(buf, "%d", configDialog.m_Aggregate);
    WritePrivateProfileString(gAppName, "Aggregate", buf, gBackupConfig);
    sprintf(buf, "%d", configDialog.m_DeleteSongHistoryInterval);
    WritePrivateProfileString(gAppName, "DeleteSongHistoryInterval", buf, gBackupConfig);
    
    for (int i=0; i<serverURLs.GetSize();i++) {
        sprintf(buf, "serverURL%d", i);
        WritePrivateProfileString(gAppName, buf, serverURLs.GetAt(i), gBackupConfig);
        sprintf(buf, "serverPassword%d", i);
        WritePrivateProfileString(gAppName, buf, serverPasswords.GetAt(i), gBackupConfig);
        sprintf(buf, "serverType%d", i);
        char buf2[25] = "";
        sprintf(buf2, "%d", serverTypes.GetAt(i));
        WritePrivateProfileString(gAppName, buf, buf2, gBackupConfig);
        sprintf(buf, "serverMountpoint%d", i);
        WritePrivateProfileString(gAppName, buf, serverMountpoints.GetAt(i), gBackupConfig);
    }
    sprintf(buf, "%d", serverURLs.GetSize());
    WritePrivateProfileString(gAppName, "serverCount", buf, gBackupConfig);
    
    WritePrivateProfileString(gAppName, "templateIn", configDialog.m_TemplateIn, gBackupConfig);
    WritePrivateProfileString(gAppName, "templateOut", configDialog.m_TemplateOut, gBackupConfig);
    sprintf(buf, "%d", configDialog.m_GenerateTemplate);
    WritePrivateProfileString(gAppName, "generateTemplate", buf, gBackupConfig);
    
    WritePrivateProfileString(gAppName, "callURL", configDialog.m_URL, gBackupConfig);
    sprintf(buf, "%d", configDialog.m_CallURL);
    WritePrivateProfileString(gAppName, "callURLFlag", buf, gBackupConfig);
    
    WritePrivateProfileString(gAppName, "FTPHost", configDialog.ftpInfoDialog.m_FTPHost, gBackupConfig);
    WritePrivateProfileString(gAppName, "FTPUser", configDialog.ftpInfoDialog.m_FTPUser, gBackupConfig);
    WritePrivateProfileString(gAppName, "FTPPassword", configDialog.ftpInfoDialog.m_FTPPassword, gBackupConfig);
    WritePrivateProfileString(gAppName, "FTPDestination", configDialog.ftpInfoDialog.m_FTPDestination, gBackupConfig);
    sprintf(buf, "%d", configDialog.ftpInfoDialog.m_FTPTemplateFlag);
    WritePrivateProfileString(gAppName, "FTPTemplateFlag", buf, gBackupConfig);
    
    //    sprintf(buf, "%d", configDialog.m_UpdateCC);
    //    WritePrivateProfileString(gAppName, "updateCC", buf, gBackupConfig);
    //    WritePrivateProfileString(gAppName, "CC_User", configDialog.m_CC_User, gBackupConfig);
    //    WritePrivateProfileString(gAppName, "CC_Password", configDialog.m_CC_Password, gBackupConfig);
    MessageBox("File saved as\n Castit.ini.bak", NULL, MB_ICONINFORMATION);
}

void CCastItDlg::OnMinimize()
{
    // TODO: Add your control notification handler code here
    
    m_SysTray.MinimiseToTray(this);
    m_SysTray.AddIcon();
    
    
}

void CCastItDlg::OnPopupRestore()
{
    // TODO: Add your command handler code here
    m_SysTray.MaximiseFromTray(this);
    m_SysTray.RemoveIcon();
    
    
}

void CCastItDlg::OnClearLog()
{
    m_Log.SetWindowText(_T(""));
    //CResizableDialog::OnClearLog();
    
}

// App command to run the dialog
void CCastItDlg::OnAppAbout()
{
    CAboutDlg aboutDlg;
    aboutDlg.DoModal();
}
