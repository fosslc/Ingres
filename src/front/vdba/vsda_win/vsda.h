/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : vsda.h : main header file for VSDA.OCX
**    Project  : INGRES II/ Visual Schema Diff Control (vsda.ocx).
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : main header file for VSDA.OCX
**
** History:
**
** 26-Aug-2002 (uk$so01)
**    SIR #109220, Initial version of vsda.ocx.
**    SIR #109220, created
** 18-Aug-2004 (uk$so01)
**    BUG #112841 / ISSUE #13623161, Allow user to cancel the Comparison
**    operation.
*/

#if !defined(AFX_VSDA_H__CC2DA2BC_B8F1_11D6_87D8_00C04F1F754A__INCLUDED_)
#define AFX_VSDA_H__CC2DA2BC_B8F1_11D6_87D8_00C04F1F754A__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#if !defined( __AFXCTL_H__ )
	#error include 'afxctl.h' before including this file
#endif
#include "resource.h"       // main symbols
#include "comsessi.h"
#include "wmusrmsg.h"
#include "synchron.h"

class CappSda : public COleControlModule
{
public:
	BOOL InitInstance();
	int ExitInstance();

	CmtSessionManager  m_sessionManager; 
	CaSychronizeInterrupt m_synchronizeInterrupt;

};

extern CappSda theApp;
extern const GUID CDECL _tlid;
extern const WORD _wVerMajor;
extern const WORD _wVerMinor;

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VSDA_H__CC2DA2BC_B8F1_11D6_87D8_00C04F1F754A__INCLUDED)
