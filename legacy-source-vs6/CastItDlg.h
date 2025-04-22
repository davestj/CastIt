// CastItDlg.h : header file
//

#if !defined(AFX_CASTITDLG_H__97BC97AD_9A27_4A08_B12E_4935EFDCEFDA__INCLUDED_)
#define AFX_CASTITDLG_H__97BC97AD_9A27_4A08_B12E_4935EFDCEFDA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#include <afxtempl.h>

typedef CArray <CString, CString &> StringArray;
typedef CArray <int, int &> IntArray;

#include "common.h"
#include "ResizableDialog.h"
#include "ConfigurationDialog.h"	// Added by ClassView
#include "DB.h"	// Added by ClassView
#include "AddServer.h"	// Added by ClassView
#include "SystemTray.h"
#include "HyperLink.h"
#include "BtnST.h"
#include "WinXPButtonST.h"
#include "BCMenu.h"
#include "TaskbarNotifier.h"
#include "HistoryEdit.h"
#include "STabCtrl.h"


/////////////////////////////////////////////////////////////////////////////
// CCastItDlg dialog

class CCastItDlg : public CResizableDialog
{
// Construction
public:
	int m_AggregateInfo;
	int FTPFile();
	int WriteTemplateOut();
	int SubstituteTagsInTemplate(serverData *pData, int nItem);
	CString m_TemplateMemory;
	void EditServerEntry(int i);
	int ReadTemplateIntoMemory();
	void callURL(serverData *pData);
	void getServerData();
	void UpdateListInfo(int index, serverData *pData);
	void UpdateStatusInfo(int index, char *pData);
	int m_GettingStats;
	long m_TimerCounter;
	void UpdateServerToList(int i,CString serverURL, int serverType, int serverCheck);
	void addServerToList(CString serverURL, int serverType, int serverCheck);
	void OnAdmin();
	void OnPlayStream();
	CAddServer addServerDialog;
	DB database;
	CConfigurationDialog configDialog;
	void config_write();
	int m_col1Width;
	int m_col2Width;
	int m_col3Width;
	int m_col4Width;
	int m_col5Width;
	int m_col6Width;
	int m_col7Width;
	int m_col8Width;
	int m_col9Width;
	int m_col10Width;
	int m_col11Width;
	int m_col12Width;
	int m_col13Width;
	int m_col14Width;
	int m_colBW1Width;
	int m_colBW2Width;
	int m_colBW3Width;
	int	m_ServerCheck;
	StringArray		serverURLs;
	StringArray		serverPasswords;
	StringArray		serverMountpoints;
	IntArray		serverTypes;
	IntArray		serverChecks;
	CSystemTray m_SysTray;
	CImageList	m_cImageListNormal;
	CString str;


	void config_read();
	
	CCastItDlg(CWnd* pParent = NULL);	// standard constructor
	LRESULT OnTrayNotification(WPARAM wParam, LPARAM lParam);

// Dialog Data
	//{{AFX_DATA(CCastItDlg)
	enum { IDD = IDD_CASTIT_DIALOG };
	//CButton	m_GetStatsCtrl;
	CWinXPButtonST m_GetStatsCtrl;
	CListCtrl	m_ServerList;
	CListCtrl	m_BwidthList;
	CHistoryEdit	m_Log;
	CSTabCtrl	m_TabCtrl;
//	CTabCtrl	m_TabCtrl;
//	CMyHyperLink	m_Static1;
	//}}AFX_DATA



	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCastItDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	void CLinkCtrl();
	HICON m_hIcon;
	CEdit m_wndEdit;
	CTaskbarNotifier m_wndTaskbarNotifier2;
	CTaskbarNotifier m_wndTaskbarNotifier1;
	// Generated message map functions
	//{{AFX_MSG(CCastItDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnClose();
	afx_msg void OnAppAbout();
	afx_msg void OnFileConfiguration();
	afx_msg void OnAdd();
	afx_msg void OnGetstats();
	afx_msg void OnDelete();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnUpdatenow();
	afx_msg void OnFileExit();
	afx_msg void OnRclickServerList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDblclkServerList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnAddserver();
	afx_msg void OnCcini();
	afx_msg void OnCciniBackup();
	afx_msg void OnMinimize();
	afx_msg void OnPopupRestore();
	afx_msg void OnClearLog();
	//}}AFX_MSG


private:

	
	CWinXPButtonST	m_btnClose;
	CWinXPButtonST	m_btnConfig;
	CWinXPButtonST	m_btnAddServer;
	//CButtonST m_btnAddServer;
	CWinXPButtonST	m_btnUpdateCastit;
	CWinXPButtonST	m_btnLog;
	CWinXPButtonST	m_btnSystray;
	CWinXPButtonST	m_btnCastitHome;


	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CASTITDLG_H__97BC97AD_9A27_4A08_B12E_4935EFDCEFDA__INCLUDED_)
