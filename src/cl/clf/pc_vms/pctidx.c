/*
** Copyright (c) 2007 Ingres Corporation
*/
# include       <compat.h>
# include       <pc.h>
# include       <vmsclient.h>
 
/*
** Name: PCtidx
**
** Description:
**      This function is merely a wrapper for PCtid() on OpenVMS.
**
**  History:
**      05-mar-2007 (abbjo03)
**          Created.
**      19-jun-2997 (Ralph Loen) SIR 118032
**          Added vmsclient.h so that PCtid() resolves to pthread_self() 
**          instead of zero.
**
*/
u_i4
PCtidx(void)
{
    /* Use PCtid() itself */
    return PCtid();
}
