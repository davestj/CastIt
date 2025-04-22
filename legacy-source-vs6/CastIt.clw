; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=CAddServer
LastTemplate=CDialog
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "CastIt.h"

ClassCount=6
Class1=CCastItApp
Class2=CCastItDlg
Class3=CAboutDlg

ResourceCount=10
Resource1=IDR_POPUP
Resource2=IDD_FTPINFO
Resource3=IDR_MAINMENU
Resource4=IDD_ADDSERVER
Class4=CConfigurationDialog
Resource5=IDR_MAINFRAME
Class5=CAddServer
Resource6=IDD_ABOUTBOX
Class6=CFTPInfo
Resource7=IDR_TOOLBAR
Resource8=IDR_MENU1
Resource9=IDD_CONFIG_DIALOG
Resource10=IDD_CASTIT_DIALOG

[CLS:CCastItApp]
Type=0
HeaderFile=CastIt.h
ImplementationFile=CastIt.cpp
Filter=N

[CLS:CCastItDlg]
Type=0
HeaderFile=CastItDlg.h
ImplementationFile=CastItDlg.cpp
Filter=D
BaseClass=CResizableDialog
VirtualFilter=dWC
LastObject=CCastItDlg

[CLS:CAboutDlg]
Type=0
HeaderFile=CastItDlg.h
ImplementationFile=CastItDlg.cpp
Filter=D
LastObject=IDOK
BaseClass=CResizableDialog
VirtualFilter=dWC

[DLG:IDD_ABOUTBOX]
Type=1
Class=CAboutDlg
ControlCount=7
Control1=IDC_STATIC,static,1342177283
Control2=IDC_HOMEPAGELINK,static,1342308352
Control3=IDOK,button,1342406656
Control4=IDC_VERSION,static,1342308352
Control5=IDC_STATIC,static,1342308352
Control6=IDC_STATIC,static,1342308352
Control7=IDC_STATIC,static,1342308352

[DLG:IDD_CASTIT_DIALOG]
Type=1
Class=CCastItDlg
ControlCount=12
Control1=IDC_SERVER_LIST,SysListView32,1350631429
Control2=ID_FILE_CONFIGURATION,button,1342246656
Control3=IDC_ADDSERVER,button,1342246656
Control4=IDC_UPDATENOW,button,1342246656
Control5=ID_MINIMIZE,button,1342246656
Control6=ID_FILE_EXIT,button,1342246656
Control7=IDC_GETSTATS,button,1342246656
Control8=ID_CASTITHOME,button,1342246656
Control9=IDC_LOG,edit,1352730820
Control10=IDC_TAB,SysTabControl32,1342177536
Control11=IDC_CLEAR_LOG,button,1342181120
Control12=IDC_BWIDTH,SysListView32,1350631429

[DLG:IDD_CONFIG_DIALOG]
Type=1
Class=CConfigurationDialog
ControlCount=43
Control1=IDC_POLLINTERVAL,edit,1350631552
Control2=IDC_MYSQL_FLAG,button,1342242819
Control3=IDC_MY_HOSTNAME,edit,1350631552
Control4=IDC_MY_USERNAME,edit,1350631552
Control5=IDC_MY_PASSWORD,edit,1350631552
Control6=IDC_MY_DATABASE,edit,1350631552
Control7=IDC_MY_TABLEPREFIX,edit,1350631552
Control8=IDC_AGGREGATE,edit,1350631552
Control9=IDC_DELETE_SONGHISTORY_INTERVAL,edit,1350631552
Control10=IDC_VERIFY_CONNECTION,button,1342275584
Control11=IDC_CREATE_TABLES,button,1342275584
Control12=IDC_GENERATETEMPLATE,button,1342242819
Control13=IDC_TEMPLATEIN,edit,1350631552
Control14=IDC_BROWSE1,button,1342275584
Control15=IDC_TEMPLATEOUT,edit,1350631552
Control16=IDC_BROWSE2,button,1342275584
Control17=IDC_FTPINFO,button,1342275584
Control18=IDC_CALLURL,button,1342242819
Control19=IDC_URL,edit,1350631552
Control20=IDC_VIEWCGI,button,1342275840
Control21=IDOK,button,1342275585
Control22=IDCANCEL,button,1342275584
Control23=IDC_STATIC,static,1342308352
Control24=IDC_STATIC,static,1342308352
Control25=IDC_STATIC,static,1342308352
Control26=IDC_STATIC,static,1342308352
Control27=IDC_STATIC,static,1342308352
Control28=IDC_STATIC,static,1342308352
Control29=IDC_STATIC,static,1342308352
Control30=IDC_STATIC,static,1342308352
Control31=IDC_STATIC,static,1342308352
Control32=IDC_STATIC,static,1342308352
Control33=IDC_STATIC,button,1342177287
Control34=IDC_STATIC,button,1342177287
Control35=IDC_STATIC,static,1342308352
Control36=IDC_STATIC,static,1342308352
Control37=IDC_STATIC,button,1342177287
Control38=IDC_STATIC,button,1342177287
Control39=IDC_VIEWHELP1,button,1342243072
Control40=IDC_VIEWHELP2,button,1342243072
Control41=IDC_VIEWHELP3,button,1342243072
Control42=IDC_VIEWHELP4,button,1342243072
Control43=IDC_VIEWHELP5,button,1342243136

