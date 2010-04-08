/*
**  tuunique.c
**
**  Copyright (c) 2004 Ingres Corporation
**
**  History:
**	13-jul-87 (bab) Changed memory allocation to use [FM]Ereqmem.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

# include	<compat.h>
# include	<st.h>		/* 6-x_PC_80x86 */
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<me.h>
# include	<er.h>
# include	<ug.h>
# include	"ertu.h"
# include	"tuconst.h"

# define	reg		register

static	struct	TUSYMTBL	*tucol_names[HASH_TBL_SIZE] = {0};

i4
tuhash(name)
char	*name;
{
	/*
	** KJ 09/17/86
	** Change "char *cp" to "u_char *cp"
	** to handle chracters which MSB is ON.
	** Then force hash_key to positive.
	**
	** The change makes the hash function consider
	** the 8th bit, which make a difference for Kanji characters.
	** (added by daver, 14-apr-87)
	**
	** changed tuhash to not call CVlower on its string argument.
	** assumption is now that called of tunmunique passes in case
	** sensitive strings(changed for Gateways and Titan project).
	*/
	reg	u_char	*cp;
	reg	i4	i = 0;
	reg	i4	j = 0;
	char	buf[40];

	STcopy(name, buf);
/*	CVlower(buf);	Titan: case may be significant */
	for (cp = (u_char *)buf; *cp != '\0'; cp++, i++)
	{
		j += *cp;
	}
	i = j % HASH_TBL_SIZE;
	return(i);
}


i4
tunmchk(hashval, name)
i4	hashval;
reg	char	*name;
{
	reg	struct	TUSYMTBL	**symtbl = tucol_names;
	reg	struct	TUSYMTBL	*ptr;
	reg	i4	status = NM_NOT_FOUND;
	reg	i4	length;

	length = STlength(name);
	for (ptr = symtbl[hashval]; ptr != NULL; ptr = ptr->next)
	{
		if (ptr->length == length)
		{
			if (STbcompare(ptr->name, ptr->length, name, length, TRUE) == 0)
			{
				status = NM_FOUND;
				break;
			}
		}
	}
	return(status);
}


void
tunmadd(hashval, name)
i4	hashval;
char	*name;
{
	reg	struct	TUSYMTBL	**symtbl = tucol_names;
	reg	struct	TUSYMTBL	*ptr;
	struct		TUSYMTBL	*newptr;  /* (ab) lint removed reg */
	reg	i4	length;

	length = STlength(name);
	ptr = symtbl[hashval];
	if ((newptr = (struct TUSYMTBL *)MEreqmem((u_i4)0,
	    (u_i4)sizeof(struct TUSYMTBL), FALSE, (STATUS *)NULL)) == NULL)
	{
		IIUGerr(E_TU0033_tunmadd_bad_mem_alloc, UG_ERR_FATAL,
			(i4) 0);
	}
	newptr->length = length;
	STcopy(name, newptr->name);
	symtbl[hashval] = newptr;
	newptr->next = ptr;
}



i4
tunmunique(name)
char	*name;
{
	reg	i4	i;
	i4		tuhash(), tunmchk();
	void		tunmadd();

	i = tuhash(name);
	if (tunmchk(i, name) == NM_FOUND)
	{
		return(NM_FOUND);
	}
	tunmadd(i, name);
	return(NM_NOT_FOUND);
}



void
tustdel()
{
	reg	struct	TUSYMTBL	**symtbl = tucol_names;
	reg	struct	TUSYMTBL	*ptr;
	reg	struct	TUSYMTBL	*optr;
	reg	i4	i;

	for (i = 0; i < HASH_TBL_SIZE; i++)
	{
		for (ptr = symtbl[i]; ptr != NULL; )
		{
			optr = ptr;
			ptr = ptr->next;
			MEfree((char *)optr);
		}
		symtbl[i] = NULL;
	}
}
