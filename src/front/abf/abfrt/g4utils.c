/*
**	Copyright (c) 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<er.h>
# include	<me.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<adf.h>
# include	<fe.h>
# include       <erg4.h>
# include       "rts.h"
# include       "g4globs.h"
# include	<st.h>

/**
** Name:	g4utils.c - Utility routines
**
** Description:
**	This file defines:
**
**	IIAG4get_data 		copy data to program variables
**	IIAG4set_data		copy data from program variables
**	IIAG4dbvFromObject	Make a DB_DATA_VALUE for specified object.
**	IIAG4semSetErrMsg	push an error message onto the stack.
**	IIAG4iosInqObjectStack	Get info about Known Object stack (& intialize).
**	IIAG4fosFreeObjectStack	Free objects from Known Object stack.
**	IIAG4aoAddObject	Add a known object
**	IIAG4gkoGetKnownObject	Get a known object
**	
** History:
**	15-dec-92 (davel)
**		 Initial version, based on W4GL version developed by MikeS.
**	22-jan-93 (sandyd)
**		Changed sprintf() to STprintf() in IIAG4fosFreeObjectStack().
**	25-Aug-1993 (fredv)
**		Included <st.h>.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	25-Aug-2009 (kschendel) 121804
**	    Call iiarCcnClassName properly (ie with 3 args, not 2.)
**/

/* # define's */
/* GLOBALDEF's */
/* extern's */
FUNC_EXTERN	char	*IIUGfemFormatErrorMessage();
FUNC_EXTERN	void	IIUGber();
FUNC_EXTERN	STATUS	IIARoasObjAssign();
FUNC_EXTERN	void	iiarUsrErr();

/*{
** Name:	IIAG4get_data   copy data to program variables
**
** Description:
**	Copy data from a DB_DATA_VALUE to the user's program variables.
**	If the data is an object, we put it into the bag of known objects, 
**	and give the user the bag's index.
**
** Inputs:
**	dbdv		DB_DATA_VALUE *	Data to put
**      type            i4              ADF type of user variable
**      length          i4              data size of user variable
**
** Outputs:
**      ind             i2 *            NULL indicator
**      data            PTR             user variable pointer
**
**	Returns:
**		STATUS
**					OK
**					E_G4271B_DATA_CONVERSION_GET
**					E_G4271A_NO_NULL_INDICATOR
**
** History:
**	15-dec-92 (davel)
**		 Initial version, based on W4GL version developed by MikeS.
*/
STATUS
IIAG4get_data(DB_DATA_VALUE *dbdv, i2 *ind, i4  type, i4  length, PTR data)
{
    STATUS status = OK;
    DB_DATA_VALUE int_dbdv;
    DB_EMBEDDED_DATA edv;
    i4 objno;
    ADF_CB *cb = FEadfcb();

    /* 
    ** See if the data is an object
    */
    if (dbdv->db_datatype == DB_DMY_TYPE)
    {
	/* Yes.  Make an i4 DBDV */
	int_dbdv.db_datatype = DB_INT_TYPE;
	int_dbdv.db_length = sizeof(i4);
	int_dbdv.db_prec = 0;
	int_dbdv.db_data = (PTR)&objno;

	if ( (status = IIAG4aoAddObject(dbdv->db_data, &objno) ) != OK )
	{
	    /* convert any IIAG4aoAddObject error to the generic
	    ** E_G42729_GETDATA_ERROR, as IIAG4aoAddObject already raised
	    ** the specific error.
	    */
	    return E_G42729_GETDATA_ERROR;
	}
	dbdv = &int_dbdv;
    }

    /* Return the data */
    edv.ed_type = type;
    edv.ed_length = length;
    edv.ed_data = data;
    edv.ed_null = ind;

    if (adh_dbcvtev(cb, dbdv, &edv) != OK)
    {
	if (cb->adf_errcb.ad_errcode == E_AD1012_NULL_TO_NONNULL)
	    status = E_G4271A_NO_NULL_INDICATOR;
	else
	    status = E_G4271B_DATA_CONVERSION_GET;
    }

    return status;
}

