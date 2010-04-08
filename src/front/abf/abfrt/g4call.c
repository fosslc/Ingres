/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<me.h>
# include	<st.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<adf.h>
# include	<fe.h>
# include	<erg4.h>
# include	<fdesc.h>
# include	<abfrts.h>
# include	<rtsdata.h>
# include	"g4globs.h"


/**
** Name:	g4call.c - functions for EXEC 4GL CALL statements.
**
** Description:
**	CALLFRAME and CALLPROC are not yet supported by ABF, so these
**	routines simply issue errors.
**
**	This file defines:
**
**	IIAG4icInitCall 	Initialize call
**	IIAG4vpValParam		Add by value parameter
**	IIAG4bpByrefParam	Add BYREF parameter
**	IIAG4rvRetVal		Specify return value
**	IIAG4ccCallComp		Complete call	
**	IIAG4seSendEvent	Send User event (unsupported)
**
** History:
**	30-dec-92 (davel)
**		Initial version.
**	13-Aug-93 (donc) Bugs 50410, 52908
**		Change IIAG4icInitCall was changed to allocate
**		additional space and assign a valid pointer for
**		prm->pr_oldprm.  Leaving this NULL was causing
**		iiarPassArg nightmares ( i.e SEG VIOS when 
**		passing to prcedures when images were run, and
**		failure to see passed args when frames where
**		called during 'Go'.  I also was forced to pass
**		the ABRTSPRM of prm to "iiinterp" via a global
**		IIarRtsprm. From within a 3GL one cannot alter
**		the interpreter's control stack.  As a result    
**		their was no way while running 'Go' to get a
**		3GL EXEC 4GL CALLFRAME.... to see it's call
**		arguments. The net net was that the called frame
**		would attempt to initialize its variables 
**		with those used to call the 3GL procedure (which
**		use different syntax ( E_AR0004 ).
**	26-aug-1993 (dianeh)
**		Changed call to MEcopy() (in add_param()) so arg1
**		is of the right type.
**	31-aug-1993 (mgw)
**		Did some more MEcopy() and MEfree() type casting for
**		prototyping.
**	21-sep-1993 (donc) Bug 55057
**		add_param(): set param_edv to point an allocated data area.
**	21-sep-1993 (donc) Bug 55059
**		initialize prm->pr_zero to 0. Otherwise the NEW_PARAM_MACRO
**		will flag the ABRTSPRM as being prior to release 6.0.
**	17-nov-93 (donc) Bugs 55057, 55059, and 56911
**		Move ABRTSPRM handling for EXEC 4GL calls so that the
**		same method works for 4GL procedure calls as well
**	17-nov-93 (donc)
**		Modified to have EXEC 4GL parameter passing to a procedure
**		in 'Go' use new wrapper routines ( IIARsarSetArRtsprm() )
**	22-nov-93 (donc) properly cast prm->oldprm.
**	30-dec-93 (donc) 
**		Changed add_param. When passing strings byref, we don't
**		want to fill in the edv's length until we copy it over to
**		call_args->param_edv. Doing it earlier futzes things up 
**		when the called frame or procedure performs operations
**		(like string concatenation) on these byref'ed strings (
**		IIARevlExprEval will generate a string that ends at the
**		first string in the concatenation expression).      
**	10-jan-94 (donc) More bug 55057
**		we only want to zero the length in add_param if the parameter
**		is DB_CHR_TYPE
**	28-jan-94 (donc) Bug 59062
**		Strings passed byref to frames and procedures were being
**		truncated upon return. I use STmove to blank pad out the
**		string passed to its declared length. That way the dbv passed
**		is of the correct full length and upon return contains enough
**		space to hold the returned contents. 
**	30-nov-98 (kitch01)
**		Bug 94312. If we need to reallocate the calling context stack
**		although the old stack is copied previous frames still have a 
**		pointer to the old (now free'd) context. This causes an accvio
**		when byref params are involved. So on return recalculate the
**		context required. New flag realloced introduced so that this
**		is only done when required.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	26-Aug-2009 (kschendel) b121804
**	    Remove function defns now in headers.
**/

