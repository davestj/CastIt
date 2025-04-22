// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__15D6E089_13E7_4DE0_B983_F9AB6A655F33__INCLUDED_)
#define AFX_STDAFX_H__15D6E089_13E7_4DE0_B983_F9AB6A655F33__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxsock.h>		// MFC socket extensions

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.
#define WM_TRAY_NOTIFY WM_APP+1000
#define IDR_SYSTRAY 666

#endif // !defined(AFX_STDAFX_H__15D6E089_13E7_4DE0_B983_F9AB6A655F33__INCLUDED_)
