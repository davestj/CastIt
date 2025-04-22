#if !defined(AFX_CONFIGURATIONDIALOG_H__C185B148_D53B_4FDB_AD4E_35A53E912F7C__INCLUDED_)
#define AFX_CONFIGURATIONDIALOG_H__C185B148_D53B_4FDB_AD4E_35A53E912F7C__INCLUDED_

#include "DB.h"
#include "FTPInfo.h"	// Added by ClassView
#include "BtnST.h"
#include "BCMenu.h"
#include "WinXPButtonST.h"

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ConfigurationDialog.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CConfigurationDialog dialog

class CConfigurationDialog : public CDialog
{
// Construction
public:
	void OnViewHelpTip6();
	void OnViewHelpTip5();
	void OnViewHelpTip4();
	void OnViewHelpTip3();
	void OnViewHelpTip2();
	void OnViewHelpTip1();

	CFTPInfo ftpInfoDialog;
	CConfigurationDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CConfigurationDialog)
	enum { IDD = IDD_CONFIG_DIALOG };
	CEdit	m_DeleteSongCtl;
	CEdit	m_AggregateCtl;
	CButton	m_CreatCtl;
	CButton	m_VerifyCtl;
	CEdit	m_UsernameCtl;
	CEdit	m_TablePrefixCtl;
	CEdit	m_PasswordCtl;
	CEdit	m_HostnameCtl;
	CEdit	m_DatabaseCtl;
	CString	m_Database;
	CString	m_Hostname;
	CString	m_Password;
	CString	m_TablePrefix;
	CString	m_Username;
	BOOL	m_MysqlFlag;
	int		m_Aggregate;
	int		m_DeleteSongHistoryInterval;
	int		m_PollInterval;
	CString	m_URL;
	BOOL	m_CallURL;
	CString	m_TemplateIn;
	CString	m_TemplateOut;
	BOOL	m_GenerateTemplate;
//	CString	m_CC_User;
//	CString	m_CC_Password;
//	BOOL	m_UpdateCC;
	//}}AFX_DATA

	DB	database;
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConfigurationDialog)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:


	// Generated message map functions
	//{{AFX_MSG(CConfigurationDialog)
	afx_msg void OnMysqlFlag();
	afx_msg void OnVerifyConnection();
	afx_msg void OnCreateTables();
	afx_msg void OnClose();
	virtual BOOL OnInitDialog();
	afx_msg void OnBrowse1();
	afx_msg void OnBrowse2();
	afx_msg void OnViewcgi();
	afx_msg void OnFtpinfo();
//	afx_msg void OnCcCheck();
	//}}AFX_MSG

private:

	CButtonST		m_btnHelp1;
	CButtonST		m_btnHelp2;
	CButtonST		m_btnHelp3;
	CButtonST		m_btnHelp4;
	CButtonST		m_btnHelp5;


	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONFIGURATIONDIALOG_H__C185B148_D53B_4FDB_AD4E_35A53E912F7C__INCLUDED_)