/* # define's */
# define INIT_CONTEXT_SIZE 2	/* Initial # of calling contexts to alloc */
# define INIT_ARG_SIZE 	10	/* Initial # of calling args to alloc */

typedef struct 
{
    i4  state;			/* State of this context so far; one of: */
# define G4CALL_RAW	0	/* no members set yet */
# define G4CALL_UNUSED	1	/* was previously used, so some members set */
# define G4CALL_ERROR	2	/* error has occured during processing a call */
# define G4CALL_INIT	3	/* initialized with IIAG4icInitCall() */
# define G4CALL_PARAM	4	/* some params added successfully */
# define G4CALL_CALL	5	/* call in progress */
    ABRTSPRM *prm;		/* main parameter structure */
    i4  numalloc;		/* size of allocated prm->pr_formals|actuals
				** and param_edv */
    DB_EMBEDDED_DATA *param_edv;/* parameter values */
    DB_EMBEDDED_DATA *retedv;	/* return value */
    char *name;			/* name of frame or procedure */
    i4  type;			/* type of call (G4CT_PROC or G4CT_FRAME) */
    TAGID paramtag;		/* memory Tag for dynamic parts */
} G4CALLARGS;

/* GLOBALDEF's */
/* extern's */
FUNC_EXTERN STATUS IIARprocCall();
FUNC_EXTERN STATUS IIARfrmCall();

/* static's */
static STATUS alloc_new_callstack(void);
static STATUS add_param (char *name, i2 *ind, i4  isvar, i4  type, 
			 i4  length, PTR data, bool isbyref);

static G4CALLARGS *calling_contexts = NULL;
static i4  alloc_calling_contexts = 0;
static i4  curr_calling_context = -1;
static bool realloced = FALSE;

/*{
** Name:	IIAG4icInitCall - Initialize call
**
** Description:
**	Set up a calling arg structure (allocating a stack of these if
**	necessary), and save the type and name in the structure for the
**	eventual call.
**
** Inputs:
**	name		char *		frame/procedure name
**	type		i4		Frame or Procedure
**
**	Returns:
**		OK
**		E_G42727_MEM_ALLOC_FAIL
**
** History:
**	31-dec-92 (davel)
**		Initial version.
**	13-Aug-93 (donc)
**		Allocate space for (ABRTSOPRM)prm->pr_oldprm
**	        and set the pointer.
*/
STATUS
IIAG4icInitCall
(char *name, i4  type)
{	
    G4ERRDEF g4errdef;
    i4	caller = (type == G4CT_PROC) ? G4CALLPROC_ID : G4CALLFRAME_ID;
    G4CALLARGS *call_args;

    /* Issue the message */
    if (FALSE)
    {
    g4errdef.errmsg = E_G42715_NOT_SUPPORTED;
    g4errdef.numargs = 2;
    g4errdef.args[0] = iiAG4routineNames[caller];
    g4errdef.args[1] = ERx("ABF");
    IIAG4semSetErrMsg(&g4errdef, TRUE);
    return E_G42715_NOT_SUPPORTED;
    }

    /* see if we need to allocate another calling_context stack */
    if (curr_calling_context+1 == alloc_calling_contexts)
    {
	/* alloc_new_callstack() allocates a new stack and copies the old
	** one (if any) over, and then resets alloc_calling_contexts to the
	** new stacksize.
	*/
	if (alloc_new_callstack() != OK)
		return E_G42727_MEM_ALLOC_FAIL;
    }

    /* increment the current calling context, and initialize the arg struct */
    call_args = calling_contexts + ++curr_calling_context;
    if (call_args->state == G4CALL_RAW)
    {
	call_args->paramtag = FEgettag();
	call_args->state = G4CALL_UNUSED;
    }

    /* Initialize the parameter structure */
    call_args->prm = (ABRTSPRM *)FEreqmem(call_args->paramtag,
	 sizeof(ABRTSPRM) + sizeof(ABRTSOPRM), FALSE, (STATUS *)NULL);
    call_args->prm->pr_version = 2;
    call_args->prm->pr_zero = 0;
    call_args->prm->pr_argcnt = 0;
    call_args->prm->pr_retvalue = NULL;
    call_args->prm->pr_flags = 0;
    call_args->prm->pr_oldprm = (ABRTSOPRM *)((char *) call_args->prm 
						+ sizeof(ABRTSPRM) );
    call_args->prm->pr_oldprm->pr_tlist = NULL;
    call_args->prm->pr_oldprm->pr_qry = NULL;
    call_args->prm->pr_oldprm->pr_argv = NULL;
    call_args->prm->pr_oldprm->pr_tfld = NULL;
    call_args->numalloc = 0;
    call_args->type = type;
    call_args->name = name;
    call_args->state = G4CALL_INIT;
    return OK;
}


