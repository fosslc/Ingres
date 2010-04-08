/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qresurow.cpp, Implementation File 
**    Project  : CA-OpenIngres/Visual DBA.
**    Author   : UK Sotheavut. (uk$so01)
**    Purpose  : The rows data. (Tuple) 
**
** History: 
**
**	xx-Dec-1997 (uk$so01)
**	    Created.
**	05-Jul-2001 (uk$so01)
**	    Integrate & Generate XML.
**	23-Oct-2001 (uk$so01)
**	    SIR #106057
**	    Transform code to be an ActiveX Control
**	22-Fev-2002 (uk$so01)
**	    SIR #107133. Use the select loop instead of cursor when we get
**	    all rows at one time.
**	27-May-2002 (uk$so01)
**	    BUG #107880, Add the XML decoration encoding='UTF-8' or
**	    'WINDOWS-1252'...  depending on the Ingres II_CHARSETxx.
**	02-feb-2004 (somsa01)
**	    Changed CFile::WriteHuge()/ReadHuge() to CFile::Write()/Read(), as
**	    WriteHuge()/ReadHuge() is obsolete and in WIN32 programming
**	    Write()/Read() can also write more than 64K-1 bytes of data.
*/

#include "stdafx.h"
#include "rcdepend.h"
#include "qresurow.h"
#include "qpageres.h"
#include "qpageqep.h"
#include "qpagegen.h"
#include "qpageraw.h"
#include "qpagexml.h"


