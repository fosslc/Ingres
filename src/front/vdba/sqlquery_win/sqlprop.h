/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : sqlprop.h, Header File
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

#if !defined (DTAGSETT_HEADER)
#define DTAGSETT_HEADER

#define DEFAULT_AUTOCOMMIT       FALSE
#define DEFAULT_READLOCK         FALSE
#define DEFAULT_TIMEOUT            0
#define DEFAULT_SELECTMODE         0
#define DEFAULT_SELECTBLOCK      200

#define DEFAULT_MAXTAB             9
#define DEFAULT_MAXTRACE         100
#define DEFAULT_TRACETOTOP       FALSE
#define DEFAULT_TRACEACTIVATED   TRUE
#define DEFAULT_SHOWNONSELECTTAB TRUE

#define DEFAULT_QEPDISPLAYMODE     1
#define DEFAULT_XMLDISPLAYMODE     0
#define DEFAULT_GRID             FALSE
#define DEFAULT_F4WIDTH           10
#define DEFAULT_F4DECIMAL          3
#define DEFAULT_F4EXPONENTIAL    FALSE
#define DEFAULT_F8WIDTH           10
#define DEFAULT_F8DECIMAL          3
#define DEFAULT_F8EXPONENTIAL    FALSE

#define SQLMASK_BASE             1
//
// Sql Session:
#define SQLMASK_AUTOCOMMIT       (SQLMASK_BASE)
#define SQLMASK_READLOCK         (SQLMASK_BASE <<  1)
#define SQLMASK_TIMEOUT          (SQLMASK_BASE <<  2)
#define SQLMASK_FETCHBLOCK       (SQLMASK_BASE <<  3)
//
// Tab Layout:
#define SQLMASK_TRACETAB         (SQLMASK_BASE <<  4)
#define SQLMASK_MAXTAB           (SQLMASK_BASE <<  5)
#define SQLMASK_MAXTRACESIZE     (SQLMASK_BASE <<  6)
#define SQLMASK_SHOWNONSELECTTAB (SQLMASK_BASE <<  7)
//
// Display:
#define SQLMASK_FLOAT4           (SQLMASK_BASE <<  8)
#define SQLMASK_FLOAT8           (SQLMASK_BASE <<  9)
#define SQLMASK_QEPPREVIEW       (SQLMASK_BASE << 10)
#define SQLMASK_XMLPREVIEW       (SQLMASK_BASE << 11)
#define SQLMASK_SHOWGRID         (SQLMASK_BASE << 12)
//
// Font:
#define SQLMASK_FONT             (SQLMASK_BASE << 13)

#define SQLMASK_LOAD             (SQLMASK_BASE << 14)

class CaSqlQueryProperty: public CObject
{
public:
	CaSqlQueryProperty();
	virtual ~CaSqlQueryProperty(){}

	//
	// Get Methods:

	//
	// SQL Session:
	BOOL IsAutoCommit(){return m_bAutocommit;}
	BOOL IsReadLock(){return m_bReadlock;}
	long GetTimeout(){return m_nTimeOut;};
	long GetSelectMode(){return m_nSelectMode;};
	long GetSelectBlock(){return m_nBlock;};
	//
	// Tab layout:
	long GetMaxTab(){return m_nMaxTab;}
	long GetMaxTrace(){return m_nMaxTraceSize;}
	BOOL IsShowNonSelect(){return m_bShowNonSelectTab;}
	BOOL IsTraceActivated(){return m_bTraceActivated;}
	BOOL IsTraceToTop(){return m_bTraceToTop;}
	//
	// Display:
	BOOL IsGrid(){return m_bGrid;}
	long GetQepDisplayMode(){return m_nQepDisplayMode;}
	long GetXmlDisplayMode(){return m_nXmlDisplayMode;}
	long GetF8Width(){return m_nF8Width;}
	long GetF8Decimal(){return m_nF8Decimal;}
	BOOL IsF8Exponential(){return m_bF8Exponential;}
	long GetF4Width(){return m_nF4Width;}
	long GetF4Decimal(){return m_nF4Decimal;}
	BOOL IsF4Exponential(){return m_bF4Exponential;}
	//
	// Font:
	HFONT GetFont(){return m_hFont;}


	//
	// Set Methods:
	void SetAutoCommit(BOOL bSet){m_bAutocommit = bSet;}
	void SetReadLock(BOOL bSet){m_bReadlock = bSet;}
	void SetTimeout(long nVal){m_nTimeOut = nVal;}
	void SetSelectMode(long nVal){m_nSelectMode = nVal;}
	void SetSelectBlock(long nVal){m_nBlock = nVal;}
	//
	// Tab layout:
	void SetMaxTab(long nVal){m_nMaxTab = nVal;}
	void SetMaxTrace(long nVal){m_nMaxTraceSize = nVal;}
	void SetShowNonSelect(BOOL bSet){m_bShowNonSelectTab = bSet;}
	void SetTraceActivated(BOOL bSet){m_bTraceActivated = bSet;}
	void SetTraceToTop(BOOL bSet){m_bTraceToTop = bSet;}
	//
	// Display:
	void SetGrid(BOOL bSet){m_bGrid = bSet;}
	void SetQepDisplayMode(long nVal){m_nQepDisplayMode = nVal;}
	void SetXmlDisplayMode(long nVal){m_nXmlDisplayMode= nVal;}
	void SetF8Width(long nVal){m_nF8Width= nVal;}
	void SetF8Decimal(long nVal){m_nF8Decimal= nVal;}
	void SetF8Exponential(BOOL bSet){m_bF8Exponential = bSet;}
	void SetF4Width(long nVal){m_nF4Width= nVal;}
	void SetF4Decimal(long nVal){m_nF4Decimal= nVal;}
	void SetF4Exponential(BOOL bSet){m_bF4Exponential = bSet;}
	//
	// Font:
	void SetFont(HFONT hSet){m_hFont = hSet;}

protected:
	//
	// SQL Session:
	BOOL  m_bAutocommit;        // Autocommit state
	BOOL  m_bReadlock;          // Readlock  state
	long  m_nTimeOut;           // Time out
	long  m_nSelectMode;        // 0=All, 1=Block of N row(s)
	long  m_nBlock;             // Only if m_nSelectMode = 1
	//
	// Tab layout:
	long  m_nMaxTab;            // Max Result Tabs
	long  m_nMaxTraceSize;      // Max Trace buffer size
	BOOL  m_bShowNonSelectTab;  // Show non-select tab
	BOOL  m_bTraceActivated;    // If TRUE, select the trace page on activation. (ie when enable TRACE, the trace Tab is selected)
	BOOL  m_bTraceToTop    ;    // If TRUE and m_bTraceActivated = TRUE, the Trace Tab is always activated when you run the statement.
	//
	// Display:
	BOOL  m_bGrid;              // Row Grid of the List Control.
	long  m_nQepDisplayMode;    // 0=Normal, 1=Display qep in preview mode.
	long  m_nXmlDisplayMode;    // 0=XML,    1=Display XML in preview mode (Transformed by XSL).
	long  m_nF8Width;           // total of characters (including m_iDecimalF8)
	long  m_nF8Decimal;         // decimal places
	BOOL  m_bF8Exponential;     // TRUE, exponential format
	long  m_nF4Width;           // total of characters (including m_iDecimalF4)
	long  m_nF4Decimal;         // decimal places
	BOOL  m_bF4Exponential;     // TRUE, exponential format
	//
	// Font:
	HFONT m_hFont;              // Font used by sqlact.
};


#endif // DTAGSETT_HEADER
