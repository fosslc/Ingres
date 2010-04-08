/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#include	<compat.h>
# include	<st.h>
#include	<ci.h>
#include	<me.h>
#include	<ex.h>
#include	<lo.h>
#include	<si.h>
#include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
#include	<fe.h>
#include	<ug.h>
#include	<iltypes.h>
#include	<ilops.h>
/* Interpreted Frame Object definition. */
#include	<ade.h>
#include	<frmalign.h>
#include	<fdesc.h>
#include	<ilrffrm.h>
/* ILRF Block definition. */
#include	<abfrts.h>
#include	<ioi.h>
#include	<ifid.h>
#include	<ilrf.h>

#include	"cggvars.h"
#include	"cgilrf.h"
#include	"cgil.h"
#include	"cgerror.h"

/**
** Name:    cgmain.c -	Code Generator principal Entry Point Routine.
**
** Description:
**	Contains the main routine for the code generator that produces
**	C code from the OSL intermediate language.
**
**	IICGcgCodeGen()	code generator entry point.
**
** History:
**	13-Jan-93 (fredb) hp9_mpe
**		Moved STlcopy call outside ifdef'd code and changed MPE 
**		specific code in IICGcgCodeGen to use lobuf like everybody else.
**	13-Jan-93 (fredb) hp9_mpe
**		Integrate dcarter's change from the 6202p codeline, see
**		detailed comments below.
**	Revision 6.4
**	04/07/91 (emerson)
**		Modifications for local procedures (in IICGcgCodeGen).
**
**	Revision 6.3/03/01
**	01/09/91 (emerson)
**		Remove 32K limit on IL (allow up to 2G of IL).
**		This entails introducing a modifier bit into the IL opcode,
**		which must be masked out to get the opcode proper.
**
**	Revision 6.0  87/02  arthur
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/*{
** Name:    IICGcgCodeGen() -	Code Generator Main Line and Entry Point.
**
** Description:
**	entry point for the code generator
**
** Inputs:
**	framename	{char *} name of the frame.
**	formname	{char *} name of the form.  (NULL if procedure).
**	symbol		{char *} name of the code entry point.
**	outname		{char *} name of the output file.
**	frameid		{i4} frame identifier
**	igframe		{PTR} in-core frame created by the translator.
**
** Outputs:
**	none.
**
** Returns:
**	OK on success.  On failure, returns whatever low-level stuff gave us.
**
** History:
**	18-feb-1987 (agh)
**		First written as a seperate process.
**	18-jan-1989 (billc)
**		Changed into a procedure call.
**	9/90 (Mike S)
**		ifdef'ed for hp9_mpe.  Porting chnage 130414
**	04/07/91 (emerson)
**		Modifications for local procedures:
**		Changed calling sequence to IIORsoSessOpen.
**	13-Jan-93 (fredb) hp9_mpe
**		Integrate dcarter's change from the 6202p codeline:
**			29-Nov-90 (dcarter)
**				For HP3000 only, LOdelete the target file if it 
**				already exists.  This system has the notion of
**				"limit" on a file, and the pre-existing copy
**				may not be big enough to hold what we're going
**				to write.
**	13-Jan-93 (fredb) hp9_mpe
**		Moved STlcopy call outside ifdef'd code and changed MPE 
**		specific code to use lobuf like everybody else.
**	10-mar-93 (leighb) DeskTop Porting Change:
**		ifdef'ed for PMFEWIN3.  Similiar problem ad hp9_mpe,
**		somebody oughtta clean this up!
**	28-Nov-2000 (hanje04)
**		Removed SIflush and SIclose after IICGoflFlush in
**		IIGCcgCodeGen(). IIGCoflFlush also performs an
**		SIflush and SIclose on the same pointer, and this causes 
**		a SEGV to occur when compiling some ABF application on Linux.
**		This is because fclose() (used in SIclose) has undefined
**		behaviour under these contitions.
*/
static VOID	GenLoop();

