/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:    lt.h -	List Module Interface Definitions File.
**
** Description:
**	Contains declarations for list used throughout frontends.
*/

typedef struct listType	LIST;

struct listType
{
    LIST	*lt_next;
    i4		lt_tag;
    PTR		lt_rec;
};
	
# define	LNIL	((LIST *) 0)

# define	LTnext(lt)	((lt)->lt_next)
# define	LTnaddr(lt)	(&((lt)->lt_next))

LIST	*LTalloc();
PTR	LTpop(); 
VOID	LTpush(); 
PTR	LTtop();

