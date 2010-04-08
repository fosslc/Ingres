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

#define STDOUT_MAXBUFSIZ	2048

#ifndef STalloc
#define STalloc IISTalloc
FUNC_EXTERN char * STalloc(
	char	*string
);
#endif

#ifndef STbcompare
#define STbcompare IISTbcompare
FUNC_EXTERN i4 STbcompare(
	const char	*a,
	i4		a_length,
	const char	*b,
	i4		b_length,
	i4		ignrcase
);
#endif

#ifndef STgetwords
#define STgetwords IISTgetwords
FUNC_EXTERN VOID STgetwords(
	char	*string,
	i4	*count,
	char	**wordarray
);
#endif
 
#ifndef STindex
#define STindex IISTindex
FUNC_EXTERN char * STindex(
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

#ifndef STrindex
#define STrindex IISTrindex
FUNC_EXTERN char * STrindex(
	const char	*str,
	const char	*mstr,
	size_t	len
);
#endif

#ifndef STrstrindex
#define STrstrindex IISTrstrindex
FUNC_EXTERN char * STrstrindex(
	const char	*str,
	const char	*mstr,
	size_t	len,
        i4     nc
);
#endif
 
#ifndef STscompare
#define STscompare IISTscompare
FUNC_EXTERN i4 STscompare(
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

#ifndef STstrindex
#define STstrindex IISTstrindex
FUNC_EXTERN char * STstrindex(
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
 
#ifndef STtrmwhite
#define STtrmwhite IISTtrmwhite
FUNC_EXTERN size_t STtrmwhite(
	char	*string
);
#endif

#ifndef STtrmnwhite
#define STtrmnwhite IISTtrmnwhite
FUNC_EXTERN size_t STtrmnwhite(
	char	*string,
        size_t	max_len
);
#endif
 
#ifndef STzapblank
#define STzapblank IISTzapblank
FUNC_EXTERN size_t STzapblank(
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
