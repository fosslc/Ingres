/*
** Copyright (c) 2004 Ingres Corporation
*/
# include       <compat.h>
# include       <pc.h>
 
/*
** Name: PCtidx
**
** Description:
**      This function is merely a wrapper for PCtid() on windows platforms.
**      It appears that Windows OS returns a well-behaved thread ID.
**
**  History:
**      31-Jan-2007 (rajus01)
**          Stripped down version of pctidx() on UNIX platforms.
**
*/
 
u_i4
PCtidx(void)
{
    /* Use PCtid() itself */
    return( PCtid() );
}
