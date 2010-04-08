/*
** Copyright (c) 2004 Ingres Corporation
*/

/*
** Name:  iplist.h    Definitions for linked-list handling functions
**
** Description:
**      See iplist.c for a detailed explanation of this particular
**      implementation of linked lists.
**
** History:
**	17-jul-1992 (jonb)
**		Created
**	22-mar-93 (jonb)
**		Added prototypes for list-handling functions.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*
**  A single element in a list.  The car is the pointer to the actual
**  data item.  The cdr is a pointer to the next element in the list,
**  or is NULL if this is the last element.  The cookie is just a magic 
**  cookie that identifies this structure as what it is.
*/

typedef struct _LISTELE
{
    i4 cookie;
    PTR car;
    struct _LISTELE *cdr;
} LISTELE;

/* 
**  The actual value of the magic cookie.
*/

# define COOKIE 11223344


/*
**  Macros to loop through all the elements of a list.
*/

# define SCAN_LIST(list,ele) \
    for ( ip_list_rewind(&list); NULL != ip_list_scan(&list, (PTR *)(&ele)); )

# define SCAN_LISTP(listp,ele) \
    for ( ip_list_rewind(listp); NULL != ip_list_scan(listp, (PTR *)(&ele)); )


/*
**  A list header.  The head and tail fields point to, respectively, the
**  first and last LISTELE in the list, or they're both NULL in the case
**  of an empty list.  The lastp field points to the cdr field of the list
**  element that points to the "current" one, which is pointed at by the
**  curr field.
*/

typedef struct _LIST
{
    LISTELE *head;
    LISTELE *tail;
    LISTELE **lastp;
    LISTELE *curr;
} LIST;

/*
**  Function declarations...
*/

FUNC_EXTERN VOID        ip_list_append( LIST *ls, PTR newcar ); 
FUNC_EXTERN VOID	ip_list_assign_new(char **sptr, char *str);
FUNC_EXTERN VOID	ip_list_assign_string(char **sptr, char *str);
FUNC_EXTERN PTR         ip_list_curr( LIST *ls ); 
FUNC_EXTERN VOID        ip_list_destroy( LIST *ls ); 
FUNC_EXTERN VOID        ip_list_init( LIST *ls ); 
FUNC_EXTERN VOID        ip_list_insert_after( LIST *ls, PTR newcar ); 
FUNC_EXTERN VOID        ip_list_insert_before( LIST *ls, PTR newcar ); 
FUNC_EXTERN bool        ip_list_is_empty( LIST *ls ); 
FUNC_EXTERN VOID	ip_list_read_rewind(LIST *ls, PTR *currp);
FUNC_EXTERN LISTELE *	ip_list_read(LIST *ls, PTR *carp, PTR *currp);
FUNC_EXTERN VOID        ip_list_remove( LIST *ls ); 
FUNC_EXTERN VOID        ip_list_rewind( LIST *ls ); 
FUNC_EXTERN LISTELE *   ip_list_scan( LIST *ls, PTR *carp ); 
FUNC_EXTERN VOID        ip_list_unlink( LIST *ls ); 
FUNC_EXTERN PTR		ip_alloc_blk(i4 size);
