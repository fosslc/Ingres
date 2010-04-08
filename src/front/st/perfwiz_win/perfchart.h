/*
**
**  Name: perfchart.h
**
**  Description:
**	This header file contains all the necessary defines for creating a
**	PerfMon chart file.
**
**  History:
**	22-sep-1999 (somsa01)
**	    Created.
*/

#define PerfSignatureLen	20
#define DEFAULT_MAX_VALUES	100

#define szPerfChartSignature	L"PERF CHART"

#define dwLineSignature		(MAKELONG ('L', 'i'))

/* minor version 3 to support alert, report, log intervals in msec */
#define ChartMajorVersion    1
#define ChartMinorVersion    3

typedef struct PERFFILEHEADERSTRUCT
{
    WCHAR	szSignature[PerfSignatureLen];
    DWORD	dwMajorVersion;
    DWORD	dwMinorVersion;
    BYTE	abyUnused[100];
} PERFFILEHEADER;

typedef struct LINEVISUALSTRUCT
{
    COLORREF	crColor;
    int		iColorIndex;
    int		iStyle;
    int		iStyleIndex;
    int		iWidth;
    int		iWidthIndex;
} LINEVISUAL;

typedef struct _LINESTRUCT
{
   struct _LINESTRUCT			*pLineNext;
   int					bFirstTime;
   int					iLineType;
   LPWSTR				lnSystemName;
   PERF_OBJECT_TYPE			lnObject;
   LPWSTR				lnObjectName;
   PERF_COUNTER_DEFINITION		lnCounterDef;
   LPWSTR				lnCounterName;
   PERF_INSTANCE_DEFINITION		lnInstanceDef;
   LPWSTR				lnInstanceName;
   LPWSTR				lnPINName;
   LPWSTR				lnParentObjName;
   DWORD				lnUniqueID;  /* Of instance, if any */
   LONGLONG				lnNewTime;
   LONGLONG				lnOldTime;
   LONGLONG				lnOldTime100Ns;
   LONGLONG				lnNewTime100Ns;
   LONGLONG				lnaCounterValue[2];
   LONGLONG				lnaOldCounterValue[2];
   DWORD				lnCounterType;
   DWORD				lnCounterLength;
   LONGLONG				lnPerfFreq;
   LINEVISUAL				Visual;

   /* Chart-related fields */
   HPEN					hPen;
   int					iScaleIndex;
   FLOAT				eScale;
   FLOAT FAR				*lnValues;
   int					*aiLogIndexes;
   FLOAT				lnMaxValue;
   FLOAT				lnMinValue;
   FLOAT				lnAveValue;
   INT					lnValidValues;

   /* Alert-related fields */
   HBRUSH				hBrush;
   BOOL					bAlertOver;        /* over or under? */
   FLOAT				eAlertValue;       /* value to
							   ** compare
							   */
   LPWSTR				lpszAlertProgram;  /* program to run */
   BOOL					bEveryTime;        /* run every time
							   ** or once?
							   */
   BOOL					bAlerted;          /* alert happened
							   ** on line?
							   */

   /* Report-related fields */
   int					iReportColumn;
   struct _LINESTRUCT			*pLineCounterNext;
   int					xReportPos;
   int					yReportPos;
} LINESTRUCT;

typedef struct _graph_options
{
    BOOL	bLegendChecked;
    BOOL	bMenuChecked;
    BOOL	bLabelsChecked;
    BOOL	bVertGridChecked;
    BOOL	bHorzGridChecked;
    BOOL	bStatusBarChecked;
    INT		iVertMax;
    FLOAT	eTimeInterval;
    INT		iGraphOrHistogram;
    INT		GraphVGrid, GraphHGrid, HistVGrid, HistHGrid;
} GRAPH_OPTIONS;

typedef struct OPTIONSSTRUCT
{
    BOOL	bMenubar;
    BOOL	bToolbar;
    BOOL	bStatusbar;
    BOOL	bAlwaysOnTop;
} OPTIONS;

typedef struct DISKCHARTSTRUCT
{
    DWORD		dwNumLines;
    INT			gMaxValues;
    LINEVISUAL		Visual;
    GRAPH_OPTIONS	gOptions;
    BOOL		bManualRefresh;
    OPTIONS		perfmonOptions;
} DISKCHART;

typedef struct DISKSTRINGSTRUCT
{
    size_t	dwLength;
    INT_PTR	dwOffset;
} DISKSTRING;

typedef struct DISKLINESTRUCT
{
    int		iLineType;
    DISKSTRING	dsSystemName;
    DISKSTRING	dsObjectName;
    DISKSTRING	dsCounterName;
    DISKSTRING	dsInstanceName;
    DISKSTRING	dsPINName;
    DISKSTRING	dsParentObjName;
    DWORD	dwUniqueID;
    LINEVISUAL	Visual;
    int		iScaleIndex;
    FLOAT	eScale;
    BOOL	bAlertOver;
    FLOAT	eAlertValue;
    DISKSTRING	dsAlertProgram;
    BOOL	bEveryTime;
} DISKLINE;
