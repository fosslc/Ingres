/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name: play_NewOrder.h
**
** Description:
**      Defines the types used for the extension server functions
**
** History:
**	18-May-1999 (matbe01)
**	    Undefine TRUE and FALSE to remove compile errors for rs4_us5.
**	28-Mar-2001 (bonro01)
**	    Set TRUE and FALSE to correct values.  TRUE=1, FALSE=0
*/

# include <stdio.h>
# include <stdlib.h>

# ifdef _H_TYPES
# undef TRUE
# undef FALSE
# endif

# define ICE_EXT_API 
# define TRUE  1
# define FALSE 0

/*
** Define the systems multi threaded safe memory
** allocater and return type
*/
# define mt_safe_malloc(size) malloc(size)
# define mt_safe_free(size)   free(size)
# define MEMORY_P char*

typedef char*       ICE_STATUS;
typedef char         BOOL;
typedef ICE_STATUS  (*PFNEXTENSION) (char**, BOOL*, char **);

typedef struct ice_function_table
{
    char*           pszName;
    char**          pszParams;
}SERVER_DLL_FUNCTION, *PSERVER_DLL_FUNCTION;

typedef ICE_STATUS  (*PFNINITIALIZE)(PSERVER_DLL_FUNCTION*);