[CLS:CConfigurationDialog]
Type=0
HeaderFile=ConfigurationDialog.h
ImplementationFile=ConfigurationDialog.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=CConfigurationDialog

[DLG:IDD_ADDSERVER]
Type=1
Class=CAddServer
ControlCount=18
Control1=IDC_SERVERURL,edit,1350631552
Control2=IDC_SERVERPASSWORD,edit,1350631552
Control3=IDC_MOUNTPOINT,edit,1350631552
Control4=IDOK,button,1342275584
Control5=IDCANCEL,button,1342275584
Control6=IDC_STATIC,static,1342179331
Control7=IDC_STATIC,static,1342179331
Control8=IDC_STATIC,static,1342308352
Control9=IDC_STATIC,static,1342308352
Control10=IDC_STATIC,static,1342308352
Control11=IDC_STATIC,button,1342177287
Control12=IDC_STATIC,button,1342177287
Control13=IDC_STATIC,button,1342177287
Control14=IDC_STATIC,button,1342177287
Control15=IDC_SERVERTYPE,button,1342308361
Control16=IDC_SERVERTYPE2,button,1342177289
Control17=IDC_SERVERTYPE3,button,1342177289
Control18=IDC_STATIC,static,1342179331

[CLS:CAddServer]
Type=0
HeaderFile=AddServer.h
ImplementationFile=AddServer.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=IDC_SERVERTYPE

[DLG:IDD_FTPINFO]
Type=1
Class=CFTPInfo
ControlCount=12
Control1=IDC_FTPTEMPLATE,button,1342242819
Control2=IDC_FTPHOST,edit,1350631552
Control3=IDC_FTPUSER,edit,1350631552
Control4=IDC_FTPPASSWORD,edit,1350631552
Control5=IDC_FTPDESTINATION,edit,1350631552
Control6=IDOK,button,1342242817
Control7=IDCANCEL,button,1342242816
Control8=IDC_STATIC,static,1342308352
Control9=IDC_STATIC,static,1342308352
Control10=IDC_STATIC,static,1342308352
Control11=IDC_STATIC,static,1342308352
Control12=IDC_STATIC,static,1342308352

[CLS:CFTPInfo]
Type=0
HeaderFile=FTPInfo.h
ImplementationFile=FTPInfo.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=CFTPInfo

[MNU:IDR_POPUP]
Type=1
Class=CCastItDlg
Command1=ID_POPUP_RESTORE
Command2=IDC_UPDATENOW
Command3=ID_FILE_EXIT
CommandCount=3

[TB:IDR_TOOLBAR]
Type=1
Class=?
Command1=ID_BUTTON32780
Command2=ID_BUTTON32781
Command3=ID_BUTTON32782
CommandCount=3

[MNU:IDR_MAINMENU]
Type=1
Class=?
Command1=ID_MINIMIZE
Command2=ID_FILE_EXIT
Command3=IDC_CCINI
Command4=ID_FILE_CONFIGURATION
Command5=IDC_CCINIBACKUP
Command6=ID_HELP
Command7=ID_APP_ABOUT
CommandCount=7

[MNU:IDR_MENU1]
Type=1
Class=?
CommandCount=0

[ACL:IDR_MAINFRAME]
Type=1
Class=?
Command1=ID_FILE_CONFIGURATION
CommandCount=1

