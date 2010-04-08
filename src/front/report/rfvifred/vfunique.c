/*
**  vfunique.c - Process unique names for vifred.
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	03/13/87 (dkh) - Added support for ADTs.
**	mm/dd/yy (RTI) -- created for 5.0.
**	10/09/86 (KY)  -- modified to be able to create a positive value
**			    for a hash key on Double Bytes chracters.
**	13-jul-87 (bab)	Changed memory allocation to use [FM]Ereqmem.
**	08/14/87 (dkh) - ER changes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Feb-2010 (frima01) Bug 122490
**	    Update return types and add header files as neccessary
**	    to eliminate gcc 4.3 warnings.
*/

# include	<compat.h>
# include	<cv.h>		/* 6-x_PC_80x86 */
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	"vfunique.h"
# include	<me.h>
# include	<ug.h>
# include	<er.h>

# define	reg		register

static	struct	VFSYMTBL	*field_st[HASH_TBL_SIZE] = {0};
static	struct	VFSYMTBL	*tblfd_st[HASH_TBL_SIZE] = {0};



i4
vfhash(name)
char	*name;
{
/*
** Change "char *cp" to "u_char *cp" 
** to handle chracters which MSB(MostSignificantBit) is ON.
** Then force hash_key to positive.
*/
	reg	u_char	*cp;
	reg	i4	i = 0;
	reg	i4	j = 0;
	u_char	buf[FE_MAXNAME + 1];

	STcopy(name, buf);
	CVlower(buf);
	for (cp = buf; *cp != '\0'; cp++, i++)
	{
		j += *cp;
	}
	i = j % HASH_TBL_SIZE;
	return(i);
}


i4
vfnmchk(hashval, name, symtbl)
i4	hashval;
reg	char	*name;
struct	VFSYMTBL	**symtbl;
{
	reg	i4	status = NOT_FOUND;
	reg	struct	VFSYMTBL	*ptr;
	reg	i4	length;

	length = STlength(name);
	for (ptr = symtbl[hashval]; ptr != NULL; ptr = ptr->next)
	{
		if (ptr->length == length)
		{
			if (STbcompare(ptr->name, ptr->length, name, length, TRUE) == 0)
			{
				status = FOUND;
				break;
			}
		}
	}
	return(status);
}

void
vfnmadd(hashval, name, symtbl)
i4	hashval;
char	*name;
struct	VFSYMTBL	**symtbl;
{
	reg	i4	length;
	reg	struct	VFSYMTBL	*ptr;
		struct	VFSYMTBL	*newptr;

	length = STlength(name);
	ptr = symtbl[hashval];
	if ((newptr = (struct VFSYMTBL *)MEreqmem((u_i4)0,
	    (u_i4)sizeof(struct VFSYMTBL), FALSE, (STATUS *)NULL)) == NULL)
	{
		IIUGbmaBadMemoryAllocation(ERx("vfnmadd"));
	}
	newptr->length = length;
	STcopy(name, newptr->name);
	symtbl[hashval] = newptr;
	newptr->next = ptr;
}


void
vfnmdel(hashval, name, symtbl)
i4	hashval;
char	*name;
struct	VFSYMTBL	**symtbl;
{
	reg	i4	length;
	struct	VFSYMTBL	*ptr;
	struct	VFSYMTBL	*prevptr;

	ptr = symtbl[hashval];
	prevptr = NULL;
	length = STlength(name);
	for (; ptr != NULL; ptr = ptr->next)
	{
		if (ptr->length == length)
		{
			if (STbcompare(ptr->name, ptr->length, name, length, TRUE) == 0)
			{
				if (prevptr == NULL)
				{
					symtbl[hashval] = ptr->next;
				}
				else
				{
					prevptr->next = ptr->next;
				}
				MEfree(ptr);
				break;
			}
		}
		prevptr = ptr;
	}
}


void
vfdelnm(name, type)
char	*name;
i4	type;
{
	reg	i4	hashval;
	reg	struct	VFSYMTBL	**symtbl;

	if (!vfuchkon)
	{
		return;
	}
	hashval = vfhash(name);
	if (type ==  AFIELD)
	{
		symtbl = field_st;
	}
	else
	{
		symtbl = tblfd_st;
	}
	vfnmdel(hashval, name, symtbl);
}


i4
vfnmunique(name, add, type)
char	*name;
bool	add;
i4	type;
{
	reg	i4	i;
	reg	struct	VFSYMTBL	**symtbl;

	if (!vfuchkon)
	{
		return(NOT_FOUND);
	}
	if (type == AFIELD)
	{
		symtbl = field_st;
	}
	else
	{
		symtbl = tblfd_st;
	}
	i = vfhash(name);
	if (vfnmchk(i, name, symtbl) == FOUND)
	{
		return(FOUND);
	}
	if (add)
	{
		vfnmadd(i, name, symtbl);
	}
	return(NOT_FOUND);
}

VOID
vfstdel(type)
i4	type;
{
	reg	struct	VFSYMTBL	**symtbl;
	reg	struct	VFSYMTBL	*ptr;
	reg	struct	VFSYMTBL	*optr;
	reg	i4	i;

	if (type == AFIELD)
	{
		symtbl = field_st;
	}
	else
	{
		symtbl = tblfd_st;
	}
	for (i = 0; i < HASH_TBL_SIZE; i++)
	{
		for (ptr = symtbl[i]; ptr != NULL; )
		{
			optr = ptr;
			ptr = ptr->next;
			MEfree(optr);
		}
		symtbl[i] = NULL;
	}
}
