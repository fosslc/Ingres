/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlprop.cpp, Implementation File
**    Project  : SqlTest ActiveX.
**    Author   : UK Sotheavut (uk$so01)
**    Purpose  : Properties
**
** History:
**
** 06-Mar-2002 (uk$so01)
**    Created
**    SIR #106648, Split vdba into the small component ActiveX/COM
**/


#include "stdafx.h"
#include "sqlprop.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CaSqlQueryProperty::CaSqlQueryProperty()
{
	//
	// SQL Session:
	m_bAutocommit       = DEFAULT_AUTOCOMMIT;
	m_bReadlock         = DEFAULT_READLOCK;
	m_nTimeOut          = DEFAULT_TIMEOUT;
	m_nSelectMode       = DEFAULT_SELECTMODE;
	m_nBlock            = DEFAULT_SELECTBLOCK;
	//
	// Tab layout:
	m_nMaxTab           = DEFAULT_MAXTAB;
	m_nMaxTraceSize     = DEFAULT_MAXTRACE;
	m_bShowNonSelectTab = DEFAULT_SHOWNONSELECTTAB;
	m_bTraceActivated   = DEFAULT_TRACEACTIVATED;
	m_bTraceToTop       = DEFAULT_TRACETOTOP;
	//
	// Display:
	m_bGrid             = DEFAULT_GRID;
	m_nQepDisplayMode   = DEFAULT_QEPDISPLAYMODE;
	m_nXmlDisplayMode   = DEFAULT_XMLDISPLAYMODE;
	m_nF8Width          = DEFAULT_F8WIDTH;
	m_nF8Decimal        = DEFAULT_F8DECIMAL;
	m_bF8Exponential    = DEFAULT_F8EXPONENTIAL;
	m_nF4Width          = DEFAULT_F4WIDTH;
	m_nF4Decimal        = DEFAULT_F4DECIMAL;
	m_bF4Exponential    = DEFAULT_F4EXPONENTIAL;
	//
	// Font:
	m_hFont = (HFONT)GetStockObject (DEFAULT_GUI_FONT);
}

