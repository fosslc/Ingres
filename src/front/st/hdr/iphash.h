/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
** Name: iphash.h
**
** Description:
**		Implement hash index for fast search for individual
**		files.
** History:
**	18-mar-1996 (angusm)
**		Initial creation
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
# define IP_NBUCKETS 257
# define IP_NAMELEN  150
/*
** Name: IP_OBID
**
** Description:
**		Generic IP object descriptor. Intended for one-way link-list
**		in hash index. 
** History:
**	18-mar-1996 (angusm)
**		Initial creation
**
*/
typedef struct _ip_obid {
	struct _ip_obid	*next;
	i4			lname;
	char		name[IP_NAMELEN];
	PTR			ele;
} IP_OBID;
/*
** Name: IP_HBUCKET
**
** Description:
**		generic hash bucket
** History:
**	18-mar-1996 (angusm)
**		Initial creation
*/
typedef struct _ip_hbucket {
	IP_OBID		*head;
	i4			count;
} IP_HBUCKET;

/*
** Name: IP_SCB 
**
** Description:
**		IP hash index control block
** History:
**	18-mar-1996 (angusm)
**		Initial creation
*/
typedef struct _ip_scb {
    i4  		ip_nbuckets;
	IP_HBUCKET	ip_hashtbl[IP_NBUCKETS];
} IP_SCB;

/*************************************************************/

VOID		ip_hash_init(VOID);
PTR 		ip_hash_file_get(char *dir, char *name);
VOID		ip_hash_file_put(char *, char *, PTR);
