/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
** Source   : xmlgener.cpp: implementation file.
** Project  : Meta data library 
** Author   : Sotheavut UK (uk$so01)
** Purpose  : Generate XML File from the select statement.
**
** History:
**	22-Fev-2002 (uk$so01)
**	    Created
**	27-May-2002 (uk$so01)
**	    BUG #107880, Add the XML decoration encoding='UTF-8' or
**	    'WINDOWS-1252'... depending on the Ingres II_CHARSETxx.
**	10-Oct-2003 (uk$so01)
**	    SIR #106648, (Integrate 2.65 features for EDBC)
**	02-Feb-2004 (somsa01)
**	    Updated to build with .NET 2003 compiler.
**	20-Aug-2008 (whiro01)
**	    Remove redundant <afx...> includes (already in stdafx.h)
**	16-Mar-2009 (drivi01)
**	    Include fstream header in stdafx.h instead of here.
*/

#include "stdafx.h"
#include "xmlgener.h"
#include "sessimgr.h"
#include "dmlcolum.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/*
**  This is a sample of XSL file used to transform the XML
**  ------------------------------------------------------

<?xml version='1.0'?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/TR/WD-xsl">
<xsl:template match="/">
	<html>
	<body>
		<table border="1" bgcolor="white">
			<tr>
				<th> COL1 </th>
				...............
				<th> COLn </th>
			</tr>
			<xsl:apply-templates select="resultset/row" />
		</table>
	</body>
	</html>
</xsl:template>
<xsl:template match="row">
	<tr>
	<xsl:apply-templates select="column" />
	</tr>
</xsl:template>
<xsl:template match="column">
<td>
	<font color="#000000" size="1" face="MS Sans Serif">
	<xsl:value-of select="."/>
	</font>
</td>
</xsl:template>
</xsl:stylesheet>
*/
void XML_GenerateXslFile(CString strFile, CTypedPtrList< CObList, CaColumn* >& listHeader, CString strEncoding)
{
	USES_CONVERSION;
	TCHAR tchszTab = _T('\t');
	ofstream f (T2A((LPTSTR)(LPCTSTR)strFile) ,ios::out);
	CString strEnc;
	strEnc.Format(_T("'%s'"), (LPCTSTR)strEncoding);

	f << "<?xml version='1.0' " << "encoding=" << strEnc  <<" ?>" << endl;
	f << "<xsl:stylesheet xmlns:xsl=\"http://www.w3.org/TR/WD-xsl\">" << endl;
	f << "<xsl:template match=\"/\">" << endl;
	f << tchszTab << "<html>" << endl;
	f << tchszTab << "<body>" << endl;
	f << tchszTab << tchszTab << "<table border=\"1\" bgcolor=\"white\">" << endl;
	f << tchszTab << tchszTab <<tchszTab <<"<tr>" << endl;
	
	POSITION posColumn = listHeader.GetHeadPosition();
	while (posColumn != NULL)
	{
		CaColumn* pCol = listHeader.GetNext(posColumn);
		f << tchszTab << tchszTab <<tchszTab << tchszTab << "<th>"<<(LPCTSTR)pCol->GetName()<<"</th>" << endl;
	}
	f << tchszTab << tchszTab <<tchszTab <<"</tr>" << endl;
	f << tchszTab << tchszTab <<tchszTab <<"<xsl:apply-templates select=\"resultset/row\" />" << endl;
	f << tchszTab << tchszTab << "</table>" << endl;
	f << tchszTab << "</body>" << endl;
	f << tchszTab << "</html>" << endl;
	f << "</xsl:template>" << endl;

	f << "<xsl:template match=\"row\">" << endl;
	f << tchszTab << "<tr>" << endl;
	f << tchszTab << "<xsl:apply-templates select=\"column\" />" << endl;
	f << tchszTab << "</tr>" << endl;
	f << "</xsl:template>" << endl;

	f << "<xsl:template match=\"column\">" << endl;
	f << "<td>" << endl;
	f << tchszTab << "<font color=\"#000000\" size=\"1\" face=\"MS Sans Serif\">" << endl;
	f << tchszTab << "<xsl:value-of select=\".\"/>" << endl;
	f << tchszTab << "</font>" << endl;
	f << "</td>" << endl;
	f << "</xsl:template>" << endl;

	f << "</xsl:stylesheet>" << endl;
}

//
// CLASS: CaColumnExport (Column layout for exporting)
// ************************************************************************************************
IMPLEMENT_SERIAL (CaColumnExport, CaColumn, 1)
void CaColumnExport::Serialize (CArchive& ar)
{
	CaColumn::Serialize(ar);
	if (ar.IsStoring())
	{
		ar << m_strNull;
		ar << m_strExportFormat;
		ar << m_nExportLength;
		ar << m_nExportScale;
	}
	else
	{
		ar >> m_strNull;
		ar >> m_strExportFormat;
		ar >> m_nExportLength;
		ar >> m_nExportScale;
	}
}

