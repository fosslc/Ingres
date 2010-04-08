/*
** Copyright (c) 2004 Ingres Corporation
*/

# include      <compat.h>
# include      <si.h>
# include      <gl.h>
# include      <iicommon.h>
# include      <fe.h>
# include      <mapping.h>
# include      "ftframe.h"
# include      <ft.h>
# include      <frame.h>
# include      <frsctblk.h>
# include      "ftrange.h"
# include      <menu.h>
# include      <omenu.h>
# include      <lo.h>
# include      <uigdata.h>
# include      <tc.h>
# include      "mwhost.h"
# include      "mwform.h"


/*
** Name:	ftdata.c
**
** Description:	Global data for ft facility.
**
** History:
**
**	23-sep-96 (mcgem01)
**	    Created.
**      18-dec-1996 (canor01)
**          Include size of arrays moved to ftdata.c to keep sizeof happy.
**	21-apr-1997 (cohmi01)
**	    Add IIMWVersionInfo and include related mw headers to make 
**	    MWS version info global. (bug 81574)
**	07-jan-1997 (chech02)
**	    Included <si.h>.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**       7-mar-2001 (mosjo01)
**          Had to make MAX_CHAR more unique, IIMAX_CHAR_VALUE, due to conflict
**          between compat.h and system limits.h on (sqs_ptx).
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
**	19-May-2009 (kschendel) b122041
**	    DSRecords proc is void, not i4, fix here.
*/

/*
**
LIBRARY = IMPFRAMELIBDATA
**
*/

/* ftbuild.c */
GLOBALDEF       bool    samefrm = FALSE;

/* ftclear.c */
GLOBALDEF       bool    IIFTscScreenCleared = FALSE;


/* ftinit.c */

GLOBALDEF WINDOW        *frmodewin = NULL;
GLOBALDEF WINDOW        *FTwin = NULL;
GLOBALDEF WINDOW        *FTfullwin = NULL;
GLOBALDEF WINDOW        *FTcurwin = NULL;
GLOBALDEF WINDOW        *FTutilwin = NULL;  
GLOBALDEF WINDOW        *IIFTdatwin = NULL;
GLOBALDEF i4    	FTwmaxy = 0;
GLOBALDEF i4    	FTwmaxx = 0;
GLOBALDEF FRAME 	*FTiofrm = NULL;
GLOBALDEF FRS_EVCB      *IIFTevcb = NULL;
GLOBALDEF FRS_AVCB      *IIFTavcb = NULL;
GLOBALDEF FRS_GLCB      *IIFTglcb = NULL;
GLOBALDEF char  	*IIFTmpbMenuPrintBuf = NULL;
GLOBALDEF u_i2    	FTgtflg = 0;
GLOBALDEF VOID    	(*iigtclrfunc)() = NULL;
GLOBALDEF VOID    	(*iigtreffunc)() = NULL;
GLOBALDEF VOID    	(*iigtctlfunc)() = NULL;
GLOBALDEF VOID    	(*iigtsupdfunc)() = NULL;
GLOBALDEF bool    	(*iigtmreffunc)() = NULL;
GLOBALDEF VOID    	(*iigtslimfunc)() = NULL;
GLOBALDEF i4      	(*iigteditfunc)() = NULL;

