/*
** Copyright (c) 1993, 1999 Ingres Corporation
*/
# include <compat.h>
# include <gl.h>
# include <me.h>

/**
** Name:        rsmemmgr.c	- Replicator Server Large memory 
**			  	  allocation/release. 
**
** Description:
**      Defines
**              RSmem_allocate	- allocate memory buffer 
**		RSmem_free	- free memory buffer
** History:
**      24-Feb-03 (inifa01)
**              Created.
**/

/*{
** Name:        RSmem_allocate  - Rep server large memory allocation. 
**
** Description:
**      Computes and allocates memory size needed for buffers to hold 
**	replicator server internal queries.
**
** Inputs:
**      row_width               - Total row width of registered cols.
**      num_cols                - Number of registered cols. 
**      colname_space           - Maximum name space of registered cols. 
**	overhead		- Additional query overhead
**
** Outputs:
**      NONE		
**
** Returns:
**      buf      		- NULL or Buffer successfully allocated.
*/
char *
RSmem_allocate(
i4 row_width,
i4 num_cols,
i4 colname_space,
i4 overhead)
{
	char *buf = (char *)MEreqmem(0, (num_cols * colname_space + row_width + overhead), TRUE, NULL);

	if (buf == NULL)
	{
	    /* Error allocating buf */
	    messageit(1, 1900);
	    return(buf) ;
	}
	
	return(buf);
}

VOID
RSmem_free(
char *buf1,
char *buf2,
char *buf3,
char *buf4)
{
	if(buf1)
	   MEfree((PTR)buf1);

	if(buf2)
	   MEfree((PTR)buf2);
	 
	if(buf3)
	   MEfree((PTR)buf3);
	
	if(buf4)
	   MEfree((PTR)buf4);

}
