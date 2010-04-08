/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**  Source   : taskxprm.cpp, Implementation file
**
**  Project  : Ingres II/ VDBA.
**  Author   : UK Sotheavut (uk$so01)
**  Purpose  : Allow interrupt task
**             (Parameters and execute task, ie DBAAddObject (...)
**
** History :
**	03-Feb-2000 (uk$so01)
**	    (bug #100324) Replace the named Mutex by the unnamed Mutex.
**	21-Dec-2000 (noifr01)
**	    (SIR 103548) use the new prototype of function GetGWlistLL()
**	    (pass FALSE for getting the whole list of gateways)
**	05-Jul-2001 (uk$so01)
**	    SIR #105199. Integrate & Generate XML.
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
#include <io.h>
#include "constdef.h"
#include "dmlcolum.h" 
#include "taskxprm.h"
#include "sqlselec.h"
#include "sqltrace.h"
#include "qpageres.h"
#include "sqlselec.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif



//
// FETCH ROW AND GENERATE XML FILE:
// CLASS: CaExecParamGenerateXMLfromSelect
// ************************************************************************************************
IMPLEMENT_SERIAL (CaExecParamGenerateXMLfromSelect, CObject, 1)
CaExecParamGenerateXMLfromSelect::CaExecParamGenerateXMLfromSelect(BOOL bDeleteTempFile):CaGenerateXmlFile(TRUE)
{
	m_bLoaded = FALSE;
	m_bClose = FALSE;
	m_bExecuteImmediately = TRUE;
	m_bDeleteTempFile = bDeleteTempFile;
	m_bAlreadyGeneratedXML = FALSE;
	m_bAlreadyGeneratedXSL = FALSE;
	m_bXMLSource = TRUE;
	m_strTempFileXML    = _T("");
	m_strTempFileXSL    = _T("");
	m_strTempFileXMLXSL = _T("");
}

void CaExecParamGenerateXMLfromSelect::Copy(const CaExecParamGenerateXMLfromSelect& c)
{
	// Case A = A
	ASSERT (&c != this);
	if (&c == this)
		return;
	SetEncoding(c.GetEncoding());
	
	// 
	// Extra copy here:
	m_bLoaded = c.m_bLoaded;
	m_bAlreadyGeneratedXML = c.m_bAlreadyGeneratedXML;
	m_bAlreadyGeneratedXSL = c.m_bAlreadyGeneratedXSL;
	m_bXMLSource = c.m_bXMLSource;
	m_strTempFileXML    = c.m_strTempFileXML;
	m_strTempFileXSL    = c.m_strTempFileXSL;
	m_strTempFileXMLXSL = c.m_strTempFileXMLXSL;
}

CaExecParamGenerateXMLfromSelect::~CaExecParamGenerateXMLfromSelect()
{
	//
	// Close the temporary files:
	if (m_bDeleteTempFile)
	{
		CStringArray arrayFile;
		CStringArray arrayExt;
		if (!m_strTempFileXML.IsEmpty())
		{
			DeleteFile(m_strTempFileXML);
			arrayFile.Add(m_strTempFileXML);
			arrayExt.Add(_T(".xml"));
		}
		if (!m_strTempFileXSL.IsEmpty())
		{
			DeleteFile(m_strTempFileXSL);
			arrayFile.Add(m_strTempFileXSL);
			arrayExt.Add(_T(".xsl"));
		}
		if (!m_strTempFileXMLXSL.IsEmpty())
		{
			DeleteFile(m_strTempFileXMLXSL);
			arrayFile.Add(m_strTempFileXMLXSL);
			arrayExt.Add(_T(".xml"));
		}

		CString strTemp;
		int nPos = -1;
		int i, nSize = arrayFile.GetSize();
		for (i=0; i<nSize; i++)
		{
			nPos = arrayFile[i].Find(arrayExt[i]);
			if (nPos != -1)
			{
				strTemp = arrayFile[i].Left(nPos +1);
				strTemp += _T("tmp");
				if (_taccess (strTemp, 0) == 0)
					DeleteFile (strTemp);
				else
				{
					strTemp = arrayFile[i].Left(nPos +1);
					strTemp += _T("TMP");
					if (_taccess (strTemp, 0) == 0)
						DeleteFile (strTemp);
				}
			}
		}
	}
}


void CaExecParamGenerateXMLfromSelect::Serialize (CArchive& ar)
{
	CaExecParam::Serialize(ar);
	if (ar.IsStoring())
	{
		ar << m_bAlreadyGeneratedXML;
		ar << m_bAlreadyGeneratedXSL;
		ar << m_bXMLSource;
		ar << m_strTempFileXML;
		ar << m_strTempFileXSL;
		ar << m_strTempFileXMLXSL;
	}
	else
	{
		m_bLoaded = TRUE;
		ar >> m_bAlreadyGeneratedXML;
		ar >> m_bAlreadyGeneratedXSL;
		ar >> m_bXMLSource;
		ar >> m_strTempFileXML;
		ar >> m_strTempFileXSL;
		ar >> m_strTempFileXMLXSL;
	}
}

static CString XML_GetTempFileName(LPCTSTR lpszExt)
{
	CString strFile;
	TCHAR tchszTempPath[MAX_PATH];
	TCHAR tchszTempFile[MAX_PATH];
	DWORD dwLen = _MAX_PATH;
	DWORD dw = GetTempPath(dwLen, tchszTempPath);
	if (dw == 0 || dw > _MAX_PATH)
		return _T("");

	dw = GetTempFileName (tchszTempPath, _T("xml"), 0, tchszTempFile);
	if (dw == 0)
		return _T("");
	strFile = tchszTempFile;

	int nFound = strFile.ReverseFind (_T('.'));
	if (lpszExt != NULL)
	{
		strFile = strFile.Left (nFound + 1);
		strFile+= lpszExt;
	}
	else
	{
		strFile = strFile.Left (nFound);
	}
	return strFile;
}

int CaExecParamGenerateXMLfromSelect::Run(HWND hWndTaskDlg)
{
	BOOL bRunQuery = FALSE;
	try
	{
		if (m_strTempFileXML.IsEmpty() || m_strTempFileXSL.IsEmpty() || m_strTempFileXMLXSL.IsEmpty())
		{
			bRunQuery = TRUE;
		}
		//
		// Generate the temporary file name:
		if (m_strTempFileXML.IsEmpty())
			m_strTempFileXML    = XML_GetTempFileName(_T("xml"));
		if (m_strTempFileXSL.IsEmpty())
			m_strTempFileXSL    = XML_GetTempFileName(_T("xsl"));
		if (m_strTempFileXMLXSL.IsEmpty())
			m_strTempFileXMLXSL = XML_GetTempFileName(_T("xml"));

		if (m_strTempFileXML.IsEmpty() || m_strTempFileXSL.IsEmpty() || m_strTempFileXMLXSL.IsEmpty())
		{
			return 1; // Fail to generate temporary files
		}

		if (bRunQuery) // The files have not been created yet
		{
			CString strCommonFile;
			if (m_bXMLSource)
				strCommonFile = m_strTempFileXML;
			else
				strCommonFile = m_strTempFileXMLXSL;

			//
			// Always generate the xsl (temporary) file to avoid saving the cursor
			// when storing the xml while the xsl is not displayed yet:
			CTypedPtrList< CObList, CaColumn* >& listHeader = GetListHeader();
			XML_GenerateXslFile(m_strTempFileXSL, listHeader, m_strEncoding);

			if (m_bXMLSource)
			{
				m_bAlreadyGeneratedXML = TRUE;
			}
			else
			{
				m_genXml.SetXslFile (m_strTempFileXSL);
				m_bAlreadyGeneratedXSL = TRUE;
				//
				// Generate reference to the XSL file for transformation:
			}

			m_genXml.SetOutput (strCommonFile);
			CTypedPtrList< CObList, CaColumn* >& listColumn = m_genXml.GetListColumn();
			POSITION pos = listHeader.GetHeadPosition();
			while (pos != NULL)
			{
				CaColumn* pCol = listColumn.GetNext(pos);
				CaColumn* pNewCol = new CaColumn ((const CaColumn&)(*pCol));
				listColumn.AddTail(pNewCol);
			}

			return CaGenerateXmlFile::Run(hWndTaskDlg);
		}
		else // The data has been fetched
		{
			const int nSize = 2048;
			int nHeaderLineCount = m_genXml.GetHeaderLine();
			TCHAR tchszBuffer[nSize + 1];
			CString strLine = _T("");
			if (m_bXMLSource)
			{
				if (!m_bAlreadyGeneratedXML)
				{
					ASSERT(m_bAlreadyGeneratedXSL);
					//
					// Generate XML Source file from the Transformed XML file:
					CFile rf(m_strTempFileXMLXSL, CFile::modeRead|CFile::shareDenyNone);
					CFile wf(m_strTempFileXML, CFile::modeCreate|CFile::modeWrite);

					DWORD dw = rf.Read(tchszBuffer, nSize);
					tchszBuffer[dw] = _T('\0');
					//
					// Skip the header lines:
					int  nLineCount = 0;
					BOOL bConstructHeader = TRUE;
					while (dw > 0)
					{
						strLine += tchszBuffer;

						if (bConstructHeader)
						{
							int nPos0x0d = strLine.Find((TCHAR)0x0d);
							while (nPos0x0d != -1)
							{
								CString strHeaderLine = strLine.Left(nPos0x0d);
								strHeaderLine.MakeLower();
								if (strHeaderLine.Find(_T("<?xml-stylesheet type")) == -1)
								{
									strHeaderLine = strLine.Left(nPos0x0d);
									wf.Write ((const void*)(LPCTSTR)strHeaderLine, strHeaderLine.GetLength());
									wf.Write ((const void*)consttchszReturn, _tcslen(consttchszReturn));
								}

								if (strLine.GetAt(nPos0x0d+1) == 0x0a)
									strLine = strLine.Mid(nPos0x0d+2);
								else
									strLine = strLine.Mid(nPos0x0d+1);
								nLineCount++;
								bConstructHeader = (nLineCount < nHeaderLineCount);
								if (strLine.IsEmpty())
								{
									dw = rf.Read(tchszBuffer, nSize);
									tchszBuffer[dw] = _T('\0');

									break;
								}

								nPos0x0d = strLine.Find((TCHAR)0x0d);
							}
							dw = rf.Read(tchszBuffer, nSize);
							tchszBuffer[dw] = _T('\0');
						}
						else
						{
							wf.Write((const void*)(LPCTSTR)strLine, strLine.GetLength());
							strLine = _T("");

							dw = rf.Read(tchszBuffer, nSize);
							tchszBuffer[dw] = _T('\0');
						}
					}
					wf.Flush();
					m_bAlreadyGeneratedXML = TRUE;
				}
			}
			else
			{
				strLine = _T("");
				if (!m_bAlreadyGeneratedXSL)
				{
					ASSERT(m_bAlreadyGeneratedXML);
					//
					// Generate Transformed XML file from the XML Source file:
					CFile rf(m_strTempFileXML, CFile::modeRead|CFile::shareDenyNone);
					CFile wf(m_strTempFileXMLXSL, CFile::modeCreate|CFile::modeWrite);

					DWORD dw = rf.Read(tchszBuffer, nSize);
					tchszBuffer[dw] = _T('\0');
					//
					// Skip the header lines:
					int  nLineCount = 0;
					BOOL bConstructHeader = TRUE;

					while (dw > 0)
					{
						strLine += tchszBuffer;

						if (bConstructHeader)
						{
							int nPos0x0d = strLine.Find((TCHAR)0x0d);
							while (nPos0x0d != -1)
							{
								CString strHeaderLine = strLine.Left(nPos0x0d);
								nLineCount++;
								wf.Write ((const void*)(LPCTSTR)strHeaderLine, strHeaderLine.GetLength());
								wf.Write ((const void*)consttchszReturn, _tcslen(consttchszReturn));
								if (nLineCount == 2)
								{
									//
									// Insert the <?xml-stylesheet ...> at the 3th line:
									CString strXslHref;
									strXslHref.Format (_T("<?xml-stylesheet type=\"text/xsl\" href=\"%s\"?>"), (LPCTSTR)m_strTempFileXSL);
									strXslHref += consttchszReturn;
									wf.Write ((const void*)(LPCTSTR)strXslHref, strXslHref.GetLength());
								}

								if (strLine.GetAt(nPos0x0d+1) == 0x0a)
									strLine = strLine.Mid(nPos0x0d+2);
								else
									strLine = strLine.Mid(nPos0x0d+1);

								bConstructHeader = (nLineCount < 2);
								if (!bConstructHeader)
									break;
								if (strLine.IsEmpty())
								{
									dw = rf.Read(tchszBuffer, nSize);
									tchszBuffer[dw] = _T('\0');
									break;
								}

								nPos0x0d = strLine.Find((TCHAR)0x0d);
							}
							if (!strLine.IsEmpty())
							{
								wf.Write((const void*)(LPCTSTR)strLine, strLine.GetLength());
								strLine = _T("");
							}
							dw = rf.Read(tchszBuffer, nSize);
							tchszBuffer[dw] = _T('\0');
						}
						else
						{
							wf.Write((const void*)(LPCTSTR)strLine, strLine.GetLength());
							strLine = _T("");

							dw = rf.Read(tchszBuffer, nSize);
							tchszBuffer[dw] = _T('\0');
						}
					}
					wf.Flush();
					m_bAlreadyGeneratedXSL = TRUE;
				}
			}
		}
	}
	catch (CeSqlException e)
	{
		TRACE1 ("Raise exception, CaExecParamGenerateXMLfromSelect::Run(): %s\n", (LPCTSTR)e.GetReason());
		m_strMsgFailed = e.GetReason();
	}
	catch(CFileException* e)
	{
		CString strErr;
		e->GetErrorMessage(strErr.GetBuffer(_MAX_PATH), _MAX_PATH);
		strErr.ReleaseBuffer();
		m_strMsgFailed = strErr;
		e->Delete();
	}
	catch (...)
	{
		TRACE0 ("Raise exception, CaExecParamGenerateXMLfromSelect::Run() unknown error\n");
		m_strMsgFailed = _T("Raise exception, CaExecParamGenerateXMLfromSelect::Run() unknown error");
	}

	return 0;
}


