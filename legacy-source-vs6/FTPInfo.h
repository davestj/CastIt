#if !defined(AFX_FTPINFO_H__D10749B5_4053_4D84_AC5B_2743F196576E__INCLUDED_)
#define AFX_FTPINFO_H__D10749B5_4053_4D84_AC5B_2743F196576E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// FTPInfo.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CFTPInfo dialog

class CFTPInfo : public CDialog
{
// Construction
public:
	CFTPInfo(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CFTPInfo)
	enum { IDD = IDD_FTPINFO };
	CString	m_FTPDestination;
	CString	m_FTPHost;
	CString	m_FTPUser;
	CString	m_FTPPassword;
	BOOL	m_FTPTemplateFlag;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFTPInfo)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CFTPInfo)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FTPINFO_H__D10749B5_4053_4D84_AC5B_2743F196576E__INCLUDED_)
