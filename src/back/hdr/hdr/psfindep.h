#define	    PSFINDEP_H
/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: PSFINDEP.H - Independent object and privilege descriptors
**
** Description:
**      This file contains declarations of structures used to describe objects
**	and privileges on which objects and permits depend.  These structures
**	are being placed into a separate file because they will be referred to
**	by PSF, RDF, and QEF, and it makes no sense to have them all include
**	psfparse.h
**
** History:
**	17-jan-92 (andre)
**	    moved PSQ_OBJ, PSQ_OBJPRIV, PSQ_COLPRIV, and PSQ_INDEP_OBJECTS from
**	    psfparse.h
**	26-mar-93 (andre)
**	    defined PSQ_OBJTYPE_IS_CONSTRAINT
**	21-oct-93 (andre)
**	    defined PSQ_OBJTYPE_IS_SYNONYM
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	3-may-02 (inkdo01)
**	    defined PSQ_OBJTYPE_IS_SEQUENCE
*/

/*
** The structures defined below will be used to describe independent objects and
** object privileges on which a new object or privilege depend.
*/

					    /*
					    ** object is a table, view, or index
					    */
#define	    PSQ_OBJTYPE_IS_TABLE	    0x01

					    /* object is a database procedure */
#define	    PSQ_OBJTYPE_IS_DBPROC	    0x02

					    /* object is a dbevent */
#define	    PSQ_OBJTYPE_IS_DBEVENT	    0x04

					    /*
					    ** beginning of the list of
					    ** independent object or privilege
					    ** descriptors associated with a
					    ** dbproc
					    */
#define	    PSQ_DBPLIST_HEADER		    0x08

					    /*
					    ** beginning of the list of object
					    ** or privilege descriptors which
					    ** was created in the process of
					    ** reparsing a dbproc which turned
					    ** out to be non-grantable.
					    ** Elements of this list may still
					    ** contain useful information.
					    */
#define	    PSQ_INFOLIST_HEADER		    0x10

					    /*
					    ** only used with privilege
					    ** descriptors; indicates that the
					    ** current user lacks privileges
					    ** listed in the descriptor
					    **
					    ** This type is expected to occur
					    ** only in the lists with header of
					    ** type PSQ_INFOLIST_HEADER
					    */
#define	    PSQ_MISSING_PRIVILEGE	    0x20

					    /*
					    ** only used with object descriptors
					    ** when the object type is set to
					    ** PSQ_OBJTYPE_IS_DBPROC
					    **
					    ** This will be used to indicate
					    ** that a dbproc owned by the
					    ** current user is marked as
					    ** grantable in IIPROCEDURE.
					    */
#define	    PSQ_GRANTABLE_DBPROC	    0x40

					    /*
					    ** only used with object descriptors
					    ** when the object type is set to
					    ** PSQ_OBJTYPE_IS_DBPROC
					    **
					    ** This will be used to indicate
					    ** that a dbproc owned by the
					    ** current user is marked as active
					    ** in IIPROCEDURE
					    */
#define	    PSQ_ACTIVE_DBPROC		    0x80

					    /* object is a constraint */
#define	    PSQ_OBJTYPE_IS_CONSTRAINT	    0x0100

					    /* object is a synonym */
#define	    PSQ_OBJTYPE_IS_SYNONYM	    0x0200

					    /* object is a sequence */
#define	    PSQ_OBJTYPE_IS_SEQUENCE	    0x0400

/*
** Name: PSQ_OBJ - an element of a list of objects
**
** Description:
**	This structure defines an element of a list of objects; it consists of
**	object type, variable size list of object ids and a pointer to the next
**	element in the list.
**
** History:
**	11-sep-91 (andre)
**	    defined
**	04-oct-91 (andre)
**	    when processing GRANT ON PROCEDURE, I need a mechanism to mark an
**	    entry describing a dbproc owned by the current user as one of
**	    - an entry describing a grantable dbproc
**	    - an entry describing a nongrantable dbproc
**	      (knowing that a user cannot grant access on a certain dbproc can
**	      spare us the expense of reparsing it)
**	    - an entry marking beginning of the list of independent objects
**	      for a given dbproc
**	    - an entry marking a beginning of a list of object descriptors
**	      belonging to a dbproc which was deemed ungrantable (the list may
**	      still hold valuable information regarding dbprocs to which a user
**	      can/cannot grant access)
**	    psq_objtype which was originally intended to indicate whehter as
**	    given entry describes privileges on a dbproc or a table will be used
**	    for that purpose
**	10-oct-91 (andre)
**	    here is the problem: we have no RDF interface for obtaining dbproc
**	    information by dbpid abd dbpidx.  One solution would be to define
**	    the interface (this would require defining a new index on
**	    IIPROCEDURE.)
**	    However, given the time constraint introduced by my imminent
**	    departure, I will adopt a temporary solution: a DB_DBP_NAME
**	    structure will be added following the variable array of object ids.
**	    The idea is that for objects other than dbprocs, we will allocate
**	    memory necessary to accomodate the required number of IDs,
**	    overlaying all or part of the new structure member if necessary, but
**	    for dbprocs we will allocate the structure to accomodate one object
**	    id and the dbproc name (whetever you do, do not try to reference the
**	    dbproc name if !(psq_objtype & PSQ_OBJTYPE_IS_DBPROC))
**	26-mar-93 (andre)
**	    added psq_2id.  This will be used to contain secondary id of an
**	    independent object.  Initially, it will be used to contain integrity
**	    number of the independent UNIQUE/PRIMARY KEY constraint on which a
**	    REFERENCES constraint depends.  (at least for now) This new field
**	    will be meaningful only when
**	    (psq_objtype & PSQ_OBJTYPE_IS_CONSTRAINT)
*/
typedef struct _PSQ_OBJ
{
    struct _PSQ_OBJ	*psq_next;
    i4			psq_objtype;
    i4			psq_num_ids;	/* number of elements in psq_objid */

    /*
    ** this must be the last member of the structure as it is, in effect, a
    ** variable size array,
    ** except for when (psq_objtype & PSQ_OBJTYPE_IS_DBPROC) (see the 10-oct-91
    ** (andre) comment) and if
    ** (psq_objtype & (PSQ_DBPLIST_HEADER|PSQ_INFOLIST_HEADER)) where it is done
    ** to make life easier on code producing output for tracepoint ps133 and if
    ** (psq_objtype & PSQ_OBJTYPE_IS_CONSTRAINT) (see the 26-mar-93 (andre)
    ** comment)
    */
    DB_TAB_ID		psq_objid[1];
    DB_DBP_NAME		psq_dbp_name;
					/*
					** secondary id (as in "constraint
					** number")
					*/
    u_i4		psq_2id;
} PSQ_OBJ;

