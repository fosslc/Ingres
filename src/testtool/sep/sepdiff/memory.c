#include <compat.h>
#include <lo.h>
#include <me.h>
#include <si.h>

#include <edit.h>
#include <token.h>
#include <sepdefs.h>

/*
**	History:
**
**	##-sep-1989 (mca)
**		Created.
**	01-dec-89 (eduardo)
**		Added capability to clean up memory held by tokens and edits
**		when needed.
**	10-dec-1989 (eduardo)
**		Added use of ME routines instead of calloc and malloc directly.
**	18-may-1990 (rog)
**		Minor cleanup.
**	11-jun-1990 (rog)
**		Cleanup external variable definitions for VMS.
**	13-jun-1990 (rog)
**		Added register declarations.
**	03-jul-1990 (rog)
**		Add a tag to the ME calls so that we can free them all
**		at once instead of one at a time.
**	19-jul-1990 (rog)
**		Change include files to use <> syntax because the headers
**		are all in testtool/sep/hdr.
**	17-feb-1991 (seiwald)
**		Performance mods: rewrote get_token() to be a micro-allocator;
**		all token data freed by single call to free_tokens().
**		Cleanup: eliminated VMSNOME code - VMS's compatlib has ME.
**	14-jan-1992 (donj)
**	    Modified all quoted string constants to use the ERx("") macro.
**	    Reformatted variable declarations to one per line for clarity.
**	    Added explicit declarations for all submodules called. Simplified
**	    and clarified some control loops.
**      10-jul-92 (donj)
**          Moved ME memory tag constants from memory.c to sepdefs.h and
**	    included sepdefs.h in this file.
**	14-jul-92 (donj)
**	    Changed TOKEN_TAG to fit the other SEP_ME_TAG_xxxx constants.
**      16-jul-1992 (donj)
**          Replaced calls to MEreqmem with either STtalloc or SEP_MEalloc.
**      04-feb-1993 (donj)
**          Included lo.h because sepdefs.h now references LOCATION type.
**      26-apr-1993 (donj)
**	    Finally found out why I was getting these "warning, trigrah sequence
**	    replaced" messages. The question marks people used for unknown
**	    dates. (Thanks DaveB).
**	 4-may-1993 (donj)
**	    NULLed out some char ptrs, Added prototyping of functions.
**	 7-may-1993 (donj)
**	    Add more prototyping.
**       7-may-1993 (donj)
**	    Take out extraneous (VOID).
**       7-may-1993 (donj)
**          Fix SEP_MEalloc().
**      15-oct-1993 (donj)
**          Make function prototyping unconditional.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**      22-sep-2000 (mcgem01)
**          More nat and longnat to i4 changes.
*/

#define TOKEN_BUNCH_SZ	200
#define EDIT_BUNCH_SZ	50
#define TOKEN_CHUNK 	8002

typedef struct memblock {
	PTR                memloc;
	struct memblock   *next;
}   MEM_BLOCK;

GLOBALDEF    FILE_TOKENS **tokarray1 = NULL ;
GLOBALDEF    FILE_TOKENS **tokarray2 = NULL ;
GLOBALDEF    E_edit       *script = NULL ;
GLOBALDEF    i4           *last_d = NULL ;

GLOBALREF    i4            failMem ;

static FILE_TOKENS        *token_free_list = NULL ;
static E_edit              edit_free_list = NULL ;
static MEM_BLOCK          *tBlockList = NULL ;
static MEM_BLOCK          *eBlockList = NULL ;

static char               *token_buf = NULL ;
static int                 token_free = 0 ;

FUNC_EXTERN  char         *SEP_MEalloc         ( u_i4 tag, u_i4 size, bool clear, STATUS *ME_status ) ;

/*
** Name: get_token() - a micro-allocator; all data freed with free_tokens()
*/

char *
get_token(i4 size)
{
    STATUS                 status ;
    char                  *ptr = NULL ;
    int                    n ;

    /* Always alloc aligned buffers */

    size = (size + (sizeof(ALIGN_RESTRICT) - 1))
		    & ~(sizeof(ALIGN_RESTRICT) - 1);

    /* If no room in current chunk, allocate another */

    if (size > token_free)
    {
	n = max(size, TOKEN_CHUNK);

	if(!(ptr = SEP_MEalloc(SEP_ME_TAG_TOKEN, n, FALSE, &status)))
	    return 0;

	token_buf = ptr;
	token_free = n;
    }

    /* Aloc space from this chunck */

    ptr = token_buf;
    token_buf += size;
    token_free -= size;

    return ptr;
}

VOID
free_tokens()
{
    /* Clear all allocated tokens */

    token_buf = 0;
    token_free = 0;
    MEtfree(SEP_ME_TAG_TOKEN);
}

