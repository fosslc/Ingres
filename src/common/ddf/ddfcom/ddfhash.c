/******************************************************************************
**
** Copyright (c) 2004 Ingres Corporation
**
******************************************************************************/

/*
NO_OPTIM = rs4_us5 i64_aix
*/

#include <ddfhash.h>
#include <erddf.h>

/**
**
**  Name: ddfhash.c - Data Dictionary Services Facility Hash Table Management
**
**  Description:
**	 That file provide a hash table management API. Specificity: text key need 
**	 not to finish by a NULL character and two element cannot have the same 
**   name.
**
**  History:    
**	5-Feb-98 (marol01)
**	    created
**      06-Aug-1999 (fanra01)
**          Change nat to i4, u_nat to u_i4, longnat to i4, u_longnat to u_i4
**	31-Jul-00 (ahaal01)
**	    Added NO_OPTIM for AIX (rs4_us5) to enable icesvr to be started.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	16-aug-2001 (toumi01)
**	    speculative i64_aix NO_OPTIM change for beta xlc_r - FIXME !!!
**/


/*
** Name: DDFmkhash()	- Create and initialize an hash table
**
** Description:
**
** Inputs:
**	u_nat		: table size
**	DDFHASHTABLE*	: table pointer
**
** Outputs:
**  DDFHASHTABLE*	: initialized table
**
** Returns:
**	GSTATUS		:	E_OG0001_OUT_OF_MEMORY
**					E_OG0006_HASH_NOT_NULL_TABLE
**					GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS
DDFmkhash(
		u_i4		size, 
		DDFHASHTABLE	*table)
{
	GSTATUS err = GSTAT_OK;
	DDFHASHTABLE	tab = NULL;

	G_ASSERT(*table, E_DF0006_HASH_NOT_NULL_TABLE);

	err = G_ME_REQ_MEM(0, tab, struct hashtable, 1);
	if (err == GSTAT_OK)
	{
		err = G_ME_REQ_MEM(0, tab->table, struct hashbucket*, size);		
		MEfill(sizeof(struct hashbucket*) * size, '\0', tab->table);
		if (err != GSTAT_OK) 
		{
			MEfree((PTR)tab);
		} 
		else 
		{
			tab->size = size;
			*table = tab;
		}
	}

	return(err);
}

/*
** Name: DDFrmhash()	- remove an hash table
**
** Description:
**
** Inputs:
**	DDFHASHTABLE*	: table pointer
**
** Outputs:
**	none
**
** Returns:
**	GSTATUS		:	E_DF0005_HASH_NULL_TABLE
**					GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  
DDFrmhash(
	   DDFHASHTABLE	*tab) 
{
	GSTATUS			err = GSTAT_OK;
	u_i4			i = 0;
	DDFHASHBUCKET		*b = NULL;
	DDFHASHBUCKET		k = NULL;
	DDFHASHBUCKET		d = NULL;

	G_ASSERT(*tab == NULL, E_DF0005_HASH_NULL_TABLE);

	b = (*tab)->table;
	i = (*tab)->size;
	while (i-- > 0) 
	{
		for (k = *b; k != NULL; )
		{
			d = k;
			k = k->next;
			MEfree((PTR)d);
		}
		b++;
	}
	MEfree((PTR)(*tab)->table);
	MEfree((PTR)*tab);
	*tab = NULL;

	return(err);
}

/*
** Name: DDFgetInternalhash()	- retrieve a value in the hash table 
**
** Description:
**
** Inputs:
**	DDFHASHTABLE*	: table pointer
**	char*		: text key
**	u_nat		: text length
**
** Outputs:
**	DDFHASHBUCKET*	: object retrieve 
**	u_nat *		: position in the table
**
** Returns:
**	GSTAT_OK
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  
DDFgetInternalhash(
	DDFHASHTABLE tab, 
	char* name, 
	u_i4 length, 
	DDFHASHBUCKET *bucket, 
	u_i4 *index) 
{
	GSTATUS		err = GSTAT_OK;
	DDFHASHBUCKET	b = NULL;
	DDFHASHBUCKET	p = NULL;
	char		*s1 = NULL;
	char		*s2 = NULL;
	u_i4		i=0, j = 0;

	for (j=0; j < length || (length == 0 && name[j] != EOS);j++) 
		i=(i<<1) + name[j];

	length = j;

	if((i%=(tab)->size)<0)
		i+=(tab)->size;
	*index =(u_i4) i;

	for (p = NULL, b = tab->table[i]; b; p = b, b = b->next)
	{
		s1 = b->name;
		s2 = name;

		for (j = 0; j < length && s1[j] != EOS && s1[j] == s2[j]; j++) ;

		if (j == length && s1[j] == EOS)
			{
				*bucket = b;
				return(err);
			}
	}
	*bucket = NULL;
return (err);
}

/*
** Name: DDFgethash()	- retrieve a value in the hash table
**
** Description:
**
** Inputs:
**	DDFHASHTABLE*	: table pointer
**	char*		: text key
**	u_nat		: text length
**
** Outputs:
**	PTR*		: object pointer
**
** Returns:
**	GSTAT_OK
**  E_DF0005_HASH_NULL_TABLE
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  
DDFgethash(
	DDFHASHTABLE tab, 
	char* name, 
	u_i4 length, 
	PTR *obj) 
{
	GSTATUS		err = GSTAT_OK;
	DDFHASHBUCKET	b = NULL;
	u_i4		index = 0;

	G_ASSERT(!tab, E_DF0005_HASH_NULL_TABLE);

	*obj = NULL;

	err = DDFgetInternalhash(tab, name, length, &b, &index);
	if (b != NULL) 
		*obj = b->object;
return(err);
}

/*
** Name: DDFputhash()	- insert a new value into the hash table
**
** Description:
**
** Inputs:
**	DDFHASHTABLE*	: table pointer
**	char*		: text key	(must be a permanent allocation block 
**							 because it uses by the hash table)
**	u_nat		: text length
**	PTR			: object pointer
**
** Outputs:
**	none
**
** Returns:
**		GSTAT_OK
**		E_OG0001_OUT_OF_MEMORY
**		E_DF0005_HASH_NULL_TABLE
**		E_DF0002_HASH_ITEM_ALR_EXIST
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  
DDFputhash(
	DDFHASHTABLE tab, 
	char* name, 
	u_i4 length, 
	PTR obj) 
{ 
	GSTATUS		err = GSTAT_OK;
	u_i4		index = 0;
	DDFHASHBUCKET	newB = NULL;

	G_ASSERT(!tab, E_DF0005_HASH_NULL_TABLE);

	err = DDFgetInternalhash(tab, name, length, &newB, &index);
	if (newB == NULL) 
	{
		err = G_ME_REQ_MEM(0, newB, struct hashbucket, 1);		
		if (tab->table[index] != NULL)
			tab->table[index]->previous = newB;
		newB->next = tab->table[index];
		tab->table[index] = newB;
		newB->name = name;
		newB->object = obj;
	} 
	else
		err = DDFStatusAlloc (E_DF0002_HASH_ITEM_ALR_EXIST);
return(err);
}

/*
** Name: DDFdelhash()	- delete a value from the hash table
**
** Description:
**
** Inputs:
**	DDFHASHTABLE*	: table pointer
**	char*		: text key(must be a permanent allocation block 
**							because it uses by the hash table)
**	u_nat		: text length
**
** Outputs:
**	PTR*		: object pointer
**
** Returns:
**	GSTAT_OK
**  E_DF0005_HASH_NULL_TABLE
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  
DDFdelhash(
	DDFHASHTABLE tab, 
	char* name, 
	u_i4 length, 
	PTR *obj) 
{ 
	GSTATUS		err = GSTAT_OK;
	u_i4		index = 0;
	DDFHASHBUCKET	B = NULL;

	G_ASSERT(!tab, E_DF0005_HASH_NULL_TABLE);

	err = DDFgetInternalhash(tab, name, length, &B, &index);
	if (B != NULL) 
	{
		*obj = B->object;
		if (B->previous != NULL)
			B->previous->next = B->next;
		else
			tab->table[index] = B->next;

		if (B->next != NULL)
			B->next->previous = B->previous;
		MEfree((PTR)B);
	}
return(err);
}

/*
** Name: DDFrehash()	- dynamic change of the hash table size
**
** Description:
**
** Inputs:
**	DDFHASHTABLE*	: table pointer
**	u_nat		: new size
**
** Outputs:
**	none
**
** Returns:
**	GSTAT_OK
**  E_OG0001_OUT_OF_MEMORY
**  E_DF0005_HASH_NULL_TABLE
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

GSTATUS  
DDFrehash(
	DDFHASHTABLE *tab, 
	u_i4 size) 
{
	GSTATUS		err = GSTAT_OK;
	u_i4		i = 0, index = 0;
	DDFHASHTABLE	newTab = NULL;
	DDFHASHBUCKET	b = NULL, tmp = NULL;

	G_ASSERT(*tab == NULL, E_DF0005_HASH_NULL_TABLE);

	if (size != 0 && (*tab)->size != size)
	{
		err = DDFmkhash(size, &newTab);
		if (err == GSTAT_OK)
		{
			for (i=0; i <(*tab)->size; i++) 
			{
				b = (*tab)->table[i];
				while (b != NULL) 
				{
					err = DDFgetInternalhash(newTab, b->name, -1, &tmp, &index);
					tmp = b;
					b = b->next;

					tmp->next = newTab->table[index];
					newTab->table[index] = tmp;
				}
			}
			MEfree((PTR)*tab);
			*tab = newTab;
		}
	}
return(err);
}

/*
** Name: DDFscanhash()	- scanning procedure to browse hash table element
**
** Description:
**
** Inputs:
**	DDFHASHTABLE*	: table pointer
**	u_nat		: new size
**
** Outputs:
**	none
**
** Returns:
**	GSTAT_OK
**  E_OG0001_OUT_OF_MEMORY
**  E_DF0005_HASH_NULL_TABLE
**
** Exceptions:
**	None
**
** Side Effects:
**	None
**
** History:
*/

bool  
DDFscanhash(
	DDFHASHSCAN *sc, 
	PTR *value) 
{
	*value = NULL;
	if (sc != NULL && sc->table != NULL)
	{
		if (sc->position == 0 && sc->lastbucket == NULL) 
		{
			sc->position = -1;
			do 
			{
				if (++sc->position >= sc->table->size) 
				{
					sc->lastbucket = NULL;
					break;
				}
				sc->lastbucket = sc->table->table[sc->position];
			} while (sc->lastbucket == NULL);
    }

		if (sc->lastbucket != NULL) 
		{
			*value = sc->lastbucket->object;
			do 
			{
        if (sc->lastbucket != NULL)
          sc->lastbucket = sc->lastbucket->next;
        else
        {
				  if (++sc->position >= sc->table->size) 
				  {
					  sc->lastbucket = NULL;
					  break;
				  }
				  sc->lastbucket = sc->table->table[sc->position];
        }
			} while (sc->lastbucket == NULL);
		}
	}
return(*value != NULL);
}
