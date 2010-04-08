/*
** Copyright (c) 2004 Ingres Corporation
*/

# include <compat.h>
# include <gl.h>
# include <iicommon.h>
# include <si.h>
# include <fe.h>
# include <ftdefs.h>
# include <er.h>
# include <termdr.h>
# include <tdkeys.h>
# include <kst.h>

/*
** Name:	termdrdata.c
**
** Description:	Global data for termdr facility.
**
** History:
**
**	24-sep-96 (mcgem01)
**	    Created.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPFRAMELIBDATA
**	    in the Jamfile.
*/

/* Jam hints
**
LIBRARY = IMPFRAMELIBDATA
**
*/

/* box.c */

GLOBALDEF i4    tdprform = 0;

/* crput.c */

GLOBALDEF i4    outcol = 0;
GLOBALDEF i4    outline = 0;
GLOBALDEF i4    destcol = 0;
GLOBALDEF i4    destline = 0;
GLOBALDEF i4    plodcnt = 0;
GLOBALDEF bool  plodflg = 0;

/* crtty.c */

GLOBALDEF bool  TDisopen = FALSE;
GLOBALDEF i4    IITDAllocSize = 0;
GLOBALDEF bool  IITDiswide = FALSE;
GLOBALDEF bool  IITDccsCanChgSize = FALSE;
GLOBALDEF i4    IITDcur_size = 0;
GLOBALDEF i4    ospeed = -1;

/* dispattr.c */

GLOBALDEF char  dispattr  = '\0';
GLOBALDEF char  fontattr  = '\0';
GLOBALDEF char  colorattr = '\0';

/* initscr.c */

GLOBALDEF bool  vt100 = 0;

/* itgetent.c */

GLOBALDEF       char    *ZZA = NULL;
GLOBALDEF       char    *ZZB = NULL;
GLOBALDEF       i4      ZZC = 0;

/* itin.c */

GLOBALDEF       i4      ITdebug = FALSE;

/* keys.c */

GLOBALDEF u_char	*KEYSEQ[KEYTBLSZ + 4] = {0};

#ifndef CMS
# define        CAST_PUNSCHAR   (u_char *)
#else /* CMS */
# define        CAST_PUNSCHAR
#endif /* CMS */
GLOBALDEF u_char	*KEYNULLDEF = CAST_PUNSCHAR ERx("");

GLOBALDEF i4	MAXFKEYS = 0;
GLOBALDEF i4	FKEYS = 0;
# ifndef EBCDIC
GLOBALDEF i4	MENUCHAR = '\033';
# else /* EBCDIC */
GLOBALDEF i4	MENUCHAR = 0x27;
# endif /* EBCDIC */
GLOBALDEF char	FKerfnm[20] = ERx("ingkey.err");

GLOBALDEF u_char	*KEYDEFS[KEYTBLSZ] = {0};


GLOBALDEF char	KEYUSE[KEYTBLSZ] =
	{
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

GLOBALDEF char	INBUF[KBUFSIZE] = ERx("");
GLOBALDEF char	*MACBUF = ERx("");
GLOBALDEF char	*MACBUFEND = ERx("");
GLOBALDEF u_char *mp = CAST_PUNSCHAR ERx("");
GLOBALDEF i4	kbufp = 0;
GLOBALDEF KEYSTRUCT	*(*in_func)() = FKgetc;
GLOBALDEF i4	F3INUSE = 0;
GLOBALDEF i4	parsing = INTERACTIVE;
GLOBALDEF i4	keyerrcnt = 0;
GLOBALDEF FILE	*erfile = 0;
GLOBALDEF i4	beingdef = 0;
GLOBALDEF bool	infde_getch = FALSE;
GLOBALDEF bool	no_errfile = FALSE;

/* locator.c */

GLOBALDEF i4    IITDdpDebugPrint = FALSE;

/* nomacro.c */

GLOBALDEF       bool    macexp[KEYTBLSZ] =
{
        TRUE,   TRUE,   TRUE,   TRUE,   TRUE,   TRUE,   TRUE,   TRUE,
        TRUE,   TRUE,   TRUE,   TRUE,   TRUE,   TRUE,   TRUE,   TRUE,
        TRUE,   TRUE,   TRUE,   TRUE,   TRUE,   TRUE,   TRUE,   TRUE,
        TRUE,   TRUE,   TRUE,   TRUE,   TRUE,   TRUE,   TRUE,   TRUE,
        TRUE,   TRUE,   TRUE,   TRUE,   TRUE,   TRUE,   TRUE,   TRUE
};

/* refresh.c */

GLOBALDEF i4    ly = 0;
GLOBALDEF i4    lx = 0;
GLOBALDEF bool  _curwin = 0;
GLOBALDEF u_char        pfonts = 0;
GLOBALDEF u_char        pcolor = 0;
GLOBALDEF u_char        pda = 0;
GLOBALDEF char          IITDcda_prev = EOS;
GLOBALDEF char          *IITDptr_prev = NULL;
GLOBALDEF WINDOW        *_win = NULL;
GLOBALDEF WINDOW        *_pwin = NULL; 
GLOBALDEF i2    depth = 0;
GLOBALDEF bool  IITDfcFromClear = FALSE;
# if defined(MSDOS) || defined(NT_GENERIC)
GLOBALDEF  i4   IIcurscr_firstch[200];  /* 1st char changed on nth line */
# endif /* MSDOS || NT_GENERIC */

/* termcap.c */

GLOBALDEF char  *IItbuf = ERx("");
GLOBALDEF i4    IIhopcount = 0; 

/* tgoto.c */

GLOBALDEF       i4      IITDgotolen = 0;

/* windex.c */

GLOBALDEF       i4      windex_flag = 0;
GLOBALDEF       i4      IITDwvWVVersion = 0;
