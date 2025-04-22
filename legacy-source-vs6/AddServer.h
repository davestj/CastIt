#if !defined(AFX_ADDSERVER_H__3EEC25FA_149F_4D62_84C3_9455A405CAEA__INCLUDED_)
#define AFX_ADDSERVER_H__3EEC25FA_149F_4D62_84C3_9455A405CAEA__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// AddServer.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAddServer dialog

class CAddServer : public CDialog
{
// Construction
public:
	CAddServer(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CAddServer)
	enum { IDD = IDD_ADDSERVER };
	int		m_ServerCheck;
	CString	m_ServerURL;
	CString	m_ServerPassword;
	CString	m_Mountpoint;
	int		m_ServerType;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAddServer)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CAddServer)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ADDSERVER_H__3EEC25FA_149F_4D62_84C3_9455A405CAEA__INCLUDED_)
