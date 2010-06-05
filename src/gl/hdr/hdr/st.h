/*
**	Copyright (c) 2004 Ingres Corporation
*/
# ifndef ST_HDR_INCLUDED
# define ST_HDR_INCLUDED 1

#include    <stcl.h>

/**CL_SPEC
** Name:	ST.h	- Define ST function externs
**
** Specification:
**
** Description:
**	Contains ST function externs
**
** History:
**	2-jun-1993 (ed)
**	    initial creation to define func_externs in glhdr
**	24-jan-1994 (gmanning)
**	    Enclose in ifndefs the following: STalloc, STbcompare, STcat, 
**	    STgetwords, STindex, STlcopy, STmove, STnlength, STrindex, 
**	    STscompare, STskipblank, STtalloc, STtrmwhite, STzapblank.
**	6-june-95 (hanch04)
**	    Added STtrmnwhite.
**	8-jun-95 (hanch04)
**	    change max to max_len
**      07-jan-1997 (wilan06)
**          Added declarations of new CL functions STstrindex and STrstrindex
**	31-mar-1997 (canor01)
**	    Protect against multiple inclusion of st.h.
**	08-oct-1997 (canor01)
**	    Add complete prototype for STprintf.
**  28-feb-2001 (lewhi01)
**      Add STDOUT_MAXBUFSIZ, which is the maximum number of characters,
**      excluding the string delimiter, that will be passed to STlcopy
**      for processing, when the ultimate destination of the buffer is
**      stdout.
**
**      19-jan-1998 (canor01)
**          Remove CL_DONT_SHIP_PROTOS.
**	29-sep-1998 (canor01)
**	    Correct prototype for STpolycat.
**	21-apr-1999 (hanch04)
**	    Removed old functions, use OS versions, replace i4 with size_t
**	26-apr-1999 (hanch04)
**	    Added casts to quite compiler warnings
**	21-may-1999 (hanch04)
**	    Replace STbcompare with STcasecmp,STncasecmp,STcmp,STncmp
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	14-Jun-2004 (schka24)
**	    Prototype STlpolycat.
**	20-Jul-2004 (lakvi01)
**		SIR #112703, Added decleration of STxcompare.
**      26-Jul-2004 (lakvi01)
**          Backed-out the above change to keep the open-source stable.
**          Will be revisited and submitted at a later date. 
**      24-Nov-2009 (frima01) Bug 122490
**          Added prototype for STxcompare to eliminate gcc 4.3 warnings.
[@history_template@]...
**/

/*
** Define pointer-vector array for ST functions which
** have single byte or multi byte variants
*/
struct	_ST_FUNCTIONS {
    i4 (*IISTbcompare)(
	const char	*a,
	i4		a_length,
	const char	*b,
	i4		b_length,
	i4		ignrcase);
    VOID (*IISTgetwords)(
	char	*string,
	i4	*count,
	char	**wordarray);
    char *(*IISTindex)(
	const char	*str,
	const char	*mstr,
	size_t	len);
    char *(*IISTrindex)(
	const char	*str,
	const char	*mstr,
	size_t	len);
    char *(*IISTrstrindex)(
	const char	*str,
	const char	*mstr,
	size_t	len,
        i4     nc);
    STATUS (*IISTscompare)(
        const char      *ap,
        size_t  a_len,  
        const char      *bp,
        size_t  b_len);
    char *(*IISTstrindex)(
	const char	*str,
	const char	*mstr,
	size_t	len,
        i4     nc);
    size_t (*IISTtrmnwhite)(
	char	*string,
        size_t	max_len);
    size_t (*IISTtrmwhite)(
	char	*string);
    size_t (*IISTzapblank)(
	char	*string,
	char	*buffer);
};

typedef struct  _ST_FUNCTIONS   ST_FUNCTIONS;
# if defined(NT_GENERIC) && defined(IMPORT_DLL_DATA)
GLOBALDLLREF       ST_FUNCTIONS    ST_fvp;
# else
GLOBALREF       ST_FUNCTIONS    ST_fvp;
#endif


#define STbcompare	ST_fvp.IISTbcompare
#define STgetwords	ST_fvp.IISTgetwords
#define STindex		ST_fvp.IISTindex
#define STrindex	ST_fvp.IISTrindex
#define STrstrindex	ST_fvp.IISTrstrindex

#define STscompare	ST_fvp.IISTscompare
#define STstrindex	ST_fvp.IISTstrindex
#define STtrmnwhite	ST_fvp.IISTtrmnwhite
#define STtrmwhite	ST_fvp.IISTtrmwhite
#define STzapblank	ST_fvp.IISTzapblank

#define STDOUT_MAXBUFSIZ	2048


#ifndef STinit
#define STinit IISTinit
FUNC_EXTERN void STinit(
);
#endif


#ifndef STalloc
#define STalloc IISTalloc
FUNC_EXTERN char * STalloc(
	char	*string
);
#endif

#ifndef STbcompare_DB
#define STbcompare_DB IISTbcompare_DB
FUNC_EXTERN i4 STbcompare_DB(
	const char	*a,
	i4		a_length,
	const char	*b,
	i4		b_length,
	i4		ignrcase
);
#endif

#ifndef STbcompare_SB
#define STbcompare_SB IISTbcompare_SB
FUNC_EXTERN i4 STbcompare_SB(
	const char	*a,
	i4		a_length,
	const char	*b,
	i4		b_length,
	i4		ignrcase
);
#endif

#ifndef STgetwords_DB
#define STgetwords_DB IISTgetwords_DB
FUNC_EXTERN VOID STgetwords_DB(
	char	*string,
	i4	*count,
	char	**wordarray
);
#endif
 