IMPLEMENT_SERIAL (CaQueryResultRowsHeader, CObject, 1)
void CaQueryResultRowsHeader::Serialize(CArchive& ar)
{
	try
	{
		if (ar.IsStoring())
		{
			ar << m_nAlign;
			ar << m_nWidth;
			ar << m_strHeader;
		}
		else
		{
			ar >> m_nAlign;
			ar >> m_nWidth;
			ar >> m_strHeader;
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (CArchiveException* e)
	{
		MSGBOX_ArchiveExceptionMessage (e);
		e->Delete();
	}
	catch (...)
	{
		AfxMessageBox (_T("CaQueryResultRowsHeader::Serialize: Unknown error."));
	}
}


IMPLEMENT_SERIAL (CaQueryResultRows, CObject, 2)
CaQueryResultRows::~CaQueryResultRows()
{
	while (!m_listHeaders.IsEmpty())
		delete m_listHeaders.RemoveHead();
	while (!m_listAdjacent.IsEmpty())
		delete m_listAdjacent.RemoveHead();
}

void CaQueryResultRows::Serialize (CArchive& ar)
{
	try
	{
		m_listAdjacent.Serialize (ar);
		m_listHeaders.Serialize (ar);
		m_aItemData.Serialize(ar);
		if (ar.IsStoring())
		{
			ar << m_nHeaderCount;
		}
		else
		{
			ar >> m_nHeaderCount;
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (CArchiveException* e)
	{
		MSGBOX_ArchiveExceptionMessage (e);
		e->Delete();
	}
	catch (...)
	{
		AfxMessageBox (_T("CaQueryResultRows::Serialize: Unknown error."));
	}
}

//
// Implementation of CaQueryPageData
// ---------------------------------
IMPLEMENT_SERIAL (CaQueryPageData, CObject, 1)
CaQueryPageData::CaQueryPageData()
{
	m_bShowStatement = FALSE;
	m_nStart = 0;
	m_nEnd = 0;
	m_strStatement = _T("");
	m_nTabImage  = -1;
}

void CaQueryPageData::Serialize (CArchive& ar)
{
	try
	{
		if (ar.IsStoring())
		{
			ar << m_bShowStatement;
			ar << m_nStart;
			ar << m_nEnd;
			ar << m_strStatement;
			ar << m_strDatabase;
			ar << m_nTabImage;
		}
		else
		{
			ar >> m_bShowStatement;
			ar >> m_nStart;
			ar >> m_nEnd;
			ar >> m_strStatement;
			ar >> m_strDatabase;
			ar >> m_nTabImage;
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (CArchiveException* e)
	{
		MSGBOX_ArchiveExceptionMessage (e);
		e->Delete();
	}
	catch (...)
	{
		AfxMessageBox (_T("CaQueryPageData::Serialize: Unknown error."));
	}
}


//
// Implementation of CaQuerySelectPageData
// ---------------------------------------
IMPLEMENT_SERIAL (CaQuerySelectPageData, CaQueryPageData, 1)
void CaQuerySelectPageData::Serialize (CArchive& ar)
{
	CaQueryPageData::Serialize (ar);
	try
	{
		m_listRows.Serialize(ar);
		if (ar.IsStoring())
		{
			ar << m_nSelectedItem;
			ar << m_strFetchInfo;
			ar << m_nTopIndex;
			ar << m_nMaxTuple;
		}
		else
		{
			ar >> m_nSelectedItem;
			ar >> m_strFetchInfo;
			ar >> m_nTopIndex;
			ar >> m_nMaxTuple;
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (CArchiveException* e)
	{
		MSGBOX_ArchiveExceptionMessage (e);
		e->Delete();
	}
	catch (...)
	{
		AfxMessageBox (_T("CaQuerySelectPageData::Serialize: Unknown error."));
	}
}


CWnd* CaQuerySelectPageData::LoadPage(CWnd* pParent)
{
	CuDlgSqlQueryPageResult* pPage = NULL;
	ASSERT (pParent);
	if (!pParent)
		return NULL;

	pPage = new CuDlgSqlQueryPageResult(pParent);
	pPage->Create (IDD_SQLQUERY_PAGERESULT, pParent);
	pPage->NotifyLoad (this);
	return (CWnd*)pPage;
}
	



//
// Implementation of CaQueryNonSelectPageData
// ------------------------------------------
IMPLEMENT_SERIAL (CaQueryNonSelectPageData, CaQueryPageData, 1)
void CaQueryNonSelectPageData::Serialize (CArchive& ar)
{
	CaQueryPageData::Serialize (ar);
	try
	{
		if (ar.IsStoring())
		{
			ar << m_bShowStatement;
			ar << m_nStart;
			ar << m_nEnd;
			ar << m_strStatement;
			ar << m_strDatabase;
		}
		else
		{
			ar >> m_bShowStatement;
			ar >> m_nStart;
			ar >> m_nEnd;
			ar >> m_strStatement;
			ar >> m_strDatabase;
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (CArchiveException* e)
	{
		MSGBOX_ArchiveExceptionMessage (e);
		e->Delete();
	}
	catch (...)
	{
		AfxMessageBox (_T("CaQueryNonSelectPageData::Serialize: Unknown error."));
	}
}


CWnd* CaQueryNonSelectPageData::LoadPage(CWnd* pParent)
{
	CuDlgSqlQueryPageGeneric* pPage = NULL;
	ASSERT (pParent);
	if (!pParent)
		return NULL;

	pPage = new CuDlgSqlQueryPageGeneric(pParent);
	pPage->Create (IDD_SQLQUERY_PAGEGENERIC, pParent);
	pPage->NotifyLoad (this);
	return (CWnd*)pPage;
}



//
// Implementation of CaQueryQepPageData
// ------------------------------------
IMPLEMENT_SERIAL (CaQueryQepPageData, CaQueryPageData, 1)
void CaQueryQepPageData::Serialize (CArchive& ar)
{
	CaQueryPageData::Serialize (ar);
	try
	{
		if (ar.IsStoring())
		{
			ar << m_bShowStatement;
			ar << m_nStart;
			ar << m_nEnd;
			ar << m_strStatement;
			ar << m_strDatabase;
			ar << m_pQepDoc;
		}
		else
		{
			ar >> m_bShowStatement;
			ar >> m_nStart;
			ar >> m_nEnd;
			ar >> m_strStatement;
			ar >> m_strDatabase;
			ar >> m_pQepDoc;
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (CArchiveException* e)
	{
		MSGBOX_ArchiveExceptionMessage (e);
		e->Delete();
	}
	catch (...)
	{
		AfxMessageBox (_T("CaQueryQepPageData::Serialize: Unknown error."));
	}
}


CWnd* CaQueryQepPageData::LoadPage(CWnd* pParent)
{
	CuDlgSqlQueryPageQep* pPage = NULL;
	ASSERT (pParent);
	if (!pParent)
		return NULL;

	pPage = new CuDlgSqlQueryPageQep(pParent);
	pPage->Create (IDD_SQLQUERY_PAGEQEP, pParent);
	pPage->NotifyLoad (this);
	return (CWnd*)pPage;
}




//
// Implementation of CaQueryRawPageData
// ------------------------------------
IMPLEMENT_SERIAL (CaQueryRawPageData, CaQueryPageData, 1)
void CaQueryRawPageData::Serialize (CArchive& ar)
{
	CaQueryPageData::Serialize (ar);
	try
	{
		if (ar.IsStoring())
		{
			ar << m_strTrace;
			ar << m_nLine;
			ar << m_nLength;
			ar << m_bLimitReached;
		}
		else
		{
			ar >> m_strTrace;
			ar >> m_nLine;
			ar >> m_nLength;
			ar >> m_bLimitReached;
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (CArchiveException* e)
	{
		MSGBOX_ArchiveExceptionMessage (e);
		e->Delete();
	}
	catch (...)
	{
		AfxMessageBox (_T("CaQueryRawPageData::Serialize: Unknown error."));
	}
}


CWnd* CaQueryRawPageData::LoadPage(CWnd* pParent)
{
	CuDlgSqlQueryPageRaw* pPage = NULL;
	ASSERT (pParent);
	if (!pParent)
		return NULL;

	pPage = new CuDlgSqlQueryPageRaw(pParent);
	pPage->Create (IDD_SQLQUERY_PAGERAW, pParent);
	pPage->NotifyLoad (this);
	return (CWnd*)pPage;
}


//
// Implementation of CaQueryXMLPageData
// ------------------------------------
IMPLEMENT_SERIAL (CaQueryXMLPageData, CaQueryPageData, 1)
CaQueryXMLPageData::CaQueryXMLPageData():CaQueryPageData()
{
	//
	// Only generate for load/save. The file names will be transfered
	// to another CaExecParamGenerateXMLfromSelect. So do not destroy the temporary files !
	BOOL bDeleteTempFile = FALSE; 
	m_pParam = new CaExecParamGenerateXMLfromSelect(bDeleteTempFile);
	m_pParam->SetEncoding(theApp.GetCharacterEncoding());
}

CaQueryXMLPageData::~CaQueryXMLPageData()
{
	if (m_pParam)
		delete m_pParam;
}


void CaQueryXMLPageData::Serialize (CArchive& ar)
{
	try
	{
		const int nSize = 2048;
		TCHAR tchszBuffer[nSize + 1];
		CaQueryPageData::Serialize (ar);
		ASSERT(m_pParam);
		if (m_pParam)
			m_pParam->Serialize(ar);
		if (ar.IsStoring())
		{
			//
			// Save the content of temporary files:
			// The content is stored as consecutive blocks. The block is: [block_size block_data]
			// The content is end when the block_size = 0;
			if (m_pParam->IsAlreadyGeneratedXML())
			{
				CFile rf(m_pParam->GetFileXML(), CFile::modeRead);
				DWORD dw = rf.Read(tchszBuffer, nSize);
				while (dw > 0)
				{
					ar << dw;
					ar.Write (tchszBuffer, dw);
					dw = rf.Read(tchszBuffer, nSize);
				}
				ar << 0;
			}

			if (!m_pParam->GetFileXSLTemplate().IsEmpty())
			{
				//
				// Always store the xsl template file: (the xsl file used to stransform the xml file)
				CFile rf(m_pParam->GetFileXSLTemplate(), CFile::modeRead);
				DWORD dw = rf.Read(tchszBuffer, nSize);
				while (dw > 0)
				{
					ar << dw;
					ar.Write (tchszBuffer, dw);
					dw = rf.Read(tchszBuffer, nSize);
				}
				ar << 0;
			}

			if (m_pParam->IsAlreadyGeneratedXSL())
			{
				CFile rf(m_pParam->GetFileXSL(), CFile::modeRead);
				DWORD dw = rf.Read(tchszBuffer, nSize);
				while (dw > 0)
				{
					ar << dw;
					ar.Write (tchszBuffer, dw);
					dw = rf.Read(tchszBuffer, nSize);
				}
				ar << 0;
			}
		}
		else
		{
			//
			// Create the temporary files and restore the content
			if (m_pParam->IsAlreadyGeneratedXML())
			{
				CFile wf(m_pParam->GetFileXML(), CFile::modeCreate|CFile::modeWrite);
				DWORD dw = 0;
				ar >> dw;
				while (dw > 0)
				{
					ar.Read (tchszBuffer, dw);
					wf.Write(tchszBuffer, dw);
					wf.Flush();
					ar >> dw;
				}
			}

			if (!m_pParam->GetFileXSLTemplate().IsEmpty())
			{
				CFile wf(m_pParam->GetFileXSLTemplate(), CFile::modeCreate|CFile::modeWrite);
				DWORD dw = 0;
				ar >> dw;
				while (dw > 0)
				{
					ar.Read (tchszBuffer, dw);
					wf.Write(tchszBuffer, dw);
					wf.Flush();
					ar >> dw;
				}
			}

			if (m_pParam->IsAlreadyGeneratedXSL())
			{
				CFile wf(m_pParam->GetFileXSL(), CFile::modeCreate|CFile::modeWrite);
				DWORD dw = 0;
				ar >> dw;
				while (dw > 0)
				{
					ar.Read (tchszBuffer, dw);
					wf.Write(tchszBuffer, dw);
					wf.Flush();
					ar >> dw;
				}
			}
		}
	}
	catch (CMemoryException* e)
	{
		theApp.OutOfMemoryMessage();
		e->Delete();
	}
	catch (CArchiveException* e)
	{
		MSGBOX_ArchiveExceptionMessage (e);
		e->Delete();
	}
	catch (...)
	{
		AfxMessageBox (_T("CaQueryXMLPageData::Serialize: Unknown error."));
	}
}


CWnd* CaQueryXMLPageData::LoadPage(CWnd* pParent)
{
	CuDlgSqlQueryPageXML* pPage = NULL;
	ASSERT (pParent);
	if (!pParent)
		return NULL;

	pPage = new CuDlgSqlQueryPageXML(pParent);
	pPage->Create (IDD_SQLQUERY_PAGEXML, pParent);
	CaExecParamGenerateXMLfromSelect* pParam = pPage->GetQueryRowParam();
	pPage->NotifyLoad (this);
	return (CWnd*)pPage;
}