/*{
** Name:	IIAG4set_data   copy data from program variables
**
** Description:
**	Copy data to a DB_DATA_VALUE from the user's program variables.
**	If the target is an object, we validate that the user has specified
**	a valid object handle, and that the types match.
**
**	Most of the error returns use the same set of arguments, which are
**	passed in as part of the G4ERRDEF argument:
**
**	arg[0] = caller name
**	arg[1] = type of data (constant, global, or attribute)
**	arg[2] = name of constant, global, or attribute
**
**	If further arguments are needed, they are filled in.  If a completely
**	differnet set of arguments are needed, we overwrite them here.  The
**	caller will not try to set arguments after calling this function.
**
** Inputs:
**	dbdv		DB_DATA_VALUE *	Data to put
**      ind             i2 *             NULL indicator
**	isvar		i4		Pased by reference?
**      type            i4              ADF type of user variable
**      length          i4              data size of user variable
**      data            PTR             user variable pointer
**
** Outputs:
**	g4errdef	G4ERRDEF *	Error descriptor
**	
**	Returns:
**		STATUS
**					OK
**					E_G4271C_DATA_CONVERSION_SET
**					E_G4271D_NULL_VALUE
**
** History:
**	15-dec-92 (davel)
**		 Initial version, based on W4GL version developed by MikeS.
*/
STATUS
IIAG4set_data
(DB_DATA_VALUE *dbdv, i2 *ind, i4  isvar, i4  type, i4  length, PTR data, 
G4ERRDEF *g4errdef)
{
    STATUS status = OK;
    DB_DATA_VALUE int_dbdv;
    DB_DATA_VALUE *target;
    DB_EMBEDDED_DATA edv;
    i4 objno;
    PTR rowptr;
    ADF_CB *cb = FEadfcb();

    /* 
    ** See if the 4GL target is an object
    */
    if (dbdv->db_datatype == DB_DMY_TYPE)
    {
	/* Yes.  Make an i4 DBDV for the user's specification to go into */
	int_dbdv.db_datatype = DB_INT_TYPE;
	int_dbdv.db_length = sizeof(i4);
	int_dbdv.db_prec = 0;
	int_dbdv.db_data = (PTR)&objno;

	target = &int_dbdv;
    }
    else
    {
	target = dbdv;
    }

    /* Get the data */
    edv.ed_type = type;
    edv.ed_length = length;
    edv.ed_data = isvar ? data : (PTR)&data;
    edv.ed_null = ind;

    if (adh_evcvtdb(cb, &edv, target) != OK)
    {
	if (cb->adf_errcb.ad_errcode == E_AD1012_NULL_TO_NONNULL)
	    status = E_G4271D_NULL_VALUE;
	else
	    status = E_G4271C_DATA_CONVERSION_SET;
    }

    /*
    ** If we got an object number, use it.
    */
    if (status == OK && target == &int_dbdv)
    {
	if (IIAG4gkoGetKnownObject(objno, &rowptr) != OK)
	{
	    g4errdef->numargs = 4;
	    g4errdef->args[3] = ERx("object");
    	    status = E_G4271E_BAD_OBJECT;
	}
	else
	{
	    DB_DATA_VALUE rdbv;

	    IIAG4dbvFromObject(rowptr, &rdbv);

	    /* an error in IIARoasObjAssign() is pretty much always a type
	    ** mismatch.
	    */
	    status = IIARoasObjAssign(&rdbv, dbdv);
	    if (status != OK)
	    {
		/* re-construct the error message.  leave args[0] alone,
		** and fetch the two record type names.
		*/
		AB_TYPENAME aname, rname;

		iiarCcnClassName(dbdv, aname, FALSE);
		iiarCcnClassName(&rdbv, rname, FALSE);
		g4errdef->numargs = 3;
		g4errdef->args[1] = (PTR)rname;
	        g4errdef->args[2] = (PTR)aname;

        	status = E_G42714_BADROWTYPE;
	    }
	}
    }
    return status;
}

/*{
** Name:        IIAG4dbvFromObject
**
** Description:
**      Fill a DB_DATA_VALUE, uising specified object pointer, to use in
**	calls to the abrtclas.c routines.
**
** Inputs:
**      object        PTR		the object.
**
** Outputs:
**	dbv	DB_DATA_VALUE *		a dbv which describes the object.
**
** History:
**      17-dec-92 (davel)
**		Initial version.
*/
VOID
IIAG4dbvFromObject( PTR object, DB_DATA_VALUE *dbv)
{
	dbv->db_datatype = DB_DMY_TYPE;
	dbv->db_data = object;
	dbv->db_length = 0;
	dbv->db_prec = 0;
}