#ifndef STgetwords_SB
#define STgetwords_SB IISTgetwords_SB
FUNC_EXTERN VOID STgetwords_SB(
	char	*string,
	i4	*count,
	char	**wordarray
);
#endif

#ifndef STindex_DB
#define STindex_DB IISTindex_DB
FUNC_EXTERN char * STindex_DB(
	const char	*str,
	const char	*mstr,
	size_t	len
);
#endif
 
#ifndef STindex_SB
#define STindex_SB IISTindex_SB
FUNC_EXTERN char * STindex_SB(
	const char	*str,
	const char	*mstr,
	size_t	len
);
#endif

#ifndef STlcopy
FUNC_EXTERN size_t STlcopy(
        const char    *source,
        char    *dest,
        size_t  l
);
#endif

#ifndef STlength
#define STlength IISTlength
FUNC_EXTERN size_t STlength(
        const char    *string
);
#endif

#ifndef STmove
#define STmove IISTmove
FUNC_EXTERN VOID STmove (
	const char    *source,
	char    padchar,
	size_t     dlen,
	char    *dest
);
#endif

#ifndef STnlength
#define STnlength IISTnlength
FUNC_EXTERN size_t STnlength(
        size_t     count,
        const char    *string
);
#endif

#ifndef STpolycat
#define STpolycat IISTpolycat
FUNC_EXTERN char * STpolycat(
        i4      n,
        char    *a,
        char    *b,
        ...
);
#endif

#ifndef STlpolycat
#define STlpolycat IISTlpolycat
FUNC_EXTERN char * STlpolycat(
        i4      n,
	i4	len,
        char    *a,
        char    *b,
        ...
);
#endif

#ifndef STprintf
#define STprintf IISTprintf
FUNC_EXTERN char * STprintf(
	char	*str,
	const char	*fmt,
	...
);
#endif


#ifndef STrindex_DB
#define STrindex_DB IISTrindex_DB
FUNC_EXTERN char * STrindex_DB(
	const char	*str,
	const char	*mstr,
	size_t	len
);
#endif

#ifndef STrindex_SB
#define STrindex_SB IISTrindex_SB
FUNC_EXTERN char * STrindex_SB(
	const char	*str,
	const char	*mstr,
	size_t	len
);
#endif


#ifndef STrstrindex_DB
#define STrstrindex_DB IISTrstrindex_DB
FUNC_EXTERN char * STrstrindex_DB(
	const char	*str,
	const char	*mstr,
	size_t	len,
        i4     nc
);
#endif
 
#ifndef STrstrindex_SB
#define STrstrindex_SB IISTrstrindex_SB
FUNC_EXTERN char * STrstrindex_SB(
	const char	*str,
	const char	*mstr,
	size_t	len,
        i4     nc
);
#endif

 
#ifndef STscompare_SB
#define STscompare_SB IISTscompare_SB
FUNC_EXTERN i4 STscompare_SB(
	const char	*pa_ptr,
	size_t	a_len,
	const char	*pb_ptr,
	size_t	b_len
);
#endif
 
#ifndef STscompare_DB
#define STscompare_DB IISTscompare_DB
FUNC_EXTERN i4 STscompare_DB(
	const char	*pa_ptr,
	size_t	a_len,
	const char	*pb_ptr,
	size_t	b_len
);
#endif
 
#ifndef STskipblank
#define	STskipblank IISTskipblank
FUNC_EXTERN char	* STskipblank (
	char	*string,
	size_t	len
);
#endif


#ifndef STstrindex_DB
#define STstrindex_DB IISTstrindex_DB
FUNC_EXTERN char * STstrindex_DB(
	const char	*str,
	const char	*mstr,
	size_t	len,
        i4     nc
);
#endif

#ifndef STstrindex_SB
#define STstrindex_SB IISTstrindex_SB
FUNC_EXTERN char * STstrindex_SB(
	const char	*str,
	const char	*mstr,
	size_t	len,
        i4     nc
);
#endif


#ifndef STtalloc
#define	STtalloc IISTtalloc
FUNC_EXTERN char	* STtalloc(
	u_i2	tag,
	const char	*string
);
#endif

 
#ifndef STtrmwhite_DB
#define STtrmwhite_DB IISTtrmwhite_DB
FUNC_EXTERN size_t STtrmwhite_DB(
	char	*string
);
#endif

#ifndef STtrmwhite_SB
#define STtrmwhite_SB IISTtrmwhite_SB
FUNC_EXTERN size_t STtrmwhite_SB(
	char	*string
);
#endif


#ifndef STtrmnwhite_DB
#define STtrmnwhite_DB IISTtrmnwhite_DB
FUNC_EXTERN size_t STtrmnwhite_DB(
	char	*string,
        size_t	max_len
);
#endif

#ifndef STtrmnwhite_SB
#define STtrmnwhite_SB IISTtrmnwhite_SB
FUNC_EXTERN size_t STtrmnwhite_SB(
	char	*string,
        size_t	max_len
);
#endif

 
#ifndef STzapblank_DB
#define STzapblank_DB IISTzapblank_DB
FUNC_EXTERN size_t STzapblank_DB(
	char	*string,
	char	*buffer
);
#endif
#ifndef STzapblank_SB
#define STzapblank_SB IISTzapblank_SB
FUNC_EXTERN size_t STzapblank_SB(
	char	*string,
	char	*buffer
);
#endif

#ifndef STxcompare
#define STxcompare IISTxcompare
FUNC_EXTERN i4 STxcompare(
        char    *a_ptr,
        size_t  a_len,
        char    *b_ptr,
        size_t  b_len,
        bool    ic,
        bool    sb
);
#endif


# endif /* ! ST_HDR_INCLUDED */

