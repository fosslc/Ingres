/*
** Copyright (c) 2004 Ingres Corporation
*/
# include	 <compat.h>
# include	 <me.h>
# include	 <generr.h>
# include	 "erls.h"

# ifdef VMS
# include	 "iivmssd.h"

/*
** IIDESC.C - VMS system dependent routine to return string descriptor 
**	      addresses (or copies) to users to setup Param addresses.
** These routines should not be ported to non-VMS environments.  
**
** Defines:	IIdesc( sd )
**		IImdesc( sdold, sdnew )
** History:
**		27-feb-1985	- Rewritten from IIsdscr.c (ncg)
**		07-jul-1986	- Modified to allocate from pool (ncg)
**		12-dec-1988	- Generic error processing. (ncg)
**		02-aug-1989	- Shut up ranlib. (GordonW)
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**  11-Sep-2003 (fanra01)
**      Bug 110903
**      Add windows specific definition of IIdesc.
*/

/*
**  IIDESC - Return pointer to string descriptor
**
**  Now allocates local descriptor for user, to avoid the IImdesc() 
**  confusion. (ncg)
*/

char	*
IIdesc( sd )
register SDESC	*sd;
{
    register  SDESC 	*nsd;
    static    i4     	sd_cnt = 0;
    static    SDESC	*sd_pool ZERO_FILL;
#   define    SD_MAX	30

    if (sd == (SDESC *)0)
	    return (char *)0;
    if (sd_cnt == 0)
    {
	if (MEcalloc(SD_MAX, sizeof(*sd), (i4 *)&sd_pool) != OK)
	{
	    /* Fatal error */
	    IIlocerr(GE_NO_RESOURCE, E_LS0101_SDALLOC, 0, 0 );
	    IIabort();
	}
	sd_cnt = SD_MAX;
    }
    nsd = sd_pool;
    sd_pool++;
    sd_cnt--;
    MEcopy( (char *)sd, (u_i2)sizeof(SDESC), (char *)nsd );
    return (char *)nsd;
}


/*
**  IIMDESC - Return pointer to moved string descriptor.
**
**  The second parameter is given (must be 8 bytes), move the descriptor
**  from the first to the second, and return a pointer to the second.
*/

char	*
IImdesc( sd, nsd )
register SDESC	*sd, *nsd;
{
    if ( sd == (SDESC *)0 || nsd == (SDESC *)0 )
	return (char *)sd;

    MEcopy( (char *)sd, (u_i2)sizeof(SDESC), (char *)nsd );
    return (char *)nsd;
}
# else
static	i4	ranlib_dummy;
# ifdef NT_GENERIC
# include	 "iivmssd.h"
/*
** IIDESC.C - System dependent routine to return string descriptor 
**	      addresses (or copies) to users to setup Param addresses.
** These routines should not be ported to non-VMS environments.  
**
** Defines:	IIdesc( sd )
** 
** Inputs:
**      data            pointer user parameter
**      type            type of parameter
**      len             size of data
**      
** Outputs:
**      None
**
** Returns:
**      (char*)0        parameter
**      nsd             descriptor for memory area
**      
** History:
**      11-Sep-2003 (fanra01)
**          Bug 110903
**          Created based on VMS version
*/

char	*
IIdesc( data, type, len )
register char	*data;
register i4 	*type;
register i4     *len;
{
    register  SDESC 	*nsd;
    static    i4     	sd_cnt = 0;
    static    SDESC	*sd_pool ZERO_FILL;
#   define    SD_MAX	30

    if (data == NULL)
	    return (char *)0;
    if (sd_cnt == 0)
    {
	if ((sd_pool = MEreqmem( 0, SD_MAX*sizeof(*nsd), TRUE, NULL )) == NULL)
	{
	    /* Fatal error */
	    IIlocerr(GE_NO_RESOURCE, E_LS0101_SDALLOC, 0, 0 );
	    IIabort();
	}
	sd_cnt = SD_MAX;
    }
    nsd = sd_pool;
    sd_pool++;
    sd_cnt--;
    nsd->sd_len = *len;
    nsd->sd_type = *type;
    nsd->sd_ptr = data;
    
    return (char *)nsd;
}
# endif /* NT_GENERIC */
# endif /* VMS */