/*{
** Name:	IIAG4bpByrefParam - Add a BYREF parameter to the call.
**
** Description:
**	Add the specified parameter to the G4CALLARGS strcuture.  This
**	routine lets the add_param() static function do all the work -
**	see it for more details.
**
** Inputs:
**	ind		i2 *		NULL indicator
**	isvar		i4		passed by reference from 3GL?
**	type		i4		ADF type
**	length		i4		Data size
**	data		PTR		data pointer.
**	name		char *		parameter name
**
** Outputs:
**	NONE directly, although ind and data get set after the call completes.
**
**	Returns:
**		< see static add_param() >
**
** History:
**	31-dec-92 (davel)
**		Initial version.
*/
STATUS
IIAG4bpByrefParam
(i2 *ind, i4  isvar, i4  type, i4  length, PTR data, char *name)
{
    bool isbyref;
    return add_param(name, ind, isvar, type, length, data, isbyref = TRUE);
}

/*{
** Name:	IIAG4vpValParam - Add a value parameter to the call.
**
** Description:
**	Add the specified parameter to the G4CALLARGS strcuture.  This
**	routine lets the add_param() static function do all the work -
**	see it for more details.
**
** Inputs:
**	name		char *		parameter name
**	ind		i2 *		NULL indicator
**	isvar		i4		passed by reference from 3GL?
**	type		i4		ADF type
**	length		i4		Data size
**	data		PTR		data pointer.
**
** Outputs:
**	NONE
**
**	Returns:
**		< see static add_param() >
**
** History:
**	31-dec-92 (davel)
**		Initial version.
*/
STATUS
IIAG4vpValParam
(char *name, i2 *ind, i4  isvar, i4  type, i4  length, PTR data)
{
    bool isbyref;
    return add_param(name, ind, isvar, type, length, data, isbyref = FALSE);
}

