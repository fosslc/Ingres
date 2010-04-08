/******************************************************************************
**
** Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

/*
** Name: ddfhash.h - Data Dictionary Facility. Hash Management
**
** Description:
**  This file contains the Hash API used by DDF.
**	Through this hash API, a duplicate key is forbidden
**
** History: 
**      02-mar-98 (marol01)
**          Created
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

#ifndef HASH_INCLUDED
#define HASH_INCLUDED

#include <ddfcom.h>

struct hashbucket			
{
	struct hashbucket	*previous;		
	struct hashbucket	*next;		
	char				*name;		
	PTR					object;		
};

struct hashtable			
{
	struct hashbucket	**table;		
	u_i4				size;	
};

struct hashscan			
{
	struct hashtable	*table;		
	u_i4				position;	
	struct hashbucket	*lastbucket;	
};

typedef struct hashtable	*DDFHASHTABLE;	
typedef struct hashbucket	*DDFHASHBUCKET;	
typedef struct hashscan		DDFHASHSCAN;	

#define HASH_ALL			0

#define DDF_INIT_SCAN(X, Y)	(X).table = Y; (X).position = 0; (X).lastbucket = NULL
#define DDF_SCAN(X, Y)		DDFscanhash(&(X), (PTR*) &Y)

GSTATUS
DDFmkhash(
	u_i4		size, 
	DDFHASHTABLE	*tab);

GSTATUS 
DDFrmhash(
	DDFHASHTABLE	*tab);

GSTATUS
DDFrehash(
	DDFHASHTABLE	*tab, 
	u_i4		size);

GSTATUS 
DDFgethash(
	DDFHASHTABLE	tab, 
	char		*name, 
	u_i4		length, 
	PTR			*value);

GSTATUS 
DDFputhash(
	DDFHASHTABLE	tab, 
	char		*name, 
	u_i4		length, 
	PTR			value);

GSTATUS 
DDFdelhash(
	DDFHASHTABLE	tab, 
	char		*name, 
	u_i4		length, 
	PTR			*value);

bool 
DDFscanhash(
	DDFHASHSCAN	*sc, 
	PTR *value);

#endif