/*{
** Name:        IIAG4semSetErrMsg
**
** Description:
**      Set an EXEC 4GL error message.  This involves:
**
**      1. Formatting it.
**      2. Saving the text and message number in globals, where INQUIRE_4GL
**         can find them.
**
** Inputs:
**      errparms        G4ERRDEF *      error parameters.
**      dump            bool            <not used >
**
** History:
**      15-dec-92 (davel)
**		Initial version, based on W4GL version developed by MikeS.
*/
VOID
IIAG4semSetErrMsg(G4ERRDEF *errparms, bool dump)
{
    char tmpbuf[ER_MAX_LEN];

    /* Format the message into iiAG4errtext */
    IIUGber(tmpbuf, sizeof(tmpbuf), errparms->errmsg, 0,
            errparms->numargs, errparms->args[0], errparms->args[1],
            errparms->args[2], errparms->args[3], errparms->args[4],
            errparms->args[5], errparms->args[6], errparms->args[7],
            errparms->args[8], errparms->args[9]);
    IIUGfemFormatErrorMessage(iiAG4errtext, sizeof(iiAG4errtext),
                              tmpbuf, FALSE);

    /* The text is already in place; save the error number */
    iiAG4errno = errparms->errmsg;

    /* put out the error if show-messages is turned on */
    if (iiAG4showMessages)
    {
    	iiarUsrErr(errparms->errmsg, errparms->numargs, 
	    errparms->args[0], errparms->args[1],
            errparms->args[2], errparms->args[3], errparms->args[4],
            errparms->args[5], errparms->args[6], errparms->args[7],
            errparms->args[8], errparms->args[9]);
    }
}

/* static's */
static	i4 alloc_stacksize = 0;
static	i4 stackpointer = 0;
static	PTR *objectstack = NULL;

# define G4_INITIAL_STACKSIZE	16

/*{
** Name:	IIAG4iosInqObjectStack - Inquire info about Known Object stack.
**
** Description:
**	This returns the current value of the static's used here for the
**	list of objects known by 3GL.  This function is used so that the caller
**	knows how many objects need to be freed (using IIAG4fosFreeObjectStack).
**	This is necessary, as 3GL can be re-entered recursively in ABF, as
**	3GL can call back into 4GL (using callproc), which can call another
**	3GL procedure.  We'll always want to free objects that were made known
**	on the last call to 3GL, once this procedure returns.
**
**	This also doubles as an initial stack allocator - if called when
**	current_alloc is zero, an initial known object stack will get
**	allocated.
**
** Inputs:
**	NONE.
**
** Outputs:
**	current_sp	{nat *}		current stackpointer.
**	current_alloc	{nat *}		current allocated stacksize.
**
** Returns:
**	OK, unless initialize fails; then FAIL.
**
** History:
**	15-dec-92 (davel)
**		 Initial version.
*/
STATUS
IIAG4iosInqObjectStack(i4 *current_sp, i4  *current_alloc)
{
	/* see if we should initialize stack */
	if (alloc_stacksize == 0)
	{
		u_i4 size = G4_INITIAL_STACKSIZE * sizeof(PTR);
		objectstack = (PTR *)MEreqmem(0, size, TRUE, NULL);
		if (objectstack == (PTR *)NULL)
		{
			return FAIL;
		}
		else
		{
			alloc_stacksize = G4_INITIAL_STACKSIZE;
		}
	}

	if (current_sp != NULL)
	{
		*current_sp = stackpointer;
	}
	if (current_alloc != NULL)
	{
		*current_alloc = alloc_stacksize;
	}
	return OK;
}

/*{
** Name:	IIAG4fosFreeObjectStack - Free objects from the Known Obj stack
**
** Description:
**	Pop the Known object stack back to the specified size, which must be
**	non-negative and not more than the current stack size.   This marks
**	the objects as "unknown".  Also, reference count information can
**	be decremented in the previously known objects so that ABF run-time can
**	potentially free up memory (this step is skipped for now).
**
** Inputs:
**	reset_sp	{nat}		value to reset stackpointer to.
**
** Outputs:
**	NONE.
**
** Returns:
**	OK 			- object stack reset
**	E_G42726_NO_INIT	- stack was never initialized.
**	E_G42728_BAD_STACK_POP	- invalid stackpointer value (beyond current).
**
** History:
**	15-dec-92 (davel)
**		 Initial version.
*/
STATUS
IIAG4fosFreeObjectStack(i4 reset_sp)
{
    G4ERRDEF g4errdef;

    if (alloc_stacksize == 0)
    {
	g4errdef.errmsg = E_G42726_NO_INIT;
	g4errdef.numargs = 0;
	IIAG4semSetErrMsg(&g4errdef, TRUE);
	return E_G42726_NO_INIT;
    }
    else if (reset_sp < 0 || reset_sp > stackpointer)
    {
	/* this should never happen.  Show the current and reset stackpointers
	** in the error message.
	*/
	char buf[64];

	g4errdef.errmsg = E_G42728_BAD_STACK_POP;
	g4errdef.numargs = 1;
	STprintf(buf, "%d -> %d", stackpointer, reset_sp);
	g4errdef.args[0] = (PTR)buf;
	IIAG4semSetErrMsg(&g4errdef, TRUE);
	return E_G42728_BAD_STACK_POP;
    }
    else 
    {
	/* XXX just reset the stack for now; no reference count decrementing */
	stackpointer = reset_sp;
	return OK;
    }
}

