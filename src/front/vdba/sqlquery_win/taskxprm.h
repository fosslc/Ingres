/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : taskxprm.h, Header file 
**    Project  : Ingres II/VDBA 
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Allow interupt task
**               (Parameters and execute task, ie DBAAddObject (...)
**
** History:
**
** xx-Aug-1998 (uk$so01)
**   Created.
** 05-Jul-2001 (uk$so01)
**    SIR #105199. Integrate & Generate XML.
** 23-Oct-2001 (uk$so01)
**    SIR #106057
**    Transform code to be an ActiveX Control
** 22-Fev-2002 (uk$so01)
**    SIR #107133. Use the select loop instead of cursor when we get
**    all rows at one time.
*/

#if !defined (TASK_EXEC_PARAM_HEADER)
#define TASK_EXEC_PARAM_HEADER
#include "synchron.h"
#include "tkwait.h"
#include "lsselres.h"
#include "xmlgener.h"


class CaSession;
//
// FETCH ROW AND GENERATE XML FILE:
// ************************************************************************************************
class CaExecParamGenerateXMLfromSelect: public CaGenerateXmlFile
{
	DECLARE_SERIAL (CaExecParamGenerateXMLfromSelect)
public:
	CaExecParamGenerateXMLfromSelect(BOOL bDeleteTempFile = TRUE);
	void Copy(const CaExecParamGenerateXMLfromSelect& c);
	virtual ~CaExecParamGenerateXMLfromSelect();
	virtual void Serialize (CArchive& ar);
	virtual int Run(HWND hWndTaskDlg = NULL);

	void SetMode (BOOL bXMLSource){m_bXMLSource = bXMLSource;}
	BOOL GetMode (){return m_bXMLSource;}
	CString GetFileXML(){return m_strTempFileXML;}
	CString GetFileXSL(){return m_strTempFileXMLXSL;}
	CString GetFileXSLTemplate(){return m_strTempFileXSL;}
	BOOL IsAlreadyGeneratedXML(){return m_bAlreadyGeneratedXML;}
	BOOL IsAlreadyGeneratedXSL(){return m_bAlreadyGeneratedXSL;}

	BOOL    m_bLoaded;
	BOOL    m_bClose;
	BOOL    m_bDeleteTempFile;
	BOOL    m_bAlreadyGeneratedXML;
	BOOL    m_bAlreadyGeneratedXSL;
	BOOL    m_bXMLSource;           // TRUE = indicate the source file mode (m_strTempFileXML)
	CString m_strTempFileXML;       // XML Data file
	CString m_strTempFileXSL;       // XSL file for transforming XML file
	CString m_strTempFileXMLXSL;    // XML file that has been applied the transformation using XSL
};


#endif // TASK_EXEC_PARAM_HEADER