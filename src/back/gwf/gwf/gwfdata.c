/*
**Copyright (c) 2004 Ingres Corporation
*/

/*
**
LIBRARY = IMPGWFLIBDATA
**
*/

# include <compat.h>
# include <gl.h>
# include <sl.h>
# include <st.h>
# include <me.h>
# include <iicommon.h>    
# include <dbdbms.h>  
# include <ulf.h> 
# include <ulm.h>    
# include <adf.h> 
# include <cs.h>  
# include <scf.h> 
# include <dmf.h>
# include <dmtcb.h>     
# include <dmrcb.h>     
# include <gwf.h>
# include <gwfint.h>

/*
** Name:	gwfdata.c
**
** Description:	Global data for gwf facility.
**
** History:
**
**	23-sep-96 (canor01)
**	    Created.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPGWFLIBDATA
**	    in the Jamfile.
**	24-Feb-2004 (kodse01)
**	    Removed gwxit.h inclusion which is not required.
*/

/* gwffac.c */
GLOBALDEF	GW_FACILITY	*Gwf_facility ZERO_FILL;
GLOBALDEF	DB_STATUS	((*Dmf_cptr))() ZERO_FILL;
