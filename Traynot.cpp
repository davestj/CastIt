#include "stdafx.h"
#include "TrayNot.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTrayNot

CTrayNot::CTrayNot ( CWnd* pWnd, UINT uCallbackMessage,
							LPCTSTR szTip, HICON* pList )
{
	// this is only for Windows 95 (or higher)
	m_bEnabled = ( GetVersion() & 0xff ) >= 4 ;
	if (!m_bEnabled)
		return ;

	// load up the NOTIFYICONDATA structure
	m_tnd.cbSize = sizeof(NOTIFYICONDATA) ;
	m_tnd.hWnd = pWnd->GetSafeHwnd() ;
	m_tnd.uID = 0 ;
	m_tnd.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP ;	
	m_tnd.uCallbackMessage = uCallbackMessage;  
	strcpy ( m_tnd.szTip, "Tray Icon Sample - By Ashley Rowe");
	//or you could use:
	//strcpy ( m_tnd.szTip, AfxGetApp()->m_pszAppName);
	//this will display the app name instead of the string you specify 

 
 
	// save the pointer to the icon list and set the initial
	// default icon.
	m_pIconList = pList ;
	m_tnd.hIcon = m_pIconList[0] ;
	Shell_NotifyIcon (NIM_ADD,&m_tnd);
 


}

CTrayNot::~CTrayNot()
{
	if (m_bEnabled)
		Shell_NotifyIcon (NIM_DELETE, &m_tnd);
}

void CTrayNot::SetState(int id)
{
	if (!m_bEnabled)
		return;
	m_tnd.hIcon = m_pIconList[id];
	//Ive found in windows XP that this command makes the icon not visable in the system tray....we dont want that now
	//do we?
//	Shell_NotifyIcon(NIM_MODIFY, &m_tnd);
}