// CastIt.cpp : Defines the class behaviors for the application.
//

#include "StdAfx.h"
#include "CastIt.h"
#include "DB.h"
#include "CastItDlg.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCastItApp

BEGIN_MESSAGE_MAP(CCastItApp, CWinApp)
	//{{AFX_MSG_MAP(CCastItApp)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	ON_COMMAND(ID_HELP, CWinApp::OnHelpIndex)
	END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCastItApp construction

CCastItApp::CCastItApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CCastItApp object

CCastItApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CCastItApp initialization

BOOL CCastItApp::InitInstance()
{
	if (!AfxSocketInit())
	{
		AfxMessageBox(IDP_SOCKETS_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	//SetDialogBkColor(RGB(250,243,221),RGB(0,0,0)); 



	CCastItDlg dlg;
	m_pMainWnd = &dlg;


	int nResponse = dlg.DoModal();
	if (nResponse == IDOK)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with OK
	}
	else if (nResponse == IDCANCEL)
	{
		// TODO: Place code here to handle when the dialog is
		//  dismissed with Cancel
	}


	// Since the dialog has been closed, return FALSE so that we exit the
	//  application, rather than start the application's message pump.
	return FALSE;
}