/*{
** Name:	IIAG4rvRetVal - Specify return value
**
** Description:
**	This is unsupported.  As the calls are generated by the preprocessor,
**	this should never be called, so we won't bother issuing a message.
**
** Inputs:
**	ind		i2 *		NULL indicator
**	isvar		i4		passed by reference from 3GL?
**	type		i4		ADF type
**	length		i4		Data size
**	data		PTR		data pointer.
**
**	Returns:
**		OK
**		E_G42727_MEM_ALLOC_FAIL
**		E_G4271C_DATA_CONVERSION_SET
**
** History:
**	31-dec-92 (davel)
**		Initial version.
*/
STATUS
IIAG4rvRetVal
(i2 *ind, i4  isvar, i4  type, i4  length, PTR data)
{
    STATUS status;
    G4ERRDEF g4errdef;
    G4CALLARGS *call_args = calling_contexts + curr_calling_context;
    DB_DATA_VALUE dbv, bufdesc;
    DB_EMBEDDED_DATA *retedv;
    f8 setbuf[(DB_GW4_MAXSTRING + DB_CNTSIZE)/sizeof(f8) +1];
    DB_DATA_VALUE *retdbv;
    ADF_CB *adfcb = FEadfcb();

    /* allocate a DBV and an EDV for the return value */
    retdbv = (DB_DATA_VALUE *) FEreqmem(call_args->paramtag, 
					sizeof(DB_DATA_VALUE),
			      		FALSE, (STATUS *)NULL);
    retedv = (DB_EMBEDDED_DATA *) FEreqmem(call_args->paramtag, 
					sizeof(DB_EMBEDDED_DATA),
			      		FALSE, (STATUS *)NULL);
    if (retdbv == (DB_DATA_VALUE *)NULL || retedv == (DB_EMBEDDED_DATA *)NULL)
    {
	/* major problem - return memeory allocation error */
	i4 caller;
	g4errdef.errmsg = E_G42727_MEM_ALLOC_FAIL;
	g4errdef.numargs = 0;		/* XXX g4alloc error takes no arg yet */
	caller =(call_args->type == G4CT_PROC) ? G4CALLPROC_ID : G4CALLFRAME_ID;
	g4errdef.args[0] = iiAG4routineNames[caller];
	IIAG4semSetErrMsg(&g4errdef, FALSE);
	call_args->state = G4CALL_ERROR;
	return E_G42727_MEM_ALLOC_FAIL;
    }

    /* First, fill the embedded data descriptor */
    retedv->ed_type = type;
    retedv->ed_length = length;
    retedv->ed_null = ind;
    if (isvar)   
	retedv->ed_data = (PTR)data;
    else
	retedv->ed_data = (PTR)&data;

    /* Next, the buffer descriptor DBV */
    bufdesc.db_length = sizeof(setbuf);
    bufdesc.db_data = (PTR)setbuf;

    /* OK, figure out what type the DBV will be */
    status = adh_evtodb(adfcb, retedv, retdbv, &bufdesc, TRUE);
    if (status != OK)
    {
	/* Some unexpected strangeness XXX need a specific error message */
	i4 caller;
	g4errdef.errmsg = E_G4271C_DATA_CONVERSION_SET;
	g4errdef.numargs = 3;
	caller =(call_args->type == G4CT_PROC) ? G4CALLPROC_ID : G4CALLFRAME_ID;
	g4errdef.args[0] = iiAG4routineNames[caller];
	g4errdef.args[1] = ERget(F_G40006_PARAMETER);
	g4errdef.args[2] = ERx("UNKNOWN");
	IIAG4semSetErrMsg(&g4errdef, FALSE);
	call_args->state = G4CALL_ERROR;
	return E_G4271C_DATA_CONVERSION_SET;
    }
    /* also, allocate a data buffer for the DBV if the edv's is not used */
    if (retdbv->db_data == bufdesc.db_data)
    {
	retdbv->db_data = FEreqmem(call_args->paramtag, 
					retdbv->db_length,
			      		FALSE, (STATUS *)NULL);
	if (retdbv->db_data == (PTR)NULL)
	{
	    /* major problem - return memory allocation error */
	    i4  caller;
	    g4errdef.errmsg = E_G42727_MEM_ALLOC_FAIL;
	    g4errdef.numargs = 0;		/* XXX no arg yet */
	    caller =(call_args->type == G4CT_PROC) ? 
				G4CALLPROC_ID : G4CALLFRAME_ID;
	    g4errdef.args[0] = iiAG4routineNames[caller];
	    IIAG4semSetErrMsg(&g4errdef, FALSE);
	    call_args->state = G4CALL_ERROR;
	    return E_G42727_MEM_ALLOC_FAIL;
	}
	
    }
    /* OK, so now we have an allocated DBV for the return value.  Save into
    ** the ABRTSPRM member of the current calling context, and the user 
    ** variable's edv also.
    */
    call_args->prm->pr_rettype = DB_DBV_TYPE;
    call_args->prm->pr_retvalue = (i4 *)retdbv;
    call_args->retedv = retedv;

    return OK;
}

