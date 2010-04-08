/*
**Copyright (c) 2004 Ingres Corporation
*/

#include        <compat.h>
#include	<gl.h>
#include	<me.h>
#include	<st.h>
#include	<lo.h>
#include	<nm.h>
#include	<er.h>
#include	<cm.h>
#include	<cs.h>
#include	"erloc.h"
#include	<errno.h>

/*
**  erdata.c  -    ER Data
**
**  Description
**      File added to hold all GLOBALDEFs in ER facility.
**      This is for use in DLLs only.
**
LIBRARY = IMPCOMPATLIBDATA
**  History:
**      12-Sep-95 (fanra01)
**          Created.
**	23-sep-1996 (canor01)
**	    Updated.
**	14-nov-2004 (mcgem01)
**	    Updated to reflect three letter language designations
**	    recommended by i18n team.  Also added PTB for Brazilian 
**	    Portugese.
**
*/

/*
**  This array of structer includes internal language code, class address, both
**  fast and slow of internal file id and internal stream id for each language.
**  Please show erloc.h, if you would like to know about ERMULTI structer.
*/
#ifdef	VMS
GLOBALDEF ERMULTI	ERmulti[ER_MAXLANGUAGE] = {{0,0,0,0,{{0,0},{0,0}}}};
#else
GLOBALDEF ERMULTI	ERmulti[ER_MAXLANGUAGE] = {{0,0,0,0,{0,0}}};
#endif

/*
**  Language table
**
**  This table is used to get directory name and internal code.
**  (in ERlangcode and ERlangstr)
*/
GLOBALDEF ER_LANGT_DEF	ER_langt[] = {
	ERx("english"),  	1   ,
	ERx("jpn"),	 	2   ,
	ERx("fra"),	   	3   ,
	ERx("flemish"),  	4   ,
	ERx("deu"),   		5   ,
	ERx("ita"),	  	6   ,
	ERx("nld"),	    	7   ,
	ERx("fin"),	  	8   ,
	ERx("dan"),	   	9   ,
	ERx("esn"),	  	10  ,
	ERx("sve"),	  	11  ,
	ERx("nor"),		12  ,
	ERx("heb"),		13  ,
	ERx("kor"),		14  ,
	ERx("sch"),		15  ,
	ERx("tha"),		16  ,
	ERx("ell"),		17  ,
	ERx("ptb"),		18  ,
	NULL,0
};
