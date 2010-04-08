/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: gcu.h
**
** Description:
**	Definitions for GCF utility functions.
**
** History:
**	26-Feb-97 (gordy)
**	    Extracted functions from GCN into GCU for general usage.
**	20-Aug-97 (gordy)
**	    Added gcu_msglog() function.
**	24-Feb-00 (gordy)
**	    Added gcu_lookup() and GCULIST for tracing support.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	15-feb-2001 (somsa01)
**	    Session types are now SIZE_TYPE.
**	27-aug-2001 (somsa01)
**	    Added prototype for gcu_alloc_str().
**	26-Dec-02 (gordy)
**	    Character set file definition parsing move to gcu: 
**	    added gcu_read_cset().
*/

#ifndef _GCUINT_INCLUDED_
#define _GCUINT_INCLUDED_

typedef struct
{
    i4		id;
    char	*name;
} GCULIST;

FUNC_EXTERN char	*gcu_lookup( GCULIST *, i4 );
FUNC_EXTERN VOID	gcu_hexdump( char *, i4 );

FUNC_EXTERN i4		gcu_alloc_str( char *,  char ** );
FUNC_EXTERN i4		gcu_get_str( char *, char ** );
FUNC_EXTERN i4		gcu_put_str( char *, char * );
FUNC_EXTERN i4		gcu_get_int( char *, i4  * );
FUNC_EXTERN i4		gcu_put_int( char *, i4  );
FUNC_EXTERN i4		gcu_words( char *, char *, char **, i4, i4  );

FUNC_EXTERN VOID	gcu_get_tracesym( char *, char *, char ** );

FUNC_EXTERN VOID	gcu_erinit( char *, char *, char * );
FUNC_EXTERN VOID	gcu_erlog( SIZE_TYPE, i4, STATUS, CL_ERR_DESC *, i4, PTR );
FUNC_EXTERN VOID	gcu_msglog( SIZE_TYPE, char * );
FUNC_EXTERN VOID	gcu_erfmt( char **, char *, i4, 
				   STATUS, CL_ERR_DESC *, i4, PTR );

FUNC_EXTERN STATUS	gcu_encode( char *, char *, char * );

FUNC_EXTERN i4		gcu_read_cset( bool (*)( char *, u_i4 ) );

#endif /* _GCUINT_INCLUDED */