void CaColumnExport::Copy(const CaColumnExport& c)
{
	CaColumn::Copy (c);
	m_strNull = c.GetNull();
	m_strExportFormat = c.GetExportFormat();
	m_nExportLength = c.GetExportLength();
	m_nExportScale = c.GetExportScale();
}

//
// Base class CaGenerator: base class for generating row of CSV, DBF, XML, ...
// ************************************************************************************************
CaGenerator::CaGenerator(): m_pFile(NULL), m_bFileCreated(FALSE), m_bTruncatedFields(FALSE)
{
	m_strFileName = _T("");
	m_nIDSError = 0;
	m_nGenerateError = 0;
}

CaGenerator::~CaGenerator()
{
	End();
	while (!m_listColumn.IsEmpty())
		delete m_listColumn.RemoveHead();
	while (!m_listColumnLayout.IsEmpty())
		delete m_listColumnLayout.RemoveHead();
}

void CaGenerator::SetOutput(LPCTSTR  lpszFile)
{
	nFileOrStream = OUT_FILE;
	m_strFileName = lpszFile;
}

void CaGenerator::SetOutput(IStream* pStream)
{
	nFileOrStream = OUT_STREAM;

}



void CaGenerateXml::Out(CaRowTransfer* pRow)
{
	CString strXMLLine;
	if (!m_bFileCreated)
	{
		CString strFieDTD;
		TCHAR* pEnv;
		pEnv = _tgetenv(consttchszII_SYSTEM);
		if (pEnv)
		{
			strFieDTD = pEnv;
			strFieDTD += consttchszIngFiles;
			strFieDTD += consttchszIngDTD;
		}
		else
		{
			strFieDTD = consttchszIngDTD;
		}

		//
		// Create xml file.
		if (nFileOrStream == OUT_FILE)
		{
			m_pFile = new CFile (m_strFileName, CFile::modeCreate|CFile::modeWrite);
		}
		else
		{

		}

		strXMLLine.Format (_T("<?xml version='1.0' encoding='%s' ?>"), (LPCTSTR)m_strEncoding);
		strXMLLine += consttchszReturn;
		strXMLLine += _T("<!DOCTYPE resultset SYSTEM ") + CString(_T("\"")) + strFieDTD + CString (_T("\">"));
		strXMLLine += consttchszReturn;
		if (!m_strXslFile.IsEmpty())
		{
			m_nHeaderLine+= 1;
			CString strXslHref;
			strXslHref.Format (_T("<?xml-stylesheet type=\"text/xsl\" href=\"%s\"?>"), (LPCTSTR)m_strXslFile);
			strXMLLine += strXslHref;
			strXMLLine += consttchszReturn;
		}

		if (nFileOrStream == OUT_FILE)
		{
			m_pFile->Write((LPCTSTR)strXMLLine, strXMLLine.GetLength());
		}
		else
		{

		}

		strXMLLine.Format (_T("<resultset>%s"), consttchszReturn);
		if (nFileOrStream == OUT_FILE)
		{
			m_pFile->Write((LPCTSTR)strXMLLine, strXMLLine.GetLength());
		}
		else
		{

		}
		m_bFileCreated = TRUE;
	}

	ConstructRow (pRow, strXMLLine);
	if (!strXMLLine.IsEmpty())
	{
		//
		// Write the XML row into the XML file:
		strXMLLine+= consttchszReturn;
		if (nFileOrStream == OUT_FILE)
		{
			m_pFile->Write((LPCTSTR)strXMLLine, strXMLLine.GetLength());
		}
		else
		{

		}
	}
}

void CaGenerateXml::End(BOOL bSuccess)
{
	CString strXMLLine;
	strXMLLine.Format (_T("</resultset>%s"), consttchszReturn);
	if (m_pFile || m_pStream)
	{
		if (bSuccess)
		{
			if (nFileOrStream == OUT_FILE)
			{
				m_pFile->Write((LPCTSTR)strXMLLine, strXMLLine.GetLength());
				m_pFile->Flush();

				delete m_pFile;
			}
			else
			{


			}
		}
		else
		{
			if (nFileOrStream == OUT_FILE)
			{
				CString strFileName = m_pFile->GetFilePath();
				delete m_pFile;
				DeleteFile(strFileName);
			}
			else
			{

			}
		}
	}
	m_pFile = NULL;
	m_pStream = NULL;
}

