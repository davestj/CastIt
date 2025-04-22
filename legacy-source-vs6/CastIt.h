// CastIt.h : main header file for the CASTIT application
//

#if !defined(AFX_CASTIT_H__8BE6CF00_756D_4FB6_B3E6_25797320D27D__INCLUDED_)
#define AFX_CASTIT_H__8BE6CF00_756D_4FB6_B3E6_25797320D27D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CCastItApp:
// See CastIt.cpp for the implementation of this class
//

class CCastItApp : public CWinApp
{
public:
	CCastItApp();
	

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCastItApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CCastItApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CASTIT_H__8BE6CF00_756D_4FB6_B3E6_25797320D27D__INCLUDED_)
