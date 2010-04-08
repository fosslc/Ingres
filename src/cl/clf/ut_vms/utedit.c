/*
** Copyright (c) 1983, 2008 Ingres Corporation
*/

# include	<compat.h> 
# include	<gl.h>
# include	<pc.h> 
# include	<er.h> 
# include	<lo.h> 
# include	<array.h>
# include	<ut.h>
# include 	<descrip.h>
# include	<st.h>

/*
 *
 *	Name:
 *		UTedit.c
 *
 *	Function:
 *	 	UTedit
 *
 *	Arguments:
 *		LOCATION	*editor;		* name of editor to spawn *
 *		LOCATION	*filename;		* name of file to edit    *
 *
 *	Result:
 *		edit the file 'filename' with 'editor'.  If 'editor' is
 *		NULL or can't be found the default editor is spawned.
 *		The default is ING_EDIT if it exists. If it doesn't then
 *		EDIT is used.
 *
 *		If the editor is "+EDT", "+TPU", or "+LSE" we'd like to 
 *		call the editor (EDT, TPU, or LSE respectively) in a VMS 
 *		shared image, rather than in a subprocess.  This simple goal 
 *		has compilcations, which are handled in the code below.
 *
 *		1.	TPUSHR has been incompatible between VMS versions.
 *			To avoid incorporating this version-incompatibility
 *			into COMPATLIB, we map TPUSHR at run time, rather
 *			than linking it in.  We handle a mapping failure,
 *			even though it should never happen.
 *		2.	Both callable EDT and callable TPU signal errors
 *			like "invalid file name" instead of handling
 *			them and returning an error status.  Thus the
 *			code in UTedcall.mar, to establish a condition handler
 *			and unwind back to UTedit on any error.  We don't
 *			expect the front ends to generate invalid file names,
 *			but we don't want to kill the process if one should.
 *		3.	Once callable EDT signals an error, it will refuse
 *			to run again, so we will use the subprocess.
 *		4.	LIB$FIND_IMAGE_SYMBOL is one of those helpful VMS
 *			routines which signals as well as returns errors;
 *			so we call it via a MACRO cover which "handles" all
 *			conditions.
 *		
 *	Side Effects:
 *		Obviously 'filename' can be changed.
 *
 *	History:
 *		03/83 -- (gb)
 *			written
 *		10/10/83 (dd) -- VMS CL
 *		5/18/89 (Mike S) -- Use subroutine calls where possible.
 *		08-mar-90 (sylviap)
 *            		Added parameters to PCcmdline.
 *              14-mar-90 (sylviap)
 *                      Passing a CL_RR_DESC to PCcmdline.
 *
**      16-jul-93 (ed)
**	    added gl.h
**	22-sep-93 (ricka)
**	    added st.h, should have been submitted prior to the 20-sep integ.
**      18-dec-2008 (joea)
**          Replace READONLY/WSCREADONLY by const.
 */

FUNC_EXTERN	i4	UTfind_image_symbol();
FUNC_EXTERN	i4	UTedcall();
FUNC_EXTERN 	VOID	NMgtAt();
FUNC_EXTERN	STATUS	PCcmdnointr();

# define NO_ADDRESS	(i4 (*)()) 0
# define BAD_ADDRESS	(i4 (*)()) 1

static const char edt_name[] 	= ERx("+EDT");
static const char edt_image[] 	= ERx("EDTSHR");
static const char edt_symbol[] 	= ERx("EDT$EDIT");
static $DESCRIPTOR( edt_image_desc, edt_image);
static $DESCRIPTOR( edt_symbol_desc, edt_symbol);

static const char tpu_name[] 	= ERx("+TPU");
static const char tpu_image[] 	= ERx("TPUSHR");
static const char tpu_symbol[] 	= ERx("TPU$EDIT");
static $DESCRIPTOR( tpu_image_desc, tpu_image);
static $DESCRIPTOR( tpu_symbol_desc, tpu_symbol);

static const char lse_name[] 	= ERx("+LSE");
static const char lse_image[] 	= ERx("LSESHR");
static const char lse_symbol[] 	= ERx("LSE$EDIT");
static $DESCRIPTOR( lse_image_desc, lse_image);
static $DESCRIPTOR( lse_symbol_desc, lse_symbol);