void CaGenerateXml::ConstructRow (CaRowTransfer* pRow, CString& strXMLLine)
{
	TCHAR tchszTab = _T('\t');
	CString strXMLSpecial = _T("&<>\"\'");
	CString strTag;
	CStringList& strlListTuple = pRow->GetRecord();
	CArray <UINT, UINT>& arrayNullIndicator = pRow->GetArrayFlag();
	int nIndex = 0;
	int nLayoutCount = m_listColumnLayout.GetCount();

	if (nLayoutCount > 0)
	{
		ASSERT(nLayoutCount == strlListTuple.GetCount());
		if (nLayoutCount != strlListTuple.GetCount())
		{
			strXMLLine = _T("");
			//
			// The result of the select statement has been changed since
			// the last fetch for previewing:
			m_nIDSError = m_nGenerateError;
			throw (int)1;
		}
	}
	
	CaColumn*       pCol1 = NULL;
	CaColumnExport* pCol2 = NULL;
	CString  strField;
	POSITION posField  = strlListTuple.GetHeadPosition();
	POSITION pos1 = m_listColumn.GetHeadPosition();
	POSITION pos2 = m_listColumnLayout.GetHeadPosition();

	strXMLLine = _T("<row>");
	strXMLLine+= consttchszReturn;
	while (posField != NULL && pos1 != NULL)
	{
		strField = strlListTuple.GetNext(posField);
		BOOL bIsNull = ((arrayNullIndicator[nIndex] & CAROW_FIELD_NULL) != 0);

		pCol1 = m_listColumn.GetNext(pos1);
		if (nLayoutCount > 0)
		{
			pCol2 = m_listColumnLayout.GetNext(pos2);
			if (m_strNullNA.CompareNoCase (pCol2->GetNull()) == 0)
				bIsNull = FALSE;
			if (bIsNull)
				strField = pCol2->GetNull();
		}

		CString strColumnName = (pCol2==NULL)? pCol1->GetName(): pCol2->GetName();
		strTag.Format (_T("%c<column column_name=\"%s\">"), tchszTab, (LPCTSTR)strColumnName);
		strXMLLine += strTag;

		if (strField.FindOneOf (strXMLSpecial) == -1)
			strXMLLine += strField;
		else
		{
			strXMLLine += _T("<![CDATA[");
			strXMLLine += strField;
			strXMLLine += _T("]]>");
		}
		strTag.Format (_T("</column>%s"), consttchszReturn);
		strXMLLine += strTag; 

		nIndex++;
	}
	strXMLLine += _T("</row>");
}




void CALLBACK XML_fnGenerateXmlFile (LPVOID lpUserData, CaRowTransfer* pResultRow)
{
	CaGenerateXml* pData = (CaGenerateXml*)lpUserData;
	pData->Out(pResultRow);
	delete pResultRow;
}


CaGenerateXmlFile::CaGenerateXmlFile(BOOL bUseAnimateDialog):CaExecParamSelectLoop(INTERRUPT_USERPRECIFY)
{
	m_bUseAnimateDialog = bUseAnimateDialog;
	m_strEncoding = _T("UTF-8");
	//UKS SetStatement(lpszStatement);
}

BOOL CaGenerateXmlFile::DoGenerateFile (CFile& file)
{
	CaGenerateXml genXml;
	genXml.SetEncoding(m_strEncoding);
	SetUserHandlerResult((LPVOID)&genXml, XML_fnGenerateXmlFile);
//UKS	genXml.SetOutput(lpszFile);


	return TRUE;
}


BOOL CaGenerateXmlFile::DoGenerateFile (LPCTSTR lpszFile)
{
	CaGenerateXml genXml;
	SetUserHandlerResult((LPVOID)&genXml, XML_fnGenerateXmlFile);
	genXml.SetOutput(lpszFile);

	CaSession* pCurrentSession = GetSession();
	ASSERT(pCurrentSession);
	if (!pCurrentSession)
		return FALSE;
	BOOL bUseAnimate = FALSE;;
#if defined (_ANIMATION_)
	bUseAnimate = m_bUseAnimateDialog;
#endif
	if (bUseAnimate)
	{
		pCurrentSession->SetSessionNone();

		CxDlgWait dlgWait(m_strAnimateTitle, NULL);
		dlgWait.SetDeleteParam(FALSE);
		dlgWait.SetExecParam (this);
		dlgWait.SetUseAnimateAvi(AVI_FETCHR);
		dlgWait.SetUseExtraInfo();
		dlgWait.SetShowProgressBar(FALSE);
		dlgWait.SetHideCancelBottonAlways(TRUE);
		dlgWait.DoModal();
	}
	else
	{
		pCurrentSession->Activate();
		Run();
	}

	return TRUE;
}

BOOL CaGenerateXmlFile::DoGenerateStream (IStream*& pSream)
{
	CaGenerateXml genXml;
	SetUserHandlerResult((LPVOID)&genXml, XML_fnGenerateXmlFile);

	return FALSE;
}


int CaGenerateXmlFile::Run(HWND hWndTaskDlg)
{
	SetUserHandlerResult((LPVOID)&m_genXml, XML_fnGenerateXmlFile);

	int nRes = CaExecParamSelectLoop::Run(hWndTaskDlg);
	m_genXml.End((nRes == 0));
	return nRes;
}