VOID
size_tokarray (i4 which,i4 nelem)
{
    STATUS                 ioerr ;
    static i4              array1size = 0 ;
    static i4              array2size = 0 ;

    switch (which)
    {
	case 1:
	    if (array1size > nelem)
		return;
	    if (array1size == 0)
		tokarray1 = (FILE_TOKENS **)
			    SEP_MEalloc(SEP_ME_TAG_NODEL,
					nelem * sizeof(FILE_TOKENS *), TRUE,
					&ioerr);
	    else
	    {
		MEfree((PTR) tokarray1);
		tokarray1 = (FILE_TOKENS **)
			    SEP_MEalloc(SEP_ME_TAG_NODEL,
					nelem * sizeof(FILE_TOKENS *), TRUE,
					&ioerr);
	    }
	    array1size = nelem;
	    break;
	case 2:
	    if (array2size > nelem)
		return;
	    if (array2size == 0)
		tokarray2 = (FILE_TOKENS **)
			    SEP_MEalloc(SEP_ME_TAG_NODEL,
					nelem * sizeof(FILE_TOKENS *), TRUE,
					&ioerr);
	    else
	    {
		MEfree((PTR) tokarray2);
		tokarray2 = (FILE_TOKENS **)
			    SEP_MEalloc(SEP_ME_TAG_NODEL,
					nelem * sizeof(FILE_TOKENS *), TRUE,
					&ioerr);
	    }
	    array2size = nelem;
	    break;
    }
}

E_edit
get_edit()
{
    register i4            i;
    STATUS                 ioerr ;
    MEM_BLOCK             *etemp = NULL ;
    E_edit                 tmp_edit ;

    if (edit_free_list != NULL)
    {
	tmp_edit = edit_free_list;
	edit_free_list = edit_free_list->link;
	return (tmp_edit);
    }
    /*
    **	allocate block of edits 
    */
    edit_free_list = (E_edit)
		     SEP_MEalloc(E_BUNCH_TAG, EDIT_BUNCH_SZ * sizeof(_E_struct),
				 TRUE, &ioerr);
    if (ioerr != OK)
	return(NULL);
    /*
    **	register block just allocated
    */
    etemp = (MEM_BLOCK *) SEP_MEalloc(E_BLOCK_TAG, sizeof(MEM_BLOCK), TRUE,
				      &ioerr);
    if (ioerr != OK)
    {
	MEfree((PTR) edit_free_list);
	return(NULL);
    }
    else
    {
	etemp->memloc = (PTR) edit_free_list;
	etemp->next = eBlockList;
	eBlockList = etemp;
    }

    /*
    **	partition block allocated
    */

    for (i = EDIT_BUNCH_SZ - 1 ; i > 1 ; i--)
	edit_free_list[i-1].link = &edit_free_list[i];
    edit_free_list[EDIT_BUNCH_SZ - 1].link = NULL;
    tmp_edit = edit_free_list;
    edit_free_list = &edit_free_list[1];
    return(tmp_edit);
}

VOID
free_edits(E_edit edit_list)
{
    register E_edit        e ;

    if (edit_list == NULL)
	return;
    for (e = edit_list ; e->link != NULL ; e = e->link)
	;
    e->link = edit_free_list;
    edit_free_list = edit_list;
}

VOID
size_diag(i4 nelem)
{
    static                 diag_size = 0 ;
    STATUS                 ioerr ;

    if (diag_size > nelem)
	return;
    if (diag_size == 0)
    {
	script = (E_edit *)
		 SEP_MEalloc(SEP_ME_TAG_NODEL, nelem * sizeof(E_edit), TRUE,
			     &ioerr);
	last_d = (i4 *)
		 SEP_MEalloc(SEP_ME_TAG_NODEL, nelem * sizeof(i4), TRUE,
			     &ioerr);
    }
    else
    {
	MEfree((PTR) script);
	MEfree((PTR) last_d);
	script = (E_edit *)
		 SEP_MEalloc(SEP_ME_TAG_NODEL, nelem * sizeof(E_edit), TRUE,
			     &ioerr);
	last_d = (i4 *)
		 SEP_MEalloc(SEP_ME_TAG_NODEL, nelem * sizeof(i4), TRUE,
			     &ioerr);
    }
    diag_size = nelem;
}

VOID
clean_edit_blocks()
{
    MEtfree(E_BUNCH_TAG);
    MEtfree(E_BLOCK_TAG);
    edit_free_list = NULL;
}


VOID
clean_token_blocks()
{
    MEtfree(T_BUNCH_TAG);
    MEtfree(T_BLOCK_TAG);
    token_free_list = NULL;
}
