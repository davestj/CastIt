// STabCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "STabCtrl.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CSTabCtrl::CSTabPage::CSTabPage()
{

}

CSTabCtrl::CSTabPage::~CSTabPage()
{
	m_ControlList.RemoveAll();
}

BOOL CSTabCtrl::CSTabPage::AttachControl (CWnd * _pControl)
{
	return (BOOL) m_ControlList.AddTail (_pControl);
}

BOOL CSTabCtrl::CSTabPage::ShowWindows ( INT _nCmdShow )
{
	POSITION _rPos = m_ControlList.GetHeadPosition();
	CWnd * _pCtrl = NULL;

	while(_rPos)
	{
		_pCtrl = m_ControlList.GetNext(_rPos);

		::ShowWindow(_pCtrl -> GetSafeHwnd(), _nCmdShow );

		// cannot use because using this call with the MS WebBrowser
		// control destroys the control's window.
		//_pCtrl -> ShowWindow(_nCurrSel == _rKey);
	}

	return TRUE;
}

BOOL CSTabCtrl::CSTabPage::EnableWindows ( BOOL _bEnable )
{
	POSITION _rPos = m_ControlList.GetHeadPosition();
	CWnd * _pCtrl = NULL;

	while(_rPos)
	{
		_pCtrl = m_ControlList.GetNext(_rPos);

		::EnableWindow(_pCtrl -> GetSafeHwnd(), _bEnable );
	}

	return TRUE;
}

CSTabCtrl::CPageToControlsMap::CPageToControlsMap( )
{

}

CSTabCtrl::CPageToControlsMap::~CPageToControlsMap( )
{
	CPageToControlsMap::RemoveAll();
}


BOOL CSTabCtrl::CPageToControlsMap::AttachControl(INT _nTabNum,CWnd * _pControl)
{
	CSTabPage * _pTabPage = NULL;

	if(!Lookup(_nTabNum,_pTabPage) || !_pTabPage)
	{
		_pTabPage = new CSTabPage();
		SetAt(_nTabNum, _pTabPage);
	}

	if(_pTabPage -> AttachControl(_pControl))
	{
		// make sure control is hidden...
		::ShowWindow( _pControl -> GetSafeHwnd(), SW_HIDE );

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

BOOL CSTabCtrl::CPageToControlsMap::EnableWindows ( INT _nCurrPage, BOOL _bEnable )
{
	CList <CWnd *, CWnd *> * _pCtrlList = NULL;
	CSTabPage * _pTabPage = NULL;

	if(Lookup(_nCurrPage,_pTabPage) && _pTabPage)
	{
		return _pTabPage -> EnableWindows ( _bEnable );
	}
	else
	{
		return FALSE;
	}
}

BOOL CSTabCtrl::CPageToControlsMap::ShowWindows ( INT _nCurrPage, INT _nCmdShow )
{
	CList <CWnd *, CWnd *> * _pCtrlList = NULL;
	CSTabPage * _pTabPage = NULL;

	if(Lookup(_nCurrPage,_pTabPage) && _pTabPage)
	{
		return _pTabPage -> ShowWindows ( _nCmdShow );
	}
	else
	{
		return FALSE;
	}
}

void CSTabCtrl::CPageToControlsMap::RemoveAll( )
{
	POSITION _rPos = GetStartPosition();
	CSTabPage * _pValue = NULL;
	INT _rKey;

	while(_rPos)
	{
		GetNextAssoc(_rPos,_rKey,_pValue);
		delete _pValue;
	}

	CMap <INT, INT&,CSTabPage *, CSTabPage *>::RemoveAll();
}


/////////////////////////////////////////////////////////////////////////////
// CSTabCtrl

CSTabCtrl::CSTabCtrl()
{
	m_nPrevSel = -1;
}

CSTabCtrl::~CSTabCtrl()
{
}


BEGIN_MESSAGE_MAP(CSTabCtrl, CTabCtrl)
	//{{AFX_MSG_MAP(CSTabCtrl)
	ON_NOTIFY_REFLECT_EX(TCN_SELCHANGE, OnSelchange)
	ON_WM_DESTROY()
	ON_WM_CREATE()
	ON_WM_ENABLE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSTabCtrl message handlers

int CSTabCtrl::SetCurSel( int nItem )
{
	int _nRetVal = CTabCtrl::SetCurSel(nItem);

	NMHDR _nDummyNMHDR;
	LRESULT _nDummyLRESULT;

	OnSelchange(&_nDummyNMHDR,&_nDummyLRESULT);

	return _nRetVal;
}

BOOL CSTabCtrl::AttachControlToTab(CWnd * _pControl,
									INT _nTabNum)
{
	if(_nTabNum >= GetItemCount())
	{
		ASSERT(FALSE);
		return FALSE;
	}

	m_TabPagesMap.AttachControl (_nTabNum,_pControl);

	return TRUE;
}

void CSTabCtrl::OnEnable( BOOL bEnable )
{

	CTabCtrl::OnEnable(bEnable);
	
	INT _nTabPageCount (GetItemCount());

	for(INT i = 0; i < _nTabPageCount; i ++)
	{
		m_TabPagesMap.EnableWindows (i, bEnable);
	}
}

BOOL CSTabCtrl::OnSelchange(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	INT _nCurrSel = GetCurSel();

	// hide Previous pages controls...
	m_TabPagesMap.ShowWindows (m_nPrevSel, SW_HIDE);
	
	// show current pages controls.
	m_TabPagesMap.ShowWindows (_nCurrSel, SW_SHOW);

	m_nPrevSel = _nCurrSel;

	*pResult = 0;

	return FALSE;	// allow control to handle as well.
}

int CSTabCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	m_nPrevSel = -1;
	return 0;
}

void CSTabCtrl::OnDestroy() 
{
	CTabCtrl::OnDestroy();
	
	// clean up map.
	m_TabPagesMap.RemoveAll();
}