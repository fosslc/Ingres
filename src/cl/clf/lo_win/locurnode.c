/*
**	LOcurnode	This is a dummy routine from cfe/3.0 CL
**
**	History:
**
**	9/9/87 (daveb) --
**			Change from FAIL to OK.  We don't need to do anything,
**			and don't really want the caller to go into error
**			handling.  This makes network copying of stuff
**			from abf work right.  (cf:  in/innetreq.c and
**			abf/abfobjs.qc).
**	15-jul-93 (ed)
**	    adding <gl.h> after <compat.h>
*/

# include	<compat.h>
# include	<gl.h>
# include	<lo.h>

/*ARGSUSED*/
LOcurnode( loc )
LOCATION *loc;
{
	return( OK );
}