/*{
** Name:	IIAG4aoAddObject - Add a known object
**
** Description:
**	This adds another object to the list of objects known by 3GL.
**	If we don't ahve room in the current allocated stack of known
**	objects, then we allocate a new stack (twice the size of the old one),
**	and copy the old stack over.
**
**	3GL uses the index into this stack as a handle for the object.
**	A value of zero is the null object (although it's not clear that
**	ABF/4GL ever uses this concept).
**
** Inputs:
**	object		PTR 	object to add
**
**	Returns:
**		i4	The index into the stack of known objects.  This is 
**			used as an object handle by 3GL.
**
** History:
**	15-dec-92 (davel)
**		 Initial version, based on W4GL version developed by MikeS.
*/
STATUS
IIAG4aoAddObject(PTR object, i4 *objno)
{
    G4ERRDEF g4errdef;
    i4 i;

    /* even though ABF doesn't use the concept of NULL objects, we leave this
    ** in for now...
    */
    if (object == NULL)
    {
	*objno = 0;
	return OK;
    }

    /* make sure we've initialized the stack */
    if (alloc_stacksize == 0)
    {
	g4errdef.errmsg = E_G42726_NO_INIT;
	g4errdef.numargs = 0;
	IIAG4semSetErrMsg(&g4errdef, TRUE);
	return E_G42726_NO_INIT;
    }
    /* don't look for this in our stack - always re-issue a new object
    ** handle for it.
    */
    if (stackpointer >= alloc_stacksize)
    {
	u_i4 size = (u_i4) (alloc_stacksize * 2 * sizeof(PTR) );
	PTR *newstack;

	newstack = (PTR *)MEreqmem(0, size, TRUE, NULL);
	if (newstack == (PTR *)NULL)
	{
	    g4errdef.errmsg = E_G42727_MEM_ALLOC_FAIL;
	    g4errdef.numargs = 0;
	    IIAG4semSetErrMsg(&g4errdef, TRUE);
	    return E_G42727_MEM_ALLOC_FAIL;
	}
	else
	{
		/* copy the old stack over.  Note: don't use MEcopy, as
		** the stack size might be larger than 64K, the limit
		** on MEcopy.
		*/
		for (i = 0; i < alloc_stacksize; i++)
		{
			newstack[i] = objectstack[i];
		}
		(void) MEfree(objectstack);
		alloc_stacksize *= 2;
		objectstack = newstack;
	}
    }
    objectstack[stackpointer++] = object;
    *objno = stackpointer;

    /* XXX also increment the reference count for this saved object.  We'll
    ** decrement when the object gets freed in IIAG4fosFreeObjectStack().
    */
    return OK;
}

/*{
** Name:	IIAG4gkoGetKnownObject - Get a known object by its index number
**
** Description:
**	Validate the index number and make sure that the known object stack
**	has been initialized.  An index of zero means the "NULL object", which
**	ABF probably doesn't need.  Return the PTR
**	from the objectstack corresponding to the specified index.
**
** Inputs:
**	index	i4		object handle
**
** Outputs:
**	object	PTR *		object found (if OK is returned)
**
**	Returns:
**		STATUS
**		E_G4271E_BAD_OBJECT
**
** History:
**	15-dec-92 (davel)
**		 Initial version, based on W4GL version developed by MikeS.
*/
STATUS
IIAG4gkoGetKnownObject(i4 index, PTR *object)
{
    G4ERRDEF g4errdef;

    if (index == 0)
    {
	*object = (PTR)NULL;
	return OK;
    }
    else if (alloc_stacksize == 0)
    {
	g4errdef.errmsg = E_G42726_NO_INIT;
	g4errdef.numargs = 0;
	IIAG4semSetErrMsg(&g4errdef, TRUE);
	return E_G42726_NO_INIT;
    }
    else if (index < 0 || index > stackpointer)
    {
	return E_G4271E_BAD_OBJECT;
    }
    else 
    {
	*object = objectstack[index-1];
	return OK;
    }
}