/*{
** Name:	IIAG4ccCallComp		Complete call to component	
**
** Description:
**	This completes the CALLPROC or CALLFRAME, given the parameter
**	structure created by previous calls to IIAG4icInitCall, 
**	IIAG4bpByrefParam, and IIAG4apAddParam.  After it returns from
**	calling the 4GL frame or procedure, it moves the return value (if
**	any) into the the user variables (the DB_EMBEDDED_DATA value is saved in
**	the G4CALLARGS struct), and also moves any byref parameter values back
**	to the user variables (the array of DB_EMBEDDED_DATA values is also 
**	saved in the G4CALLARGS struct).
**
**	Returns:
**		OK
**
** History:
**	31-dec-92 (davel)
**		Initial version.
**	13-Aug-93 (donC)
**		Pass the address of the ABRTSPRM through IIarRtsprm
**		so that that the called frame has the correct
**		argument list.
**	6-oct-93 (donc)
**		Make GLOBALDEF of IIarRtsprm a GLOBALREF, the GLOBALDEF
**		is moved to rtsdata.c and provided with a wrapper routine 
**		IIARgarGetArRtsprm to access it from the iiinterp. 
**	17-nov-93 (donc)
**		Use wrapper routine for access now. (IIARsarSetArRtsprm()) 
**	30-nov-98 (kitch01)
**		Bug 94312. Recalculate the call_args if the calling contexts
**		stack has been expanded. 
*/
STATUS
IIAG4ccCallComp(void)
{
    STATUS status;
    G4ERRDEF g4errdef;
    G4CALLARGS *call_args = calling_contexts + curr_calling_context;

    DB_DATA_VALUE *retdbv, *dbv;
    DB_EMBEDDED_DATA *retedv, *edv;
    register i4  argcnt;
    ABRTSPV *abrtspv;

    if (call_args->state < G4CALL_INIT)
    {
	/* Something has already gone wrong */
	i4 caller;
	caller =(call_args->type == G4CT_PROC) ? G4CALLPROC_ID : G4CALLFRAME_ID;
	g4errdef.errmsg = E_G42724_PREVIOUS_ERRS;
	g4errdef.numargs = 1;
	g4errdef.args[0] = iiAG4routineNames[caller];
	IIAG4semSetErrMsg(&g4errdef, TRUE);
	return E_G42724_PREVIOUS_ERRS;
    }

    /* call the procedure */
    call_args->state = G4CALL_CALL;

    IIARsarSetArRtsprm( (PTR)call_args->prm );
    if (call_args->type == G4CT_PROC) 
	status = IIARprocCall(call_args->name, call_args->prm);
    else 
	status = IIARfrmCall(call_args->name, call_args->prm);

    if (status != OK)
    {
	return E_G42722_BAD_NAME;	/* XXX better errr for no such frame */
    }

	/* Bug 94312 */
	if (realloced)
	{
		call_args = calling_contexts + curr_calling_context;
	}

    /* move the return value into the user variables */
    retdbv = (DB_DATA_VALUE *)call_args->prm->pr_retvalue;
    if (retdbv != (DB_DATA_VALUE *)NULL)
    {
	status = IIAG4get_data( retdbv, call_args->retedv->ed_null, 
					call_args->retedv->ed_type, 
					call_args->retedv->ed_length, 
					call_args->retedv->ed_data);
	/*  XXX ignore IIAG4get_data status for now */
    }

    /* move any byref parameters back into the user variables */
    for (abrtspv = call_args->prm->pr_actuals, edv = call_args->param_edv, 
	 argcnt = 0;
	 argcnt < call_args->prm->pr_argcnt; 
	 argcnt++, abrtspv++, edv++)
    {
	/* only deal with byref params */
	if (abrtspv->abpvtype == -DB_DBV_TYPE)
	{
	    dbv = (DB_DATA_VALUE *)abrtspv->abpvvalue;
	    status = IIAG4get_data(dbv, edv->ed_null, edv->ed_type, 
					edv->ed_length, edv->ed_data);
	    /*  XXX ignore IIAG4get_data status for now */
	}
    }

    /* 
    ** Dump any possible error messages that the sendevent method
    ** generated.
    */

    if (call_args->paramtag > 0)
	FEfree(call_args->paramtag);
    call_args->prm = NULL;
    call_args->retedv = NULL;
    call_args->param_edv = NULL;

    /* pop the calling context stack */
    call_args->state = G4CALL_UNUSED;
    curr_calling_context--;
    return OK;
}

/*{
** Name:	IIAG4seSendEvent	Send a user event to a frame
**
** Description:
**	This functionality is not support in ABF/Vision.
**
** Inputs:
**	frame	i4	The frame to send the event to.
**
** Returns:
**	STATUS = E_G42715_NOT_SUPPORTED
**
** History:
**	30-dec-92 (davel)
**		Initial version.
*/
STATUS
IIAG4seSendEvent(i4 frame)
{
    G4ERRDEF g4errdef;

    /* Issue the message */
    g4errdef.errmsg = E_G42715_NOT_SUPPORTED;
    g4errdef.numargs = 2;
    g4errdef.args[0] = iiAG4routineNames[G4SEND_UEV_ID];
    g4errdef.args[1] = ERx("ABF/Vision");
    IIAG4semSetErrMsg(&g4errdef, TRUE);
    return E_G42715_NOT_SUPPORTED;
}

