/*
**	iiftfake.c
**
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

# include	<compat.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<fe.h>
# include	<ft.h>
# include	<fmt.h>
# include	<adf.h>
# include	<frame.h>
# include	<menu.h>
# include	<runtime.h>
# include	<frserrno.h>
# include	<rtvars.h>
# include       <er.h>

/**
** Name:	iiftfake.c
**
** Description:
**	IBM support routines.
**
**	Public (extern) routines defined:
**		IIFT_fake()
**	Private (static) routines defined:
**
** History:
**	Jan. 1985 (dkh) - Written by Neil Goodman to support table field
**			  scrolling for IBM version of FT.
**	08/26/87 (dkh) - Changes for 8000 series error numbers.
**	09/01/87 (dkh) - Added explicit include of <frserrno.h>.
**	21-mar-94 (smc) Bug #60829
**		Added #include header(s) required to define types passed
**		in prototyped external function calls.
**	21-Aug-2009 (kschendel) 121804
**	    Update some of the function declarations to fix gcc 4.3 problems.
**/

/*{
** Name:	IIFT_fake
**
** Description:
**    This routine is called only by the IBM version of FTrun when
**    the user is trying to add a new row to a table field in 'fill'
**    mode and number of displayed rows (the number of dataset records)
**    is less than the number of physical rows in the table field
**    (i.e.:  tb->tb_display < tb->tb_numrows)
**
**    IIFT_fake simulates the second clause of IIscr_fake, where a new
**    dummy row is added to the bottom of the dataset. (This is necessary
**    so that the IBM version of FT can tell TBACC that a new row
**    has been added *without* returing back using tfscrup.)
**
** Inputs:
**	tbname		Name of the tablefield
**
** Outputs:
**
** Returns:
**
** Exceptions:
**	none
**
** Side Effects:
**
** History:
**	(scl and ncg) Written to correct IBM FT table field
**			     scrolling problem.
**
**	
*/

IIFT_fake(tbname)
   char *tbname;
{
	TBSTRUCT	*tb;
        DATSET		*ds;
     
        tb = IItfind( IIstkfrm, tbname );
     
        ds = tb->dataset;
     
        if ( ds == NULL )
        {
           IIFDerror(TBNODATSET, 1 , tb->tb_name);
           return;
        }
     
        /* append new row to bot (disbot) and update counters */
        IIdsinsert(tb, ds, ds->bot, (TBROW *) NULL, stNEWEMPT);
        ds->disbot = ds->disbot->nxtr;
        tb->tb_display++;
     
        /* don't display any data, because we don't want to physically */
        /* update the screen; IBM FTrun will do that                   */
     
        tb->tb_fld->tflastrow++;
	ds->ds_records++;
}
