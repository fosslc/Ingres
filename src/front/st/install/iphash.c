/*
** Copyright (c) 2004 Ingres Corporation
*/
 
# include <compat.h>
# include <me.h>
# include <st.h>
# include <iphash.h>
/*
** Name: iphash.c
**
** Description:
**		Implement hash index for fast search for individual
**		names.
**
**		For looking for packages within an installation
**		or parts within a package, or even files within a part
**		a sequential scan is probably good enough.
**		This is not so for a search for a series of 
**		individual files within an installation - number
**		of files is approx 1400
**
**
** Externally visible functions:
**	ip_hash_file_get-	wrapper for ip_hash_get
**	ip_hash_file_put-	wrapper for ip_hash_put
**	ip_hash_init	-	initialise control block for hash index
** Internal functions:
**	ip_hash			- 	generate hash value
**	ip_hash_get		-	get
**	ip_hash_new		-	allocate new IP_OBID structure
**	ip_hash_put		-	add item to index
*/
GLOBALDEF	IP_SCB	*ip_scb;

FUNC_EXTERN PTR ip_alloc_blk( i4 );

/*
** static declarations
*/
static IP_HBUCKET * ip_hash( char *, i4  ) ;
static PTR ip_hash_get(char *name);
static IP_OBID * ip_hash_new(char *, PTR );
static VOID ip_hash_put( char *, PTR );
/*
** externally visible functions
*/
/*
** Name: ip_hash_file_get
**
** Description
** Wrapper for ip_hash_get for dir/file combination
** Input:
**			dir		- directory name for file
**			name 	- file name
** Output:
**		VOID
** History:
**		18-march-1996 (angusm)
**		initial creation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Aug-2009 (kschendel) 121804
**	    Define ip-alloc-blk properly.
*/
PTR ip_hash_file_get(char *dir, char *name)
{
	char buf[IP_NAMELEN];
	(VOID)MEfill((u_i2)sizeof(buf), 0 , buf);
	(VOID)STcat(buf, dir);
	(VOID)STcat(buf, name);

	return ( ip_hash_get(buf) );
}
/*
** Name: ip_hash_file_put
**
** Description
** Wrapper for ip_hash_put for dir/file combination
** Input:
**			dir		- directory name for file
**			name 	- file name
**			ele		- pointer to FILBLK cast as PTR
** Output:
**		VOID
** History:
**		18-march-1996 (angusm)
**		initial creation
*/
VOID ip_hash_file_put(char *dir, char *name, PTR ele)
{
	char buf[IP_NAMELEN];
	(VOID)MEfill((u_i2)sizeof(buf), 0 , buf);
	(VOID)STcat(buf, dir);
	(VOID)STcat(buf, name);

	ip_hash_put(buf, ele);
}
/*
** Name: ip_hash_init
**
** Description:
**		Initialise control block for hash index
**
** Input:
**			VOID
**		
** Output:
**			VOID
**
** History:
**		18-march-1996 (angusm)
**		initial creation
*/
VOID
ip_hash_init()
{
	ip_scb = (IP_SCB *)ip_alloc_blk(sizeof(IP_SCB));
	MEfill((u_i2)sizeof(IP_SCB), 0, (PTR)ip_scb);
	ip_scb->ip_nbuckets = IP_NBUCKETS;

}


/*
** internal functions
*/


static IP_HBUCKET *
ip_hash( char *name, i4  length ) 
{
    u_i4		idx;
    u_i4		hashnum;
    u_i4		shiftnum;

    for ( hashnum = 0 ; length > 0 && *name != ' ' ; length-- )
    {
	hashnum = (hashnum << 4) + *name;
	shiftnum = hashnum & 0xf0000000;
	if (shiftnum != 0)
	{
	    hashnum = hashnum ^ (shiftnum >> 24);
	    hashnum = hashnum ^ shiftnum;
	}
	name++;
    }

    idx = (u_i4) (hashnum % ip_scb->ip_nbuckets);


    return (ip_scb->ip_hashtbl + idx);
}
/*
** Name: ip_hash_get
**
** Description:
**		For given name, find corresponding IP_OBID
**		structure(s).
**
** Input:
**			name	-	name to look up
**		
** Output:
**			PTR 	  - if found
**			NULL	  - else
**
** History:
**		18-march-1996 (angusm)
**		initial creation
*/
PTR
ip_hash_get(char *name)
{
	IP_HBUCKET	*h;
	IP_OBID		*tmp;
	i4 len = STlength(name);

	h = ip_hash(name, len);

	for (tmp=h->head; tmp->next; tmp=tmp->next)
	{
		if (tmp->lname != len)
			continue;
		if STequal(tmp->name, name)
			return (tmp->ele);
	}
	return NULL;	
}

static
IP_OBID *
ip_hash_new(char *name, PTR ele)
{
	IP_OBID 	*new;

	new = (IP_OBID *)ip_alloc_blk( sizeof(IP_OBID) );

	new->lname = STlength(name);
	STcopy(name, new->name);
	new->ele = ele;
	new->next = NULL;
	return new;
}

/*
** Name: ip_hash_put
**
** Description:
**		Given combination of name and pointer
**		(pointer may point to IP structure or NULL)
**		add new IP_OBID structure to hash bucket.	
**
** Input:
**			name	-	name to insert
**			ele		-	PTR to IP struct containing name
**		
** Output:
**			VOID
**
** History:
**		18-march-1996 (angusm)
**		initial creation
*/
VOID
ip_hash_put( char *name, PTR ele )
{
	IP_OBID 	*new;
	IP_OBID		*tmp;
	IP_HBUCKET	*h;

	new = ip_hash_new(name, ele);

	h = ip_hash(name, new->lname);

	if (h->head == NULL)
		h->head = new;
	else
	{
		for (tmp = h->head; tmp->next; tmp=tmp->next)
			;
		tmp->next = new;
	}
	h->count++;
}