/* allocate a new calling_context stack */
static STATUS
alloc_new_callstack(void)
{
    i4  newsize, i;
    PTR new_contexts;

    /* allocate the new calling stack */
    if (alloc_calling_contexts == 0)
	newsize = INIT_CONTEXT_SIZE;
    else
	newsize = 2 * alloc_calling_contexts;

    new_contexts = MEreqmem(0, newsize * sizeof(G4CALLARGS), 
				FALSE, (STATUS *)NULL);

    if (new_contexts == (PTR)NULL)
	return FAIL;

    /* copy over the previous stck to the new one, if there was one */
    if (alloc_calling_contexts > 0)
    {
	MEcopy((PTR)calling_contexts,
		   (u_i2)(alloc_calling_contexts * sizeof(G4CALLARGS)),
		   new_contexts);
	realloced = TRUE;
    }
    if (calling_contexts != (G4CALLARGS *)NULL)
	(void) MEfree((PTR)calling_contexts);
    calling_contexts = (G4CALLARGS *)new_contexts;
    alloc_calling_contexts = newsize;

    /* now initialize the new stack elements */
    for (i = curr_calling_context+1; i < alloc_calling_contexts; i++)
    {
	(calling_contexts + i)->state = G4CALL_RAW;
    }

    return OK;
}

