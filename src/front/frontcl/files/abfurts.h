/*
**	Copyright (c) 2007 Ingres Corporation
**	All rights reserved.
*/

#ifndef ABFURTS_H_INCLUDED
#define ABFURTS_H_INCLUDED
/**
** Name:	abfurts.h -	User level ABF RTS Definitions File.
**
** Description:
**	Defines the structures of the objects written to the "abextract.c"
**	file, and included by that file.   (On UNIX, "abextract.c" is written
**	instead of an "abextract.mar."; On MPE/iX, "ABEX.C" is written.)
**
**	This file is derived from the "abfrts.h" file.
**
** History:
**	Revision 5.0  1986  daveb
**	Initial revision.
**
**	Revision 6.1  88/09  wong
**	Added DB_DATA_VALUE definition, etc., to install script.
**
**	89/08  wong  Renamed ABRTSHLPROC to ABRTSFUNCS.
**	89/08  billc  Added GLOBARRAY for application globals and constants.
**	Revision 6.4  89/10  wong
**	Added typedef for i1, f4, f8, and macro for text strings.
**	89/12  wong  Added TEXT_STR macro and associated types.
**	90/09  mikes  Added CHAR_ARRAY macro and associated types.
**	92/03  mgw   Changed size of abrrwoutput and abrgfoutput struct members
**	             to match size of field on form.
**	03 Aug 92 (vijay)
**		Added semicolons for the last elements in struct defines.
**	18-Nov-1992 (fredb)
**		Expanded the definition comment to reveal MPE/iX file name.
**		Arranged history comments in chronological order.
**	29-jan-93 (fraser)
**		Define int as long for MS-DOS operating systems.
**      04-Dec-1998 (merja01)
**              Set i4 to int for Digital UNIX. This change had been
**              previously sparse branched.
**      04-May-1999 (hweho01)
**              Set i4 to int for ris_u64 (AIX 64-bit port).
**	
**	Revision 6.1  88/09  wong
**	Added DB_DATA_VALUE definition, etc., to install script.
**
**	Revision 5.0  1986  daveb
**	Initial revision.
**	12-Sep-2000 (hanch04)
**	    There should only be one define for i4.
**      21-apr-1999 (hanch04)
**          replace long with i4
**	7-Jan-2004 (schka24)
**	    Update this copy (!) of DB_DATA_VALUE.
**	11-Oct-2007 (kiria01) b118421
**	    Include guard against repeated inclusion.
**/

/* casts used by DS to write the initialization of the ABRTSFRM abrfrvar */

# define CAST_FRAMEP	(FRAME *)
# define CAST_RTSVAR	(ABRTSVAR *)
# define CAST_MENU	(MENU)
# define CAST_FNPTR	(FUNCPTR)
# define CAST_RTSFORM	(ABRTSFO *)
# define CAST_PTR	(char *)

# ifndef NULL
# define NULL	0
# endif

/* puns to avoid pulling in other otherwise uneeded headers */

#ifdef  MSDOS				 
#define	int		long		 
#endif					 

typedef char		FRAME;
typedef char		* MENU;
typedef char		* FDESC;
typedef char		ABRTSPRM;
typedef char		i1;
typedef short		i2;
typedef	int		i4;
typedef float		f4;
typedef double		f8;
typedef char		bool;
typedef i4		(*FUNCPTR)();
typedef char		*PTR;
typedef unsigned char	u_char;
typedef unsigned short	u_i2;

typedef struct {
	PTR	data;
	i4	length;
	i2	type;
	i2	prec;
	i2	collID;
} DB_DATA_VALUE;

typedef struct {
	PTR	abrmenu;
	DB_DATA_VALUE	abrrettype;
} ABRTSUSER;

typedef struct {
	i4	(*abrfunc)();
	DB_DATA_VALUE	abrrettype;
	i4	abrfid;
} ABRTSNUSER;

typedef struct {
	i4		abrfid;
	DB_DATA_VALUE	abrrettype;
} ABRTSIUSER;