STATUS
UTedit(editor, filename)
char		*editor;
LOCATION	*filename;
{
	char	command[256];		/* DCL command to print filename */
	char	*fname;			/* pointer to filename */
	i4	status;
	i4	arglist[3];
	static  i4 (*edt_address)() = NO_ADDRESS;
	static  i4 (*tpu_address)() = NO_ADDRESS;
	static  i4 (*lse_address)() = NO_ADDRESS;
	CL_ERR_DESC	err_code;

	/* string descriptors */
 	$DESCRIPTOR(fdesc, ERx(""));	
	$DESCRIPTOR(nulldesc, ERx(""));

	if ((filename == NULL) )
		return(UT_ED_CALL);
        LOtos(filename, &fname);
        if  (fname == NULL)
		return(UT_ED_CALL);

	/*      
	**	Use :	1. The "editor" argument, if one was specified.
	**		2.  The value of "ING_EDIT", if it's defined.
	**		3.  "+EDT" (Callable EDT).
	*/
	if((editor == NULL) || (*editor == EOS))
	{
		NMgtAt("ING_EDIT", &editor);
		if ((editor == NULL) ||  (*editor == EOS))
			editor = edt_name;
	}

	/* call the editor or spawn a CLI subprocess that will edit the file */
	if ( STbcompare(editor, 0, edt_name, 0, TRUE) == 0)
	{
	        if (edt_address == NO_ADDRESS)
		{
		  	status = UTfind_image_symbol(
				&edt_image_desc, &edt_symbol_desc, 
				&edt_address);
			if (! (status & 1) ) edt_address = BAD_ADDRESS;
		}         
		if (edt_address != BAD_ADDRESS)
		{
			/* 
			** Set up arglist for EDT  -- EDT$EDIT(infile) 
			*/
			fdesc.dsc$a_pointer = fname;
			fdesc.dsc$w_length = STlength(fname);
			arglist[0] = 1;
			arglist[1] = &fdesc;

			status = UTedcall( edt_address, arglist);
			/*
			**	Callable EDT can't recover from an error.
			**	If it failed, use the subprocess henceforth.
			*/
			if (status & 1)
			{
			    return OK;
			}
			else
			{
				edt_address = BAD_ADDRESS;
				return status;
			}
		}
	}
	else if ( STbcompare(editor, 0, tpu_name, 0, TRUE) == 0)
	{
	        if (tpu_address == NO_ADDRESS)
		{
		  	status = UTfind_image_symbol(
				&tpu_image_desc, &tpu_symbol_desc, 
				&tpu_address);
			if (! (status & 1) ) tpu_address = BAD_ADDRESS;
		}         
		if (tpu_address != BAD_ADDRESS)
		{
			/* 
	 		**	Set up arglist for TPU  -- 
			**	TPU$EDIT(infile, outfile)
			**	"outfile" is a null string, to indicate that 
			**	it's the same as "infile".
			*/
			fdesc.dsc$a_pointer = fname;
			fdesc.dsc$w_length = STlength(fname);
			arglist[0] = 2;
			arglist[1] = &fdesc;
			arglist[2] = &nulldesc;
	
			status = UTedcall( tpu_address, arglist);
			if (status & 1)
			{
				return OK;
			}
			else
			{
				return status;
			}
		}
	}
	else if ( STbcompare(editor, 0, lse_name, 0, TRUE) == 0)
	{
	        if (lse_address == NO_ADDRESS)
		{
		  	status = UTfind_image_symbol(
				&lse_image_desc, &lse_symbol_desc, 
				&lse_address);
			if (! (status & 1) ) lse_address = BAD_ADDRESS;
		}         
		if (lse_address != BAD_ADDRESS)
		{
			/* 
	 		**	Set up arglist for LSE  -- 
			**	LSE$EDIT(infile, outfile)
			**	"outfile" is a null string, to indicate that 
			**	it's the same as "infile".
			*/
			fdesc.dsc$a_pointer = fname;
			fdesc.dsc$w_length = STlength(fname);
			arglist[0] = 2;
			arglist[1] = &fdesc;
			arglist[2] = &nulldesc;
	
			status = UTedcall( lse_address, arglist);
			if (status & 1)
			{
				return OK;
			}
			else
			{
				return status;
			}
		}
	}
	/* 
	**	It's not a callable editor,  we couldn't get the
	**	symbol's address, or callable EDT has failed once. Use the 
	**	subprocess. If we have a callable editor name, get rid of
	**	the "+".
	*/
	if (*editor == '+')
		editor++;
	STpolycat(3, editor, " ", fname, command);
	return(PCcmdnointr(NULL, command, FALSE, NULL, FALSE, &err_code));
}
