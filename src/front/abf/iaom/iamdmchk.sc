/*
**	Copyright (c) 1990, 2004 Ingres Corporation
*/

# include	<compat.h>
# include	<tm.h>
# include	<st.h>
# include	<er.h>
# include	<gl.h>
# include	<sl.h>
# include	<iicommon.h>
# include	<adf.h>
# include	<fe.h>
# include	<afe.h>
EXEC SQL INCLUDE <ui.sh>;
EXEC SQL INCLUDE <oocat.sh>;
# include	<dmchecks.h>
EXEC SQL INCLUDE 'iamdmtyp.sh';
# include	"eram.h"
# include	"abproc.h"


/**
** Name:	iamdmchk.sc - DM traversal routines
**
** Description:
**	These are the routines called while traversing a graph.  They have
**	a common calling sequence:
*
**	applid		OOID		Application object ID
**
**	relation	DM_REL *	Relation between frame and invalidator
**
**	arg1		i4		Argument always passed to this 
**					travesal routine.  It is specified in 
**					iamdmat.roc, in the structure which
**					also specifies the traversal routine.
**
**	arg		PTR		Argument passed from the routine
**					which called IIAMxdsDepSearch or
**					iiAMxsgSearchGraph to cause the 
**					traversal.
**
**	The routine is called for each traversal across a relation from entity
**	to entity.  It can return one of the following values:
**	
**			OK	Continue the traversal.
**			-1	End the traversal
**			-2	Continue traversiong the current source entity,
**				but don't recurse from the target entity.
**
**	Only iiAMxsgSearchGraph sees these returns; to return status to the 
**	original caller, set a flag inside the "arg" structure.
**
**	Each routine knows whether it traverses from primary to secondary or
**	visa versa.  
**
** Contains:
** 	IIAMdx1Disallow 	- Don't allow something to happen
** 	IIAMdx2Invalidate 	- Invalidate a frame
** 	IIAMdx4CompCheck 	- Recompile a frame
** 	IIAMdx5FormDateCheck 	- Check form date vs. frame date
** 	IIAMdx6TableDateCheck 	- Check table date vs. frame date
** 	IIAMdx7RecordName 	- Check for recursive use of record
*
** History:
**	7/90 (Mike S) Initial version
**	1/91 (Mike S) Renamed from iamtrvrtn.sc to iamdmchk.sc
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	25-Aug-2009 (kschendel) 121804
**	    Need abproc.h to satisfy gcc 4.3.
**/

/* # define's */
/* GLOBALDEF's */
/* extern's */
/* static's */

/*{
** Name:	IIAMdx1Disallow - Don't allow something to happen
**
** Description:
**	We notice that some the existence of a dependency forbids something
**	from happening; usually, that some object is "in use" and can't be
**	deleted.  We issue a message to that effect.
**
** Inputs:
**	applid		OOID		Application object ID
**	relation	DM_REL *	Relation between object we can't delete
**					(primary) and object using it 
**					(secondary)
**	msg		i4		Message to issue.
**
** Outputs:
**	arg		PTR		Flag to set 
**					(actually a bool *)
**
**	Returns:
**		STATUS		Always -1; we don't need two reasons not to do
**				something.
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	7/90 (Mike S)	Initial version
*/
/*ARGSUSED*/
STATUS
IIAMdx1Disallow(applid, relation, msg, arg)
OOID    applid;
DM_REL  *relation;
i4 msg;
PTR	arg;
{
	/* Issue a message that we can't do it */
	IIUGerr((ER_MSGID)msg, 0, 2, relation->s_ent->name, 
		relation->p_ent->name);

	/* Set a flag saying "you can't do that" */
	*(bool *)arg = TRUE;

	/* We can stop searching now */
	return -1;
}


/*{
** Name:	IIAMdx7RecordName - Check for recursive use of record
**
** Description:
**	Our argument the name of a record which our caller is proposing to add 
**	as an attribute to a second record.  We are searching to see if the 
**	proposed attribute (recursively) contains this second record.  If 
**	it does, we'll diasallow the addition.
**
** Inputs:
**	applid		OOID		Application ID
**	relation	DM_REL *	Relation between a record (primary)
**					and another record which it contains
**					(secondary)
**	dummy		i4		ignored
**	arg		PTR		from orignal caller
**					actually a (DM_RACHECK_ARG *)
**
** Outputs:
**
**	Returns:
**		STATUS		-1 if we have a match
**
**	Exceptions:
**
** Side Effects:
**
** History:
**	7/90 (Mike S)	Initial version
*/
/*ARGSUSED*/
STATUS
IIAMdx7RecordName(applid, relation, dummy, arg)
OOID	applid;
DM_REL	*relation;
i4	dummy;
PTR	arg;
{
	DM_RACHECK_ARG *ra_arg = (DM_RACHECK_ARG *)arg;

	if (STequal(ra_arg->attr_type, relation->p_ent->name))
	{
		/* A match!  We can't let this happen. */
		ra_arg->self = TRUE;
		return -1;
	}

	return OK;
}


/*{
** Name:	IIAMdx5FormDateCheck - Check form date vs. frame date
**
** Description:
**	Check if the alter date on a form forces a frame which uses it to be
**	recompiled.
**
** Inputs:
**	applid		OOID		Application ID
**	relation	DM_REL *	Relation between frame (primary)
**					and form (secondary)
**	dummy		i4		ignored
**	arg		PTR		from orignal caller
**					Actually a (DM_DTCHECK_ARG *)
**
** Outputs:
**	none
**
**	Returns:
**		-1	If frame must be recompiled
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	7/90 (Mike S) Initial version
*/
/*ARGSUSED*/
STATUS
IIAMdx5FormDateCheck(applid, relation, dummy, arg)
OOID	applid;
DM_REL	*relation;
i4	dummy;
PTR	arg;
{
	DM_DTCHECK_ARG *dt_arg = (DM_DTCHECK_ARG *)arg;
	char dbuf[OODATESIZE+1];
	OOID	dummyid;
	bool	doit = FALSE;
	SYSTIME	formtime;

	/* 
	** Get the form's date.  If anything goes wrong, we'd better recompile
	** in case the form doesn't exist.
	*/
	if (IIAMfoiFormInfo(relation->s_ent->name, dbuf, &dummyid) != OK ||
	    dbuf[0] == EOS)
	{
		/* We couldn't find the form, or it doesn't exist */
		doit = TRUE;
	}
	else 
	{
		/* Recompile if the frame is older than the form */
		IIUGdtsDateToSys(dbuf, &formtime);
		if (TMcmp(dt_arg->frame_time, &formtime) < 0)
		doit = TRUE;
	}

	if (doit)
	{
		/* Recompile, and explain why */
		dt_arg->recompile = TRUE;
		dt_arg->msg = F_AM0022_FormChange;
		dt_arg->name = relation->s_ent->name;
		return -1;		/* No need to look further */
	}
	return OK;
}

/*{
** Name:	IIAMdx6TableDateCheck - Check table date vs. frame date
**
** Description:
**	Check if the alter date on a table forces a frame which uses it to be
**	recompiled.
**
** Inputs:
**	applid		OOID		Application ID
**	relation	DM_REL *	Relation between frame (primary)
**					and table (secondary)
**	dummy		i4		ignored
**	arg		PTR		from orignal caller
**					Actually a (DM_DTCHECK_ARG *)
**
** Outputs:
**	none
**
**	Returns:
**		-1	If frame must be recompiled
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	7/90 (Mike S) Initial version
*/
/*ARGSUSED*/
STATUS
IIAMdx6TableDateCheck(applid, relation, dummy, arg)
OOID	applid;
DM_REL	*relation;
i4	dummy;
PTR	arg;
{
	DM_DTCHECK_ARG *dt_arg = (DM_DTCHECK_ARG *)arg;
	FE_REL_INFO info;
	char	datebuf[AFE_DATESIZE+1];
	bool	doit = FALSE;
	SYSTIME	tabtime;

	/* 
	** Get the table's date.  If anything goes wrong, we'd better recompile
	** in case the table doesn't exist.
	*/
	if (FErel_ffetch(relation->s_ent->name, FALSE, &info) != OK)
	{
		/* We couldn't find the table, or it doesn't exist */
		doit = TRUE;
	}
	else 
	{
		/* Recompile if the table is older than the form */
		UGdt_to_cat(&info.alter_date, datebuf);
		if (datebuf[0] == EOS)
			UGdt_to_cat(&info.create_date, datebuf);
		IIUGdtsDateToSys(datebuf, &tabtime);
		if (TMcmp(dt_arg->frame_time, &tabtime) < 0)
		doit = TRUE;
	}

	if (doit)
	{
		/* Recompile, and explain why */
		dt_arg->recompile = TRUE;
		dt_arg->msg = F_AM0023_TableChange;
		dt_arg->name = relation->s_ent->name;
		return -1;		/* No need to look further */
	}
	return OK;
}



/*{
** Name:	IIAMdx4CompCheck - Recompile a frame
**
** Description:
**	We're traversing the call dependencies of a frame.  When we find a
**	callee who hasn't been recompiled yet, we recompile it.  If successful,
**	we update its dependencies, and start recompiling its callees.
**
** Inputs:
**	applid		OOID		Application ID
**	relation	DM_REL *	Relation between caller (primary)
**					and callee (secondary)
**	dummy		i4		ignored
**	arg		PTR		from original calling routine
**
** Outputs:
**	none
**
**	Returns:
**		OK	If we want to traverse the callee's dependencies	
**		-2 	Otherwise (i.e. if the callee can't have callees )
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	8/90 (Mike S) Initial Version
*/
/*ARGSUSED*/
STATUS
IIAMdx4CompCheck(applid, relation, dummy, arg)
OOID	applid;
DM_REL	*relation;
i4	dummy;
PTR	arg;
{
	APPL_COMP *comp;
	DM_COMPTREE_ARG *ctarg = (DM_COMPTREE_ARG *)arg;
	STATUS status;
	STATUS rval = -2;	/* Assume no recursion from here */

	if ((comp = relation->s_ent->comp) == NULL)
		return -2;	/* No component */

	switch (comp->class)
	{
            case OC_OSLFRAME:
            case OC_OLDOSLFRAME:
            case OC_MUFRAME:
            case OC_APPFRAME:
            case OC_UPDFRAME:
            case OC_BRWFRAME:
            case OC_OSLPROC:
		/* Try to compile it */
		status =  (*(ctarg->compile)) (comp, ctarg->tstlnk);
		if (status != OK)
			ctarg->rval = status;

		rval = OK;	/* Try this one's callees */
		break;

            case OC_HLPROC:
		/* 
		** We attempt recompilation of all 3GL procs at the end of a
		** test build; there's no need to compile it here.
		*/
		break;
	}

	return rval;
}

/*{
** Name:	IIAMdx2Invalidate - Invalidate a frame
**
** Description:
**	Invalidate a frame to force recompilation.  Write the change to the
**	database.
**
** Inputs:
**	applid		OOID		applid
**	relation	DM_REL *	Relation between frame (primary)
**					and invalidator (secondary)
**	flag		i4		flag bit to set
**	arg		PTR		from orignal caller (ignored)
**
** Outputs:
**	none
**
**	Returns:
**		Status from DB operation
**
**	Exceptions:
**		none
**
** Side Effects:
**
** History:
**	7/90 (Mike S) Initial version	
*/
/*ARGSUSED*/
STATUS
IIAMdx2Invalidate(applid, relation, iflag, arg)
EXEC SQL BEGIN DECLARE SECTION;
OOID	applid;
DM_REL	*relation;
EXEC SQL END DECLARE SECTION;
i4 iflag;
PTR	arg;
{
EXEC SQL BEGIN DECLARE SECTION;
	OOID	objid;
	i4 flags;
	char	*name;		/* Frame's name */
	APPL_COMP *comp;	/* Frame to invalidate */
	i2	flags_ind;
EXEC SQL END DECLARE SECTION;
	STATUS status;

	name = relation->p_ent->name;
	comp = relation->p_ent->comp;

	/* 
	** If component is in memory, invalidate it in memory.  In that case
	** we can look up the component by object_id. If it's already 
	** invalid, we can quit now.
	*/
	if (comp != NULL)
	{
		if ((comp->flags & APC_RECOMPILE) != 0)
			return OK;

		comp->flags |= (APC_RECOMPILE|iflag);
		objid = comp->ooid;
	}
	else
	{
		/* Get the object_id */
		objid = iiamDbGlobalName( (OO_CLASS *)NULL, name, (char *)NULL,
					  applid);
		if (objid == nil)
			return OK;	/* No such object */
	}
	
	/* Get the flags word from the DB */
	IIUIbeginXaction();
	EXEC SQL REPEAT SELECT 
		abf_flags
		INTO :flags:flags_ind
		FROM ii_objects o, ii_abfobjects a
		WHERE o.object_id = a.object_id AND
			a.object_id = :objid AND
			a.applid = :applid;

	/* 
	** Quit now if :
	** 1. The select failed,
	** 2. We didn't retrieve a row, or
	** 3. It already needs recompilation.
	*/
	if ((status = FEinqerr()) != OK ||
	    FEinqrows() <= 0 ||
	    (flags & APC_RECOMPILE) != 0)
	{
		IIUIendXaction();		
		return OK;
	}

	/* Update it in the DB */
	if (flags_ind == DB_EMB_NULL)
		flags = 0;
	iiuicw1_CatWriteOn();
	flags |= (APC_RECOMPILE|iflag);
	EXEC SQL UPDATE ii_abfobjects	
	SET abf_flags = :flags
	WHERE object_id = :objid AND
	      applid = :applid;
	iiuicw0_CatWriteOff();

	status = FEinqerr();
	IIUIendXaction();		
	return OK;
}

