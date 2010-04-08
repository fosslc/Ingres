/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
** Source   : taskanim.h, Header File 
** Project  : Ingres II/ (Vdba monitor as ActiveX control)
** Author   : UK Sotheavut (uk$so01)
** Purpose  : Data manipulation: access the lowlevel data through 
**            Library or COM module with animation
** History:
**
** 07-Dec-2001 (uk$so01)
**    Created.
*/

#if !defined (_IPMDML_x_TASKANIMAT_HEADER)
#define _IPMDML_x_TASKANIMAT_HEADER
#include "qryinfo.h" 
#include "usrexcep.h"
#include "tkwait.h"

class CaIpmQueryInfo;
class CdIpmDoc;
class CaRunObject
{
public:
	CaRunObject(CdIpmDoc* pIpmDoc): m_pIpmDoc(pIpmDoc) {}
	~CaRunObject(){}

	virtual BOOL Run(){return TRUE;};

protected:
	CdIpmDoc* m_pIpmDoc;
};


class CaExecParamQueryData: public CaExecParam
{
public:
	CaExecParamQueryData(CaRunObject* pRunObject);
	virtual ~CaExecParamQueryData(){}
	virtual int  Run(HWND hWndTaskDlg = NULL);
	virtual void OnTerminate(HWND hWndTaskDlg = NULL);
	virtual BOOL OnCancelled(HWND hWndTaskDlg = NULL);
	BOOL IsInterrupted(){return m_bInterrupted;}
	BOOL IsSucceeded(){return m_bSucceeded;}
private:
	CaRunObject* m_pRunObject;
	BOOL m_bInterrupted;
	BOOL m_bSucceeded;
};




BOOL IPM_QueryInfo (CaIpmQueryInfo* pQueryInfo, CPtrList& listInfoStruct);
BOOL IPM_QueryDetailInfo (CaIpmQueryInfo* pQueryInfo, LPVOID lpInfoStruct);


#endif // _IPMDML_x_TASKANIMAT_HEADER
