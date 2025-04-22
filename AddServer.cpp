// AddServer.cpp : implementation file
//

#include "stdafx.h"
#include "CastIt.h"
#include "AddServer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAddServer dialog


CAddServer::CAddServer(CWnd* pParent /*=NULL*/)
	: CDialog(CAddServer::IDD, pParent)
{
	//{{AFX_DATA_INIT(CAddServer)
	m_ServerCheck = -1;
	m_ServerURL = _T("");
	m_ServerPassword = _T("");
	m_Mountpoint = _T("");
	m_ServerType = -1;
	//}}AFX_DATA_INIT
}


void CAddServer::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAddServer)
	DDX_Text(pDX, IDC_SERVERURL, m_ServerURL);
	DDX_Text(pDX, IDC_SERVERPASSWORD, m_ServerPassword);
	DDX_Text(pDX, IDC_MOUNTPOINT, m_Mountpoint);
	DDX_Radio(pDX, IDC_SERVERTYPE, m_ServerType);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CAddServer, CDialog)
	//{{AFX_MSG_MAP(CAddServer)
		// NOTE: the ClassWizard will add message map macros here
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CAddServer message handlers