GLOBALDEF i2    froperation[IIMAX_CHAR_VALUE+2] =
{
#ifndef EBCDIC
/*00*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*04*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*08*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*0C*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*10*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*14*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*18*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*1C*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*20*/  (i2)fdopSPACE,  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*24*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*28*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*2C*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*30*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*34*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*38*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*3C*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*40*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*44*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*48*/  (i2)fdopLEFT,   (i2)fdopERR,    (i2)fdopDOWN,   (i2)fdopUP,
/*4C*/  (i2)fdopRIGHT,  (i2)fdopERR,    (i2)fdopNEXT,   (i2)fdopERR,
/*50*/  (i2)fdopPREV,   (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*54*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*58*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopBEGF,
/*5C*/  (i2)fdopERR,    (i2)fdopENDF,   (i2)fdopERR,    (i2)fdopERR,
/*60*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*64*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*68*/  (i2)fdopLEFT,   (i2)fdopERR,    (i2)fdopDOWN,   (i2)fdopUP,
/*6C*/  (i2)fdopRIGHT,  (i2)fdopERR,    (i2)fdopNEXT,   (i2)fdopERR,
/*70*/  (i2)fdopPREV,   (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*74*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*78*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*7C*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*80*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*84*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*88*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*8C*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*90*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*94*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*98*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*9C*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*A0*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*A4*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*A8*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*AC*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*B0*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*B4*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*B8*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*BC*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*C0*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*C4*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*C8*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*CC*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*D0*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*D4*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*D8*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*DC*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*E0*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*E4*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*E8*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*EC*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*F0*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*F4*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*F8*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*FC*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopRUB,
#else /* EBCDIC */
/*00*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*04*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*08*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*0C*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*10*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*14*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*18*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*1C*/  (i2)fdopERR,    (i2)fdopTOTOP,  (i2)fdopERR,    (i2)fdopERR,
/*20*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*24*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*28*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*2C*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*30*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*34*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*38*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*3C*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*40*/  (i2)fdopSPACE,  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*44*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*48*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*4C*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*50*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*54*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*58*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*5C*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*60*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*64*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*68*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*6C*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*70*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*74*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*78*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*7C*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*80*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*84*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*88*/  (i2)fdopLEFT,   (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*8C*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*90*/  (i2)fdopERR,    (i2)fdopDOWN,   (i2)fdopUP,     (i2)fdopRIGHT,
/*94*/  (i2)fdopERR,    (i2)fdopNEXT,   (i2)fdopERR,    (i2)fdopPREV,
/*98*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*9C*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*A0*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*A4*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*A8*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*AC*/  (i2)fdopERR,    (i2)fdopBEGF,   (i2)fdopERR,    (i2)fdopERR,
/*B0*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*B4*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*B8*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*BC*/  (i2)fdopENDF,   (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*C0*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*C4*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*C8*/  (i2)fdopLEFT,   (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*CC*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*D0*/  (i2)fdopERR,    (i2)fdopDOWN,   (i2)fdopUP,     (i2)fdopRIGHT,
/*D4*/  (i2)fdopERR,    (i2)fdopNEXT,   (i2)fdopERR,    (i2)fdopPREV,
/*D8*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*DC*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*E0*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*E4*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*E8*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*EC*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*F0*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*F4*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*F8*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,
/*FC*/  (i2)fdopERR,    (i2)fdopERR,    (i2)fdopERR,    (i2)fdopRUB,
#endif /* EBCDIC */
        0
};

GLOBALDEF i1	fropflg[IIMAX_CHAR_VALUE+2] =
{
#ifndef EBCDIC
/*00*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*04*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*08*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*0C*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*10*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*14*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*18*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*1C*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*20*/	(i1)fdcmBRWS,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*24*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*28*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*2C*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*30*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*34*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*38*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*3C*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*40*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*44*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*48*/	(i1)fdcmBRWS,	(i1)fdcmNULL,	(i1)fdcmBRWS,	(i1)fdcmBRWS,
/*4C*/	(i1)fdcmBRWS,	(i1)fdcmNULL,	(i1)fdcmBRWS,	(i1)fdcmNULL,
/*50*/	(i1)fdcmBRWS,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*54*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*58*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmBRWS,
/*5C*/	(i1)fdcmNULL,	(i1)fdcmBRWS,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*60*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*64*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*68*/	(i1)fdcmBRWS,	(i1)fdcmNULL,	(i1)fdcmBRWS,	(i1)fdcmBRWS,
/*6C*/	(i1)fdcmBRWS,	(i1)fdcmNULL,	(i1)fdcmBRWS,	(i1)fdcmNULL,
/*70*/	(i1)fdcmBRWS,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*74*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*78*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*7C*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*80*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*84*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*88*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*8C*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*90*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*94*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*98*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*9C*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*A0*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*A4*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*A8*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*AC*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*B0*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*B4*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*B8*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*BC*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*C0*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*C4*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*C8*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*CC*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*D0*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*D4*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*D8*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*DC*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*E0*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*E4*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*E8*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*EC*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*F0*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*F4*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*F8*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,
/*FC*/	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,	(i1)fdcmNULL,

#else /* EBCDIC */
/*00*/	(i1)fdcmNULL,		(i1)fdcmINSRT,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*04*/	(i1)fdcmNULL,		(i1)fdcmBRWS|(i1)fdcmINSRT, (i1)fdcmNULL,	(i1)fdcmINSRT,
/*08*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmBRWS|(i1)fdcmINSRT,
/*0C*/	(i1)fdcmBRWS|(i1)fdcmINSRT,		(i1)fdcmBRWS|(i1)fdcmINSRT, (i1)fdcmBRWS|(i1)fdcmINSRT, (i1)fdcmNULL,
/*10*/	(i1)fdcmBRWS|(i1)fdcmINSRT,		(i1)fdcmNULL,		(i1)fdcmBRWS|(i1)fdcmINSRT, (i1)fdcmNULL,
/*14*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmBRWS|(i1)fdcmINSRT,		(i1)fdcmNULL,
/*18*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*1C*/	(i1)fdcmNULL,		(i1)fdcmBRWS|(i1)fdcmINSRT,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*20*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*24*/	(i1)fdcmNULL,		(i1)fdcmBRWS|(i1)fdcmINSRT,		(i1)fdcmBRWS|(i1)fdcmINSRT, (i1)fdcmBRWS|(i1)fdcmINSRT,
/*28*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*2C*/	(i1)fdcmNULL,		(i1)fdcmINSRT,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*30*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmINSRT,		(i1)fdcmNULL,
/*34*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmINSRT,
/*38*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*3C*/	(i1)fdcmNULL,		(i1)fdcmBRWS|(i1)fdcmINSRT,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*40*/	(i1)fdcmBRWS,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*44*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*48*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*4C*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*50*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*54*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*58*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*5C*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*60*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*64*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*68*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*6C*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*70*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*74*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*78*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*7C*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*80*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*84*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*88*/	(i1)fdcmBRWS,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*8C*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*90*/	(i1)fdcmNULL,		(i1)fdcmBRWS,		(i1)fdcmBRWS,		(i1)fdcmBRWS,
/*94*/	(i1)fdcmNULL,		(i1)fdcmBRWS,		(i1)fdcmNULL,		(i1)fdcmBRWS,
/*98*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*9C*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*A0*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*A4*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*A8*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*AC*/	(i1)fdcmNULL,		(i1)fdcmBRWS,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*B0*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*B4*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*B8*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*BC*/	(i1)fdcmNULL,		(i1)fdcmBRWS,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*C0*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*C4*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*C8*/	(i1)fdcmBRWS,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*CC*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*D0*/	(i1)fdcmNULL,		(i1)fdcmBRWS,		(i1)fdcmBRWS,		(i1)fdcmBRWS,
/*D4*/	(i1)fdcmNULL,		(i1)fdcmBRWS,		(i1)fdcmNULL,		(i1)fdcmBRWS,
/*D8*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*DC*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*E0*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*E4*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*E8*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*EC*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*F0*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*F4*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*F8*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
/*FC*/	(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,		(i1)fdcmNULL,
#endif /* EBCDIC */
	0
};

/*
**  Don't forget to reset this mapping when going between frames
*/

GLOBALDEF	struct	frsop	frsoper[MAX_CTRL_KEYS + KEYTBLSZ] =
{
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 1 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 2 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 3 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 4 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 5 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 6 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 7 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 8 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 9 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 10 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 11 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 12 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 13 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 14 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 15 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 16 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 17 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 18 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 19 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 20 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 21 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 22 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 23 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 24 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 25 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 26 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 27 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 28 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 29 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 30 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 31 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 32 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 33 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 34 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 35 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 36 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 37 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 38 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 39 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 40 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 41 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 42 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 43 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 44 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 45 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 46 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 47 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 48 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 49 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 50 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 51 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 52 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 53 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 54 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 55 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 56 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 57 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 58 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 59 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 60 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 61 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 62 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 63 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 64 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 65 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 66 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF,		/* frskey 67 */
	NULL,	fdopERR,	FRS_UF,	FRS_UF		/* frskey 68 */
};

/* ftlotus.c */

# ifdef MSDOS
GLOBALDEF i4    FTuselotus = TRUE;
# else
GLOBALDEF i4    FTuselotus = FALSE;
# endif /* MSDOS */

/* ftmushft.c */

GLOBALDEF       i4      mubutton = 0;
GLOBALDEF       i4      mutype = MU_TYPEIN;
GLOBALDEF       i4      mu_where = MU_BOTTOM;
GLOBALDEF       bool    mu_statline = FALSE;
GLOBALDEF       bool    mu_popup = FALSE;

/* ftptmenu.c */

GLOBALDEF MENU  IIFTcurmenu = NULL;

/* ftqbf.c */

GLOBALDEF       bool    FTqbf = FALSE;

/* ftrange.c */

GLOBALDEF       i2      FTrngtag = 0;

/* ftrun.c */

GLOBALDEF       FRAME   *FTfrm = NULL;

/* ftscrlfd.c */

GLOBALDEF       i4      IIFTdsi = FALSE;

/* ftsubmu.c */

GLOBALDEF       WINDOW  *MUswin = NULL;
GLOBALDEF       WINDOW  *MUsdwin = NULL;

/* ftsvinpt.c */

GLOBALDEF 	char	*fdrcvname = {0};
GLOBALDEF 	FILE	*fdrcvfile = NULL;
GLOBALDEF 	bool	fdrecover = FALSE;
GLOBALDEF 	bool	Rcv_disabled = FALSE;	
GLOBALDEF 	LOCATION fdrcvloc = {0};
GLOBALDEF	FLDHDR	*(*FTgethdr)() = NULL;
GLOBALDEF	FLDTYPE	*(*FTgettype)() = NULL;
GLOBALDEF	FLDVAL	*(*FTgetval)() = NULL;
GLOBALDEF	VOID	(*FTgeterr)() = NULL;
GLOBALDEF	STATUS	(*FTrngchk)() = NULL;
GLOBALDEF	i4	(*FTvalfld)() = NULL;
GLOBALDEF	VOID	(*FTdmpmsg)() = NULL;
GLOBALDEF	VOID	(*FTdmpcur)() = NULL;
GLOBALDEF	i4	(*FTfieldval)() = NULL;
GLOBALDEF	VOID	(*FTsqlexec)() = NULL;
GLOBALDEF	VOID	(*IIFTiaInvalidateAgg)() = NULL;
GLOBALDEF	VOID	(*IIFTpadProcessAggDeps)() = NULL;
GLOBALDEF	VOID	(*IIFTiaaInvalidAllAggs)() = NULL;
GLOBALDEF	i4	(*IIFTgtcGoToChk)() = NULL;
GLOBALDEF	i4	(*IIFTldrLastDsRow)() = NULL;
GLOBALDEF	void	(*IIFTdsrDSRecords)() = NULL;
GLOBALDEF	i4	IIFTltLabelTag = 0;


/* ftsyndsp.c */

GLOBALDEF       WINDOW  *(*IIFTdvl)() = NULL;

/* fttest.c */

GLOBALDEF   TCFILE  *IIFTcommfile = NULL;

/* fttmout.c */

GLOBALDEF       bool    IIFTtmout = FALSE;
GLOBALDEF       i4      IIFTntsNumTmoutSecs = 0;

/* ftusrinp.c */

GLOBALDEF       FILE    *FTdumpfile = NULL;

/* ftutil.c */

GLOBALDEF       bool    ftshmumap = TRUE;

/* ftvalid.c */

GLOBALDEF       bool    ftvalnext = TRUE;
GLOBALDEF       bool    ftvalprev = FALSE;
GLOBALDEF       bool    ftvalmenu = FALSE;
GLOBALDEF       bool    ftvalanymenu = FALSE;
GLOBALDEF       bool    ftvalfrskey = FALSE;
GLOBALDEF       bool    ftactnext = TRUE;
GLOBALDEF       bool    ftactprev = FALSE;

/* funckeys.c */
GLOBALDEF       i4      FKdebug = 0;

/* insrt.c */
GLOBALDEF       i4      FTfldno = 0;
GLOBALDEF       RGRFLD  *rgfptr = NULL;

/* mapping.c */

GLOBALDEF	struct	mapping	keymap[MAX_CTRL_KEYS + KEYTBLSZ + 4] =
{
	NULL,	fdopADUP,	(i1)fdcmINSRT,	(i1)DEF_INIT,	/* control A */
	NULL,	fdopSCRDN,	(i1)fdcmINBR,	(i1)DEF_INIT,	/* control B */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* control C */
	NULL,	fdopDELF,	(i1)fdcmINSRT,	(i1)DEF_INIT,	/* control D */
	NULL,	fdopMODE,	(i1)fdcmINSRT,	(i1)DEF_INIT,	/* control E */
	NULL,	fdopSCRUP,	(i1)fdcmINBR,	(i1)DEF_INIT,	/* control F */
	NULL,	fdopSCRLT,	(i1)fdcmINBR,	(i1)DEF_INIT,	/* control G */
	NULL,	fdopLEFT,	(i1)fdcmINBR,	(i1)DEF_INIT,	/* control H */
	NULL,	fdopNEXT,	(i1)fdcmINBR,	(i1)DEF_INIT,	/* control I */
	NULL,	fdopDOWN,	(i1)fdcmINBR,	(i1)DEF_INIT,	/* control J */
	NULL,	fdopUP,		(i1)fdcmINBR,	(i1)DEF_INIT,	/* control K */
	NULL,	fdopRIGHT,	(i1)fdcmINBR,	(i1)DEF_INIT,	/* control L */
	NULL,	fdopRET,	(i1)fdcmINBR,	(i1)DEF_INIT,	/* control M */
	NULL,	fdopNROW,	(i1)fdcmINBR,	(i1)DEF_INIT,	/* control N */
	NULL,	fdopSCRRT,	(i1)fdcmINBR,	(i1)DEF_INIT,	/* control O */
	NULL,	fdopPREV,	(i1)fdcmINBR,	(i1)DEF_INIT,	/* control P */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* control Q */
	NULL,	fdopBKWORD,	(i1)fdcmINBR,	(i1)DEF_INIT,	/* control R */
# ifdef PMFE
	NULL,	fdopWORD,	(i1)fdcmINBR,	(i1)DEF_INIT,	/* control S */
# else
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* control S */
# endif
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* control T */
	NULL,	fdopWORD,	(i1)fdcmINBR,	(i1)DEF_INIT,	/* control U */
	NULL,	fdopVERF,	(i1)fdcmINSRT,	(i1)DEF_INIT,	/* control V */
	NULL,	fdopREFR,	(i1)fdcmINBR,	(i1)DEF_INIT,	/* control W */
	NULL,	fdopCLRF,	(i1)fdcmINSRT,	(i1)DEF_INIT,	/* control X */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* control Y */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* control Z */
	NULL,	fdopMENU,	(i1)fdcmINBR,	(i1)DEF_INIT,	/* ESC */
	NULL,	fdopRUB,	(i1)fdcmINSRT,	(i1)DEF_INIT,	/* DEL */
	NULL,	fdopUP,		(i1)fdcmINBR,	(i1)DEF_INIT,	/* UP KEY */
	NULL,	fdopDOWN,	(i1)fdcmINBR,	(i1)DEF_INIT,	/* DOWN KEY */
	NULL,	fdopRIGHT,	(i1)fdcmINBR,	(i1)DEF_INIT,	/* RIGHT KEY */
	NULL,	fdopLEFT,	(i1)fdcmINBR,	(i1)DEF_INIT,	/* LEFT KEY */
	NULL,	fdopMENU,	(i1)fdcmINBR,	(i1)DEF_INIT,	/* PF 1 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 2 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 3 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 4 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 5 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 6 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 7 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 8 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 9 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 10 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 11 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 12 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 13 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 14 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 15 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 16 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 17 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 18 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 19 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 20 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 21 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 22 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 23 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 24 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 25 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 26 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 27 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 28 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 29 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 30 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 31 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 32 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 33 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 34 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 35 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 36 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 37 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 38 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET,	/* PF 39 */
	NULL,	ftNOTDEF,	(i1)fdcmNULL,	(i1)NO_DEF_YET	/* PF 40 */
};

GLOBALDEF       char    *menumap[MAX_MENUITEMS] = {0};

/* mwhost.c */
GLOBALDEF       i4      IIMWmws = FALSE;
GLOBALDEF  	MWSinfo	IIMWVersionInfo = {0};