/*
** Name: PSQ_OBJPRIV - an element of a list of privileges on objects
**
** Description:
**	This structure defines an element of a list of privileges on objects
**	(these include EXECUTE privilege on a database procedure, REGISTER/RAISE
**	privileges on a dbevent, and any table-wide privilege on a table or
**	view);  it consists of object type, object id, privilege bit, and a
**	pointer to the next element in the list.
**
** History:
**	11-sep-91 (andre)
**	    defined
**	04-oct-91 (andre)
**	    when processing GRANT ON PROCEDURE, I need a mechanism to mark an
**	    entry as one of
**	    - an entry describing privileges which the user posesses
**	    - an entry describing privileges which a user does not posess
**	      (knowing that a user does not posess a certain privilege can spare
**	      us the expense of scanning IIPROTECT)
**	    - an entry marking beginning of the list of independent privileges
**	      for a given dbproc
**	    - an entry marking a beginning of a list of privilege descriptors
**	      belonging to a dbproc which was deemed ungrantable (the list may
**	      still hold valuable information regarding privileges which the
**	      current user posesses or does not posess)
**	    psq_objtype which was originally intended to indicate whether as
**	    given entry describes privileges on a dbproc or a table will be used
**	    for that purpose

*/
typedef struct _PSQ_OBJPRIV
{
    struct _PSQ_OBJPRIV     *psq_next;
    i4			    psq_objtype;
    i4			    psq_privmap;
    DB_TAB_ID		    psq_objid;
} PSQ_OBJPRIV;

/*
** Name: PSQ_COLPRIV - an element of a list of column-specific privileges
**
** Description:
**	This structure defines an element of a list of column-specific
**	privileges on tables and view; it consists of object type,privilege map,
**	map of attributes, object id and a pointer to the next element in the
**	list.
**
** History:
**	11-sep-91 (andre)
**	    defined
**	04-oct-91 (andre)
**	    added psq_objtype. While column-specific privileges can be defined
**	    only for tables, when processing GRANT ON PROCEDURE, I need a
**	    mechanism to mark an entry as one of
**	    - an entry describing privileges which the user posesses
**	    - an entry describing privileges which a user does not posess
**	      (knowing that a user does not posess a certain privilege can spare
**	      us the expense of scanning IIPROTECT)
**	    - an entry marking beginning of the list of independent privileges
**	      for a given dbproc
**	    - an entry marking a beginning of a list of privilege descriptors
**	      belonging to a dbproc which was deemed ungrantable (the list may
**	      still hold valuable information regarding privileges which the
**	      current user posesses or does not posess)
*/
typedef struct _PSQ_COLPRIV
{
    struct _PSQ_COLPRIV     *psq_next;
    i4			    psq_objtype;
    i4			    psq_privmap;
    DB_TAB_ID		    psq_tabid;
    i4			    psq_attrmap[DB_COL_WORDS];
} PSQ_COLPRIV;

/*
** Name: PSQ_INDEP_OBJECTS - description of independent objects and privileges
**			     on objects on which a new object or privilege
**			     depends
**
** Description:
**	This structure will be used to describe independent objects, privileges
**	on objects (EXECUTE privilege on database procedures, REGISTER/RAISE
**	privileges on dbevents, and table-wide privileges on tables and views),
**	and column-specific privileges on tables and views on which a new object
**	or privilege depend
**
** History:
**	11-sep-91 (andre)
**	    defined
**	29-may-92 (andre)
**	    added psq_grantee.  this will be copied into IIPRIV tuples.
**	    While in many cases QEF can get it from QEF_CB.qef_user, when
**	    building independent privilege descriptor for a recreated dbproc, we
**	    are not assured that the current DBMS sesion identifier is the same
**	    as the owner of the dbproc.
**	    
*/
typedef struct _PSQ_INDEP_OBJECTS
{
    PSQ_OBJ	    *psq_objs;
    PSQ_OBJPRIV	    *psq_objprivs;
    PSQ_COLPRIV	    *psq_colprivs;
    DB_OWN_NAME	    *psq_grantee;
} PSQ_INDEP_OBJECTS;