typedef struct {
	char	abrqfrelid[32+1];
	char	abrqfcomline[30+1];
	i4	abrqfjdef;
} ABRTSQBF;

typedef struct {
	char	abrrwname[32+1];
	char	abrrwcomline[30+1];
	char	abrrwoutput[48+1];
} ABRTSRW;

typedef struct {
	char	abrgfname[32+1];
	char	abrgfcomline[30	+1];
	char	abrgfoutput[48	+1];
} ABRTSGBF;

typedef	union abrvarstr {
	ABRTSQBF	abrfrvqbf;
	ABRTSUSER	abrfrvuser;
	ABRTSRW		abrfrvrw;
	ABRTSGBF	abrfrvgbf;
	ABRTSNUSER	abrfrvnuser;
	ABRTSIUSER	abrfrviuser;
} ABRTSVAR;

typedef struct {
	char	*abrfoname;
	PTR	abrfoaddr;
	i4	abrfopos;
	i4	abrfosource;
	i4	abrforef;
} ABRTSFO;

typedef struct {
	char		*abrfrname;
	i4		abrfrtype;
	ABRTSFO		*abrform;
	ABRTSVAR	*abrfrvar;
	i4		abrnext;
	i4		abrdml;
} ABRTSFRM;

typedef struct {
	char		*abrgname;
	char		*abrgtname;
	DB_DATA_VALUE	abrgdattype;	
	i4		abrgnext;
	i4		abrgtype;
} ABRTSGL;

typedef struct abrstktype
{
	char			*abrfsname;
	i4			abrfsusage;
	ABRTSFRM		*abrfsfrm;
	ABRTSPRM		*abrfsprm;
	FDESC			*abrfsfdesc;
	i4			abrfsdml;
	struct abrstktype	*abrfsnext;
} ABRTSSTK;

typedef struct abrproctype
{
	char	*abrpname;
	i4	abrplang;

	DB_DATA_VALUE	abrprettype;

	int	(*abrpfunc)();
	i4	abrpfid;
	i4	abrpnext;
	i4	abrpdml;
} ABRTSPRO;

typedef struct _a_attr {
	char		*abraname;
	char		*abratname;
	DB_DATA_VALUE	abradattype;
	i4		abraoff;
} ABRTSATTR;

typedef struct {
	char		*abrtname;
	ABRTSATTR	*abrtflds;
	i4		abrtcnt;
	i4		abrtsize;
	i4		abrtnext;
} ABRTSTYPE;

typedef struct {
	i4		abroversion;
	char		*abrodbname;
	char		*abrosframe;
	i2		abdml;
	bool		start_proc;
	bool		batch;
	i4		abrohcnt;
	i4		*abrofhash;
	i4		*abrophash;
	i4		abrofcnt;
	i4		abropcnt;
	i4		abrofocnt;
	ABRTSFRM	*abroftab;
	ABRTSPRO	*abroptab;
	ABRTSFO		*abrofotab;
	i4             language;
	char		*abroleid;
	char		*abpassword;
	i4		*abroghash;
	i4		*abrothash;
	i4		abroglcnt;
	i4		abrotycnt;
	ABRTSGL		*abrogltab;
	ABRTSTYPE	*abrotytab;
	char		*abcndate;
	char		*abroappl;
} ABRTSOBJ;



typedef struct {
	char *name;
	i4	(*addr)();
} ABRTSFUNCS;

/* used by DS to write out the structures */

typedef i4		i4array[];
typedef i4		natarray[];
typedef char		*string;

typedef ABRTSPRO	PROCARRAY[];
typedef ABRTSFRM	FRMARRAY[];
typedef ABRTSFUNCS	HLARRAY[];
typedef ABRTSGL		GLOBARRAY[];
typedef ABRTSTYPE	TYPEARRAY[];
typedef ABRTSATTR	ATTRARRAY[];

#define TEXT_STR(n)	struct {u_i2 db_t_count;u_char db_t_text[(n)+1];}
#define CHAR_ARRAY(n)	struct {u_char db_t_text[(n)+1];}

#endif /* ABFURTS_H_INCLUDED */
