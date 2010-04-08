/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : eapstep2.h: header file
**    Project  : EXPORT ASSISTANT 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Common behaviour of Export Assistant in page 2.
**
** History:
**
** 16-Jan-2001 (uk$so01)
**    SIR  #106952, Add new Ingres Export Assistant & Cleanup.
**    Created
** 22-Aug-2003 (uk$so01)
**    SIR #106648, Add silent mode in IEA. 
**/

#if !defined (COMMON_EXPORT_PAGE2_HEADER)
#define COMMON_EXPORT_PAGE2_HEADER
#include "exportdt.h"

class CuIeaPPage2CommonData
{
public:
	CuIeaPPage2CommonData();
	~CuIeaPPage2CommonData();

	BOOL CreateDoubleListCtrl (CWnd* pWnd, CRect& r);
	void DisplayResults(CWnd* pWnd, ExportedFileType expft);
	BOOL Findish(CaIEAInfo& data);
	void SaveExportParameter(CWnd* pWnd);

protected:
	CuListCtrlDouble m_wndHeaderRows;
	CaIngresExportHeaderRowItemData m_rowInfo;
	CaColumnExport* m_pDummyColumn;

};

//
// Here we have an extra column at the position 0 = "Source Data".
void CALLBACK DisplayExportListCtrlLower(void* pItemData, CuListCtrl* pListCtrl, int nItem, BOOL bUpdate, void* pInfo);


#endif // COMMON_EXPORT_PAGE2_HEADER