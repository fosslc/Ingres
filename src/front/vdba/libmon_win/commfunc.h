/********************************************************************
**
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
**
**  Project : libmon library for the Ingres Monitor ActiveX control
**
**    Source : commfunc.h
**
** History
**
**  05-Dec-2001 (schph01)
**      created
**  14-Fev-2002 (schph01)
**      SIR #106648, remove the use of repltbl.sh
********************************************************************/
#ifndef COMMONREPLICATION_HEADER
#define COMMONREPLICATION_HEADER

#include <st.h>
#include <lo.h>
#include <si.h>
#include <er.h>
#include <gl.h>
#include <generr.h>
#include <iicommon.h>
#include <me.h>
#include <cm.h>

#include <adf.h>
#include <gc.h>

#include "reptbdef.h"

#define  MAX_CONNECTION_NAME 209 /*   64      3     66     1 7       32    3   32 + 1 '\0'= 209 */
                                 /*"VNODENAME (\SERVERCLASS) (user:USERNAME)::DBNAME"   */
#define  MAX_VNODE_NAME      175 /*   64      3     66     1 7       32    1 + 1 '\0' = 175 */
                                 /*"VNODENAME (\SERVERCLASS) (user:USERNAME)"   */


#ifdef LIBMON_HEADER
// called in collisio.sc and tblinteg.sc
long   LIBMON_VdbaColl_tbl_fetch(long table_no,int force, LPERRORPARAMETERS lpError,TBLDEF *pRmTbl);
LPTSTR Vdba_DuplicateStringWithCRLF(LPTSTR pBufSrc);
#endif // LIBMON_HEADER

#endif // COMMONREPLICATION_HEADER