/*
**      Copyright (c) 2004 Ingres Corporation
*/

# include       <compat.h>
# include       <er.h>
# include       <gl.h>
# include       <sl.h>
# include       <iicommon.h>
# include       <fe.h>
# include       <erg4.h>
# include       "rts.h"
# include       "g4globs.h"


/**
** Name:        g4array.c - EXEC 4GL array statements
**
** Description:
**      These are the array manipulation routines for EXEC 4GL support in
**      ABF.  Most are straightforward calls to abrtclas.c routines.
**
**      This file defines:
**
**      IIAG4acArrayClear               Clear an array
**      IIAG4drDelRow                   Set an array row deleted.
**      IIAG4grGetRow                   Get a row from an array
**      IIAG4irInsRow                   Insert a row into an array
**      IIAG4rrRemRow                   Remove a row from an array
**      IIAG4srSetRow                   Set a row in an array.
**
** History:
**      18-dec-92 (davel)
**              Initial version, based on W4GL version developed by MikeS.
**      22-nov-93 (donc,davel)
**              Use index-1 for position for index into array
**      08-nov-94 (lawst01)
**              in function 'bad_index' the 'index' argument should be
**              passed as the address of (&).
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
FUNC_EXTERN     STATUS  iiarArClearArray();
FUNC_EXTERN     STATUS  IIARardArrayDelete();
FUNC_EXTERN     STATUS  IIARariArrayInsert();
FUNC_EXTERN     STATUS  iiarArxRemoveRow();
FUNC_EXTERN     STATUS  IIARoasObjAssign();

/* static's */
static VOID     bad_index(i4 caller, i4 index);

/*{
** Name:        IIAG4acArrayClear - Clear an array
**
** Description:
**      Clear an array.  This is the 
**      
**              EXEC 4GL CLEAR ARRAY
**      
**      statement.
**
** Inputs:
**      array   i4      handle for the array to clear.
**
**      Returns:
**              STATUS
**
** History:
**      18-dec-92 (davel)
**              Initial version, based on W4GL version developed by MikeS.
*/
STATUS
IIAG4acArrayClear(i4 array)
{
    STATUS status;
    DB_DATA_VALUE dbv;

    CLEAR_ERROR;

    /* See if the handle is valid */
    status = IIAG4chkobj(array, G4OA_ARRAY, 0, G4CLRARR_ID);

    if (status == OK)
    {
        IIAG4dbvFromObject(iiAG4savedObject, &dbv);
        (void) iiarArClearArray(&dbv);
    }

    return status;
}

/*{
** Name:        IIAG4drDelRow - Set an array row deleted
**
** Description:
**      This is the EXEC 4GL SETROW DELETED statement
**
** Inputs:
**      array           i4      handle for array.
**      index           i4      index of row to set deleted.
**
**      Returns:
**              STATUS
**
** History:
**      18-dec-92 (davel)
**              Initial version, based on W4GL version developed by MikeS.
*/
STATUS
IIAG4drDelRow(i4 array, i4  index)
{
    STATUS status;

    CLEAR_ERROR;

    status = IIAG4chkobj(array, G4OA_ARRAY, 0, G4DELROW_ID);
    if (status == OK)
    {
        i4 lindex = index;
        DB_DATA_VALUE dbv;

        IIAG4dbvFromObject(iiAG4savedObject, &dbv);
        /* XXX unfortunately, if IIARardArrayDelete enounters a bad index, it
        ** XXX will raise an error itself.  NO way yet to squelch that...
        */
        status = IIARardArrayDelete(&dbv, lindex);
        if (status != OK)
        {
            /* The only error we expect to see from here is an invalid index. */
            bad_index(G4DELROW_ID, lindex);
        }
    }

    return status;
}

/*{
** Name:        IIAG4grGetRow - Get a row from an array
**
** Description:
**      EXEC 4GL GETROW
**
**      If we succeed in getting a row from the array, we must put it
**      into our bag of known objects and bump the reference count.
**
** Inputs:
**      rowind          i2 *    NULL indicator for row (ignored)
**      isvar           i4      Passed by reference (ignored)
**      rowtype         i4      data type for row (DB_INT_TYPE)
**      rowlen          i4      data size for row (sizeof i4)
**      array           i4      array handle
**      index           i4      array index
**
** Outputs:
**      rowp            (PTR)   array row which we got
**
**      Returns:
**              STATUS
**
** History:
**      18-dec-92 (davel)
**              Initial version, based on W4GL version developed by MikeS.
**              Note: this isn't supposed to be supported for ABF! Try it
**              for now....
*/
STATUS
IIAG4grGetRow
(i2 *rowind, i4  isvar, i4  rowtype, i4  rowlen, PTR rowp, i4 array, i4  index)
{
    G4ERRDEF g4errdef;
    STATUS status;
    i4  *row = (i4 *)rowp;

    CLEAR_ERROR;

    status = IIAG4chkobj(array, -G4OA_ROW, index, G4GETROW_ID);
    if (status == OK)
    {
        /* We have the object; return it to the user */
        if ( (status = IIAG4aoAddObject(iiAG4savedObject, row) ) != OK)
        {
            /* IIAG4aoAddObject() will have raised the specific error */
            g4errdef.errmsg = E_G42724_PREVIOUS_ERRS;
            g4errdef.numargs = 1;
            g4errdef.args[0] = (PTR)iiAG4routineNames[G4GETROW_ID];
            IIAG4semSetErrMsg(&g4errdef, TRUE);
        }
    }
    return status;
}

/*{
** Name:        IIAG4irInsRow - Insert a row into an array.
**
** Description:
**      EXEC 4GL INSERTROW
**
**      Insert a row into an array.
**
** Inputs:
**      array           i4      Array handle
**      index           i4      index to insert row before.
**      row             i4      object to insert
**      state           i4      state for new row.
**      which           i4      which of the following are specified
**
**      Returns:
**              STATUS
**
** History:
**      18-dec-92 (davel)
**              Initial version, based on W4GL version developed by MikeS.
*/
STATUS
IIAG4irInsRow(i4 array, i4  index, i4 row, i4  state, i4  which)
{
    G4ERRDEF g4errdef;

    STATUS status;
    i4 lindex = index;
    DB_DATA_VALUE dbv;

    CLEAR_ERROR;

    /* disallow if ROW or STATE is specified */
    if (which & (G4IR_ROW|G4IR_STATE) )
    {
        g4errdef.errmsg = E_G4272A_BAD_INSERTROW_OPTS;
        g4errdef.numargs = 1;
        g4errdef.args[0] = ERx("ABF/Vision");
        IIAG4semSetErrMsg(&g4errdef, TRUE);
        return E_G4272A_BAD_INSERTROW_OPTS;
    }

    /* First, check the array */
    status = IIAG4chkobj(array, G4OA_ARRAY, 0, G4INSROW_ID);
    if (status != OK)
        return status;

    IIAG4dbvFromObject(iiAG4savedObject, &dbv);
    /* XXX unfortunately, if IIARariArrayInsert enounters a bad index, it
    ** XXX will raise an error itself.  No way yet to squelch that...
    */
    status = IIARariArrayInsert(&dbv, lindex-1);
    if (status != OK)
    {
        bad_index(G4INSROW_ID, lindex);
    }
    return status;
}

/*{
** Name:        IIAG4rrRemRow  Remove a row from an array
**
** Description:
**      EXEC 4GL REMOVEROW
**
** Inputs:
**      array           i4      array handle
**      index           i4      array index     
**
** History:
**      18-dec-92 (davel)
**              Initial version, based on W4GL version developed by MikeS.
*/
STATUS
IIAG4rrRemRow(i4 array, i4  index)
{
    STATUS status;

    CLEAR_ERROR;

    status = IIAG4chkobj(array, G4OA_ARRAY, 0, G4REMROW_ID);
    if (status == OK)
    {
        i4 lindex = index;
        DB_DATA_VALUE dbv;

        IIAG4dbvFromObject(iiAG4savedObject, &dbv);
        /* XXX unfortunately, if iiarArxRemoveRow enounters a bad index, it
        ** XXX will raise an error itself.  NO way yet to squelch that...
        */
        status = iiarArxRemoveRow(&dbv, lindex);
        if (status != OK)
        {
            /* The only error we expect to see from here is an invalid index. */
            bad_index(G4REMROW_ID, lindex);
        }
    }

    return status;
}

/*{
** Name:        IIAG4srSetRow                   Set a row in an array.
**
** Description:
**      EXEC 4GL SETROW.  
**
** Inputs:
**      array   i4      array handle
**      index   i4      index
**      row     i4      Object to put into array
**
**      Returns:
**              STATUS
**
** History:
**      18-dec-92 (davel)
**              Initial version, based on W4GL version developed by MikeS.
*/
STATUS
IIAG4srSetRow(i4 array, i4  index, i4 row)
{
    G4ERRDEF g4errdef;
    i4 lindex = index;
    i2 *stateptr;
    STATUS status;
    DB_DATA_VALUE adbv, rdbv;

    CLEAR_ERROR;

    /* First, check the row of the array */
    status = IIAG4chkobj(array, G4OA_ROW, index, G4SETROW_ID);
    if (status != OK)
        return status;

    IIAG4dbvFromObject(iiAG4savedObject, &adbv);

    /* Now check row type vs. the object type */
    status = IIAG4chkobj(row, G4OA_OBJECT, 0, G4SETROW_ID);
    if (status != OK)
    {
        return status;
    }
    IIAG4dbvFromObject(iiAG4savedObject, &rdbv);

    /* go for broke: try to do the row assignment.  The most common reason
    ** for this to fail is that the types don't match; IIARoasObjAssign
    ** returns FAIL for any failure, so for now assume that this
    ** means incompatible records.
    */
    status = IIARoasObjAssign(&rdbv, &adbv);
    if (status != OK)
    {
        AB_TYPENAME aname, rname;

        iiarCcnClassName(&adbv, aname, FALSE);
        iiarCcnClassName(&rdbv, rname, FALSE);
        g4errdef.errmsg = E_G42714_BADROWTYPE;
        g4errdef.numargs = 3;
        g4errdef.args[0] = (PTR)iiAG4routineNames[G4SETROW_ID];
        g4errdef.args[1] = (PTR)rname;
        g4errdef.args[2] = (PTR)aname;
        IIAG4semSetErrMsg(&g4errdef, TRUE);
        return E_G42714_BADROWTYPE;
    }
    return OK;
}

/*
** Report a 'bad index' error
*/
static VOID
bad_index(i4 caller, i4 index)
{       
    G4ERRDEF g4errdef;

    g4errdef.errmsg = E_G42713_BADINDEX;
    g4errdef.numargs = 2;
    g4errdef.args[0] = (PTR)iiAG4routineNames[caller];
/*  g4errdef.args[1] = (PTR)index;              08-nov-94 #60912        */
    g4errdef.args[1] = (PTR)&index;     /*      08-nov-94 #60912        */
    IIAG4semSetErrMsg(&g4errdef, TRUE);
}
