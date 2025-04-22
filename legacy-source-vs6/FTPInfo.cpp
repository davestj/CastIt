// FTPInfo.cpp : implementation file
//

#include "stdafx.h"
#include "CastIt.h"
#include "FTPInfo.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CFTPInfo dialog


CFTPInfo::CFTPInfo(CWnd* pParent /*=NULL*/)
	: CDialog(CFTPInfo::IDD, pParent)
{
	//{{AFX_DATA_INIT(CFTPInfo)
	m_FTPDestination = _T("");
	m_FTPHost = _T("");
	m_FTPUser = _T("");
	m_FTPPassword = _T("");
	m_FTPTemplateFlag = FALSE;
	//}}AFX_DATA_INIT
}


void CFTPInfo::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CFTPInfo)
	DDX_Text(pDX, IDC_FTPDESTINATION, m_FTPDestination);
	DDX_Text(pDX, IDC_FTPHOST, m_FTPHost);
	DDX_Text(pDX, IDC_FTPUSER, m_FTPUser);
	DDX_Text(pDX, IDC_FTPPASSWORD, m_FTPPassword);
	DDX_Check(pDX, IDC_FTPTEMPLATE, m_FTPTemplateFlag);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CFTPInfo, CDialog)
	//{{AFX_MSG_MAP(CFTPInfo)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFTPInfo message handlers
