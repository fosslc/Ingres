/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : chartini.h, Header File 
**    Project  : Ingres II/Vdba.
**    Author   : UK Sotheavut
**    Purpose  : Initialize the pie chart data 
**
** History:
** xx-Sep-1998 (uk$so01)
**    created.
** 08-Apr-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating
**    the libraries (use libwctrl.lib).
*/

#ifndef CHART_INIT_HEADER
#define CHART_INIT_HEADER

extern COLORREF tab_colour [];
class CaPieInfoData;
class CaBarInfoData;

BOOL Pie_Create(CaPieInfoData* pPieInfo, int nHnode, void* pLocationDatamin, BOOL bDetail);
BOOL Pie_Create(CaPieInfoData* pPieInfo, int nHnode, void* pLocationDatamin);
BOOL Chart_Create(CaBarInfoData* pDiskInfo, int nNodeHandle);



#endif