STATUS
IICGcgCodeGen(framename, formname, symbol, outname, frameid, igframe, fr_static)
    char *framename;
    char *formname;
    char *symbol;
    char *outname;
    i4	frameid;
    PTR	igframe;
    bool fr_static;
{
    EX_CONTEXT  cgcontext;
    EX  IICGexhExitHandler();
    FID fid;
    i4  ilsestype;

    /* set up our globals to point to these args. */
    CG_frame_name = framename;
    CG_symbol_name = symbol;
    CG_of_name = outname;
    CG_fid = frameid;
    CG_static = fr_static;

    /* no form-name indicates that this is an osl procedure. */
    if (formname == NULL || *formname == EOS)
	CG_form_name = NULL;
    else
	CG_form_name = formname;

    /*
    ** Declare exit handler for the code generator.
    ** This handler closes the ILRF session, and calls the general
    ** front-end exit handler FEhandler().
    */
    if (EXdeclare(IICGexhExitHandler, &cgcontext) != OK)
    {
	EXdelete();
	return (FAIL);
    }

    /*
    ** Open ILRF session.
    ** No filename for the runtime table is required in this case.
    ** Also, no names of routines for handling the stack frame need be
    ** passed in.  We may need to pass in a ptr to an in-core frame.
    */

    if (igframe == (PTR) NULL)
	    ilsestype = IORI_DB;
    else
	    ilsestype = IORI_CORE;

    /* set up the FID struct to pass to ILRF */
    fid.name = CG_frame_name;
    fid.id = CG_fid;

    if (IIORsoSessOpen(&IICGirbIrblk, ilsestype, igframe, &fid, (char *) NULL,
		NULL) != OK)
    {
	IICGerrError(CG_SESSOPEN, (char *) NULL);
    }

    /*
    ** Get interpreted frame object for this frame.
    */
    IICGfrgFrmGet(CG_frame_name, CG_fid);

    /*
    ** Open file to generate C code into.
    */
    if (CG_of_name == NULL || *CG_of_name == EOS)
    {
	CG_of_name = NULL;
	CG_of_ptr = stdout;
    }
    else
    {
	LOCATION	loc;
	char		lobuf[MAX_LOC+1];

	STlcopy(CG_of_name, lobuf, sizeof(lobuf));
# ifdef hp9_mpe
	LOfroms(PATH&FILENAME, lobuf, &loc);
	LOdelete( loc );
# else
	STlcopy(CG_of_name, lobuf, sizeof(lobuf));
#    ifdef	PMFEWIN3
	   LOfroms(PATH&FILENAME, lobuf, &loc);
#    else
	   LOfroms(FILENAME, lobuf, &loc);
#    endif /* PMFEWIN3 */
# endif	/* hp9_mpe */
	if (SIfopen(&loc, ERx("w"), SI_TXT, 512, &CG_of_ptr))
	    IICGerrError(CG_NOOUT, CG_of_name, (char *) NULL);
    }

    /*
    ** Initialize label stack.
    */
    IICGlbiLblInit();

    /*
    ** Begin to generate C code into the file.
    */
    IICGfltFileTop();

    /*
    ** Start generating code from the IL statements for the frame.
    */
    GenLoop();

    /*
    ** Close C file, flush buffer to output file, and close file.
    */
    IICGpeProcEnd();
    IICGoflFlush();

    /*
    ** Clean up ILRF session, backend and exception handlers
    ** before exiting.
    */ 
    IIORscSessClose(&IICGirbIrblk);

    EXdelete();	/* IICGexhExitHandler */

    return(OK);
}

/*{
** Name:    GenLoop() -	Main loop of the code generator
**
** Description:
**	Main loop of the code generator.
**	Generates C code based on the IL statements read.
**	Stops when the frame's IL has been completely read through.
**
** History:
**	18-feb-1987 (agh)
**		First written
*/
static VOID
GenLoop()
{
    register IL	*stmt;

    bool	IICGlrqLabelRequired();

    while (!IICGdonFrmDone && (stmt = IICGgilGetIl(CG_il_stmt)) != NULL)
    {
	/*
	** Generate a label for this IL statement, if required.
	*/
	if (IICGlrqLabelRequired(stmt))
		IICGlblLabelGen(stmt);

	(*(IIILtab[*stmt&ILM_MASK].il_func))(stmt);
    }
    return;
}