/*{
** Name:	add_param - Add a parameter (byref or not) to the call.
**
** Description:
**	
**	Add the specified parameter to the current calling context.
**
** Inputs:
**	name		char *		parameter name
**	ind		i2 *		NULL indicator
**	isvar		i4		passed by reference from 3GL?
**	type		i4		ADF type
**	length		i4		Data size
**	data		PTR		data pointer.
**	isbyref		bool		Is param passed byref?
**
** Outputs:
**	NONE directly, although ind and data get set after the call completes
**	for byref parameters.
**
**	Returns:
**		OK
**
** History:
**	4-jan-93 (davel)
**		Initial Version.
*/
static STATUS
add_param
(char *name, i2 *ind, i4  isvar, i4  type, i4  length, PTR data, bool isbyref)
{
    STATUS status;
    G4ERRDEF g4errdef;
    G4CALLARGS *call_args = calling_contexts + curr_calling_context;
    DB_DATA_VALUE dbv, bufdesc;
    DB_EMBEDDED_DATA edv;
    f8 setbuf[(DB_GW4_MAXSTRING + DB_CNTSIZE)/sizeof(f8) +1];
    register i4  argcnt;
    PTR datptr;
    DB_DATA_VALUE *argptr;
    ABRTSPV *abrtspv;
    ADF_CB *adfcb = FEadfcb();
    
    if (call_args->state < G4CALL_INIT)
    {
	/* Something has already gone wrong */
	i4 caller;
	g4errdef.errmsg = E_G42724_PREVIOUS_ERRS;
	g4errdef.numargs = 1;
	caller =(call_args->type == G4CT_PROC) ? G4CALLPROC_ID : G4CALLFRAME_ID;
	g4errdef.args[0] = iiAG4routineNames[caller];
	IIAG4semSetErrMsg(&g4errdef, TRUE);
	return E_G42724_PREVIOUS_ERRS;
    }

    /*
    ** Convert the user's value to a DB_DATA_VALUE.
    */

    /* First, fill the embedded data descriptor */
    edv.ed_type = type;
    edv.ed_null = ind;
    edv.ed_length = length;
    if (isvar)   
	edv.ed_data = (PTR)data;
    else
	edv.ed_data = (PTR)&data;

    if ( edv.ed_type == DB_CHR_TYPE && length > STlength( edv.ed_data) )
        STmove( edv.ed_data, ' ', length, edv.ed_data );

    /* Next, the buffer descriptor DBV */
    bufdesc.db_length = sizeof(setbuf);
    bufdesc.db_data = (PTR)setbuf;

    /* OK, figure out what type the DBV will be */
    status = adh_evtodb(adfcb, &edv, &dbv, &bufdesc, TRUE);
    if (status == OK)
    {
       /* Now we can convert into the DBV */
       status = adh_evcvtdb(adfcb, &edv, &dbv);
    }
    if (status != OK)
    {
	/* Some unexpected strangeness */
	i4 caller;
	g4errdef.errmsg = E_G4271C_DATA_CONVERSION_SET;
	g4errdef.numargs = 3;
	caller =(call_args->type == G4CT_PROC) ? G4CALLPROC_ID : G4CALLFRAME_ID;
	g4errdef.args[0] = iiAG4routineNames[caller];
	g4errdef.args[1] = ERget(F_G40006_PARAMETER);
	g4errdef.args[2] = name;
	IIAG4semSetErrMsg(&g4errdef, FALSE);
	call_args->state = G4CALL_ERROR;
	return E_G4271C_DATA_CONVERSION_SET;
    }

    /* Now convert an object, if need be.  XXX open issue: how do we know
    ** the differnece between an i4 paramater and one that represents
    ** a complex object?  For now, disallow passing object handles back to
    ** 4GL back in their object form.
    */

    /* Now add the argument: first see if new actuals/formals array is needed */
    argcnt = call_args->prm->pr_argcnt;
    if (argcnt == call_args->numalloc)
    {
	/* Need to alloc more space */
	i4 newsize;
	PTR newactuals;
	PTR newformals;
	PTR newedvs;

	if (argcnt == 0)
	    newsize = INIT_ARG_SIZE;
	else
	    newsize = 2 * argcnt;

	newactuals = FEreqmem(call_args->paramtag,
		  	      newsize * sizeof(ABRTSPV),
		   	      FALSE,
			      (STATUS *)NULL);
	newformals = FEreqmem(call_args->paramtag,
			      newsize * sizeof(char *),
			      FALSE,
			      (STATUS *)NULL);
	    	    
	newedvs = FEreqmem(call_args->paramtag,
			      newsize * sizeof(DB_EMBEDDED_DATA),
			      FALSE,
			      (STATUS *)NULL);
	    	    
	if (argcnt > 0)
	{
	    MEcopy((PTR)call_args->prm->pr_actuals,
		   (u_i2)(argcnt * sizeof(ABRTSPV)),
		   newactuals);

	    MEcopy((PTR)call_args->prm->pr_formals,
		   (u_i2)(argcnt * sizeof(char *)),
		   newformals);

	    MEcopy((PTR)call_args->param_edv,
		   (u_i2)(argcnt * sizeof(DB_EMBEDDED_DATA)),
		   newedvs);
	}
	call_args->prm->pr_actuals = (ABRTSPV *)newactuals;
	call_args->prm->pr_formals = (char **)newformals;
	call_args->param_edv = (DB_EMBEDDED_DATA *)newedvs;
	call_args->numalloc = newsize;
    }

    /* Now we're ready to add the parameter */
    call_args->prm->pr_formals[argcnt] = FEtsalloc(call_args->paramtag, name);
    abrtspv = call_args->prm->pr_actuals + argcnt;
    abrtspv->abpvtype = isbyref ? -DB_DBV_TYPE : DB_DBV_TYPE;
    argptr = (DB_DATA_VALUE *)FEreqmem(call_args->paramtag,
					  sizeof(DB_DATA_VALUE),
					  FALSE,
					  (STATUS *)NULL);
    MEcopy((PTR)&dbv, (u_i2)sizeof(DB_DATA_VALUE), (PTR)argptr);
    abrtspv->abpvvalue = (char *)argptr;
    datptr = FEreqmem(call_args->paramtag, dbv.db_length, 
					FALSE, (STATUS *)NULL);
    MEcopy(dbv.db_data, (u_i2)dbv.db_length, datptr);
    argptr->db_data = datptr;
    call_args->prm->pr_argcnt++;

    /* copy the edv over if this is a byref parameter */
    if (isbyref)
    {
        edv.ed_length = length;
	MEcopy( (PTR)&edv, (u_i2)sizeof(DB_EMBEDDED_DATA), 
		(PTR)&call_args->param_edv[argcnt]);
    }
    return OK;
}

