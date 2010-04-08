/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : xmlgener.h: header file.
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Generate XML File from the select statement.
**
** History:
**
** 22-Fev-2002 (uk$so01)
**    Created
** 27-May-2002 (uk$so01)
**    BUG #107880, Add the XML decoration encoding='UTF-8' or 'WINDOWS-1252'...
**    depending on the Ingres II_CHARSETxx.
**/

#if !defined (XMLGENERARTE_FILE_FROM_SELECT_STATEMENT_HEADER)
#define XMLGENERARTE_FILE_FROM_SELECT_STATEMENT_HEADER
#include "lsselres.h"
#include "dmlcolum.h"
#include "objidl.h"

//
// CLASS: CaColumnExport (Column layout for exporting)
// ************************************************************************************************
class CaColumnExport: public CaColumn
{
	DECLARE_SERIAL (CaColumnExport)
public:
	CaColumnExport(): CaColumn(), m_strNull(_T("NULL"))
	{
		m_strExportFormat = _T("text");
		m_nExportLength = 0;
		m_nExportScale = 0;
	}
	CaColumnExport(LPCTSTR lpszItem, LPCTSTR lpszType, int nDataType = -1):CaColumn(lpszItem, lpszType, nDataType)
	{
		m_strNull = _T("NULL");
		m_strExportFormat = _T("text");
		m_nExportLength = 0;
		m_nExportScale = 0;
	}
	CaColumnExport(const CaColumn& c): CaColumn()
	{
		CaColumn::Copy (c);
		m_strNull = _T("NULL");
		m_strExportFormat = _T("text");
		m_nExportLength = m_nLength;
		m_nExportScale = m_nScale;
	}
	CaColumnExport(const CaColumnExport& c){Copy (c);}

	virtual ~CaColumnExport(){}
	virtual void Serialize (CArchive& ar);

	void SetNull(LPCTSTR lpszText){m_strNull = lpszText;}
	void SetExportFormat(LPCTSTR lpszText){m_strExportFormat = lpszText;}
	void SetExportLength(int nSet){m_nExportLength = nSet;}
	void SetExportScale(int nSet){m_nExportScale = nSet;}
	CString GetNull() const {return m_strNull;}
	CString GetExportFormat() const {return m_strExportFormat;}
	int GetExportLength() const { return m_nExportLength;}
	int GetExportScale() const { return m_nExportScale;}
protected:
	void Copy (const CaColumnExport& c);
	CString m_strExportFormat;
	CString m_strNull; // Null exported as m_strNull;
	int m_nExportLength;
	int m_nExportScale;

};


//
// Base class CaGenerator: base class for generating row of CSV, DBF, XML, ...
// ************************************************************************************************
class CaGenerator: public CObject
{
public:
	enum {OUT_FILE= 0, OUT_STREAM};
	CaGenerator();
	virtual ~CaGenerator();
	CTypedPtrList< CObList, CaColumn* >& GetListColumn() {return m_listColumn;}
	CTypedPtrList< CObList, CaColumnExport* >& GetListColumnLayout(){return m_listColumnLayout;}
	BOOL IsTruncatedFields(){return m_bTruncatedFields;}

	virtual void Out(CaRowTransfer* pRow){ASSERT(FALSE);}
	virtual void End(BOOL bSuccess = TRUE){}
	UINT GetIDSError(){return m_nIDSError;}
	void SetIDSError(UINT nID){m_nGenerateError = nID;}
	void SetOutput(LPCTSTR  lpszFile);
	void SetOutput(IStream* pStream);


protected:
	UINT nFileOrStream;
	CString  m_strFileName;
	BOOL m_bFileCreated;
	BOOL m_bTruncatedFields;
	UINT m_nIDSError;
	//
	// Result header of the final fetch for exporting:
	CTypedPtrList< CObList, CaColumn* > m_listColumn; 
	//
	// [Optional]: the list of column modified for exporting:
	CTypedPtrList< CObList, CaColumnExport* > m_listColumnLayout;
protected:
	UINT     m_nGenerateError;
	CFile*   m_pFile;
	IStream* m_pStream;
};

class CaGenerateXml: public CaGenerator
{
public:
	CaGenerateXml(){m_strXslFile=_T(""); m_nHeaderLine = 2; m_strEncoding = _T("UTF-8");}
	virtual ~CaGenerateXml(){}

	void SetEncoding(LPCTSTR lpszEncoding){m_strEncoding = lpszEncoding;}
	void SetXslFile (LPCTSTR lpszXslFile){m_strXslFile = lpszXslFile;}
	void SetNullNA(LPCTSTR lpszNA){m_strNullNA = lpszNA;}
	int  GetHeaderLine(){return m_nHeaderLine;}
	virtual void Out(CaRowTransfer* pRow);
	virtual void End(BOOL bSuccess = TRUE);

	void ConstructRow (CaRowTransfer* pRow, CString& strXMLLine);
protected:
	CString m_strEncoding;
	CString m_strXslFile;
	//
	// For the null field, if the null value in CaColumnExport == m_strNullNA then
	// generate the native value othervise the value null in CaColumnExport is used.
	CString m_strNullNA;
	//
	// XML Header (in line count) before the result set.
	// The default is 2:
	//    <?xml version=\"1.0\"?>
	//    <!DOCTYPE resultset SYSTEM "II_SYSTEM\ingres\files\ingres.dtd">
	int m_nHeaderLine;
};


class CaGenerateXmlFile: public CaExecParamSelectLoop
{
public:
	CaGenerateXmlFile(BOOL bUseAnimateDialog = FALSE);
	~CaGenerateXmlFile(){};
	virtual int Run(HWND hWndTaskDlg = NULL);

	void SetAnimateTitle(LPCTSTR lpszTitle){m_strAnimateTitle = lpszTitle;}
	void SetEncoding(LPCTSTR lpszEncoding)
	{
		m_strEncoding = lpszEncoding;
		m_genXml.SetEncoding(m_strEncoding);
	}
	CString GetEncoding() const {return m_strEncoding;}

	BOOL DoGenerateFile (LPCTSTR lpszFile);   // Generate XML file
	BOOL DoGenerateFile (CFile& file);   // Generate XML file
	BOOL DoGenerateStream (IStream*& pSream); // Geneated IStream that contains XML elements.
protected:
	BOOL m_bUseAnimateDialog;
	CString m_strAnimateTitle;
	CString m_strEncoding;
	CaGenerateXml m_genXml;

};

void CALLBACK XML_fnGenerateXmlFile (LPVOID lpUserData, CaRowTransfer* pResultRow);
void XML_GenerateXslFile(CString strFile, CTypedPtrList< CObList, CaColumn* >& listHeader, CString strEncoding);


#endif // XMLGENERARTE_FILE_FROM_SELECT_STATEMENT_HEADER
