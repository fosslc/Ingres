/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

#ifndef OSSYM_H_INCLUDED
#define OSSYM_H_INCLUDED

/**
** Name:	ossym.h -	OSL Parser Symbol Table Definitions File.
**
** Description:
**	This file contains the definition of the structure of the symbol table
**	entries.  Entries in the symbol table are hashed on the symbol name.
**	(Manipulation of the hash table is private to ossym.c).
**	All fields and columns are hashed and lookup must be done based on the
**	name and the parent.  For example, given the form
**
**		emp
**			name = c20
**			salary = i2
**			child = table field
**				name = c20
**
**	The symbols "emp", "name", "salary", "child", and "name" are
**	all placed in the symbol table.  Obviously "name" from "emp" and
**	"name" from "child" would hash to the same location, so the lookup
**	is differentiated by their parents.
**
**	The symbols are linked into lists so that each parent points
**	to list(s) of its children, and each child points to its parent.
**	(Certain kinds of parents partition their children into disjoint
**	groups; such a parent has a separate list for each group).
**
**	Note that all lists of symbol table entries are maintained as stacks;
**	i.e. new entries are added to the beginning of the list, so the list
**	is in ascending order by age.  All such lists are null-terminated.
**
**	Not all symbols need be placed on a hash list.  (Temporaries aren't).
**
**	Every symbol that *is* placed on a hash list must have a parent.
**
** History:
**	Revision 6.4/03
**	09/20/92 (emerson)
**		Added s_tmpseq and OS_TMPFROZEN (for bug 34846).
**
**	Revision 6.4/02
**	10/03/91 (emerson)
**		Added s_block and removed OS_LABELACT flag from s_ref
**		(for bugs 34788, 36427, 40195, 40196).
**	11/07/91 (emerson)
**		Added s_tmpil (for bugs 39581, 41013, and 41014).
**
**	Revision 6.4
**	04/07/91 (emerson)
**		Revisions for local procedures.  Also took the opportunity
**		to simplify the structure (e.g. get rid of elaborate unions),
**		although there are still many #define's, for historical reasons.
**
**		Some noteworthy changes:
**		(1) An OSFORM symbol can now represent a local procedure
**		    as well as a frame's form or a main procedure.
**		    (The name OSFORM is misleading, but it's retained
**		    for historical reasons; the term "form" is used throughout
**		    the comments in the OSL directory to represent a "scope":
**		    a set of fields, variables, constants, etc. belonging
**		    to a frame or procedure).  In addition to "topological"
**		    information (lists of children), the OSFORM symbol now 
**		    contains information about the frame/procedure return type
**		    and some information used by ILG.
**		(2) The member s_brightness has been added.
**		(3) The member s_const has been removed (it was unused).
**		(4) The member s_consts has also been removed; application
**		    constants are now on the same list as variables.
**		(5) The size of a table entry has increased from 44 to 60 bytes
**		    (assuming 4-byte nat's and pointers and "natural" alignment)
**		    If this is a problem, we could split some of the OSFORM
**		    stuff off into separate entries, but this complicates things
**
**	Revision 6.0  87/01/24  wong
**	Moved attribute, table field info., etc. to OSNODE structure.
**	Added OSCONSTANT symbol node.
**
**	Revision 5.1  86/10/17  16:35:15  wong
**	Added subscript reference field.
**	Declare only symbol table functions.
**	Added reference for intermediate language.
**
**	Revision 3.0  84/04  joe
**	Initial revision.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**	24-Aug-2009 (kschendel) 121804
**	    Multiple inclusion protection.
**/

typedef struct _ossym	OSSYM;

struct _ossym 
{
	char	*s_name;	/* The symbol name.  Points into string table,
				** or points to empty string. Cannot be NULL */
	OSSYM	*s_next;	/* Next older synonym (if any) in hash chain */
	i2	s_kind;		/* Kind of symbol table entry.  See below */
	u_i2	s_ref;		/* Miscellaneous flags, including flags
				** indicating whether symbol has been referenced
				** in the current OSL statement.  See below */
	u_i2	s_flags;	/* Certain flags defined in fdesc.h
				** (FDF_RECORD, FDF_ARRAY, FDF_READONLY) */
	i2	s_brightness;	/* "Brightness" of symbol table entry.
				** See below */

	OSSYM	*s_parent;			/* Parent symbol (if any) */
	OSSYM	*s_sibling;			/* Next older sibling (if any)*/
	OSSYM	*s_children[4];			/* Up to 4 groups of children
						** (pointers to youngest child,
						** if any, in each group) */
#define		OS_MAX_GROUPS_CHILDREN	4
#define		s_attributes	s_children[0]	/* Attributes of a record */
#define		s_columns	s_children[0]	/* Columns of a table field */
						/* Children of a routine: */
#define		s_vars		s_children[0]	/* Variables; for top routine,
						** this includes globals and
						** application constants */
#define		s_subprocs	s_children[1]	/* Subordinate local procs */
#define		s_fields	s_children[2]	/* Visible simple fields */
#define		s_tables	s_children[3]	/* Table fields */
 
	DB_DATA_VALUE	s_dbdt;			/* Info about a variable (or
						** value returned by routine);
						** n.a. to labels */
#define		s_length	s_dbdt.db_length	/* size of variable */
#define		s_type		s_dbdt.db_datatype	/* DB_DT_ID types */
#define		s_scale		s_dbdt.db_prec		/* scale (decimal) */
#define		s_typename	s_dbdt.db_data		/* complex types */
	union
	{
		i2	sv_ilref;
		PTR	sv_block;
		PTR	sv_sid;
	}	sv_id;
#define		s_ilref		sv_id.sv_ilref	/* IL reference */
#define		s_block		sv_id.sv_block	/* -> info(private to osblock.c)
						** r.e. a labelled block */
#define		s_sid		sv_id.sv_sid	/* -> local procedure's IGSID */
	union
	{
		PTR	sv_rdesc;
		PTR	sv_tmpil;
		i4	sv_tmpseq;
	}	sv_ptr;
#define		s_rdesc		sv_ptr.sv_rdesc	/* -> local proc's routine
						** descriptor (private to ILG)*/
#define		s_tmpil		sv_ptr.sv_tmpil	/* applicable only to
						** *allocated* temps: NULL,
						** or -> last IL_ARRAYREF
						** or IL_DOT into the temp
						** (see ostmpall.c) */ 
#define		s_tmpseq	sv_ptr.sv_tmpseq /* applicable only to
						** temps in a free list:
						** sequence number indicating
						** when this temp was last
						** added to a free list. */
};

/*
** Symbol Object Types (s_kind field).
*/
#define	OSDUMMY		0	/* A dummy for constants, etc. */
#define	OSFORM		1	/* A form: the frame's form (a true form),
				** or the main procedure, or a local procedure*/
#define	OSTABLE		2	/* A table field */
#define	OSFIELD		3	/* A field */
#define	OSCOLUMN	4	/* A column in a table field */
#define	OSVAR		5	/* A declared variable (hidden field) */
#define	OSHIDCOL	6	/* A column in a table field */
#define OSCONSTANT	7	/* A constant definition -- really a literal */
#define	OSUNDEF		8	/* An undefined variable */
#define	OSTEMP		9	/* A temporary for expression analysis */
#define	OSLABEL		10	/* A label for a while loop, etc. */
#define	OSFRAME		11	/* A frame */
#define	OSPROC		12	/* A procedure */
#define OSGLOB          13      /* A global variable */
#define OSACONST        14      /* An application constant */
#define OSRATTR         15      /* A record attribute */

/*
** The values for the s_ref field are listed below, in 2 groups:
*/

/*
** s_ref flags indicating how a symbol has been referenced.
*/
#define	OS_NOTREF	0x0	/* Not referenced */
#define	OS_FLDREF	0x01	/* Regular field referenced */
#define	OS_TFLDREF	0x02	/* Entire table field referenced */
#define	OS_OBJREF	0x04	/* Frame or procedure referenced */
#define	OS_TFCOLREF	0x10	/* Specific column of a table field referenced*/
#define	OS_TFUNLOAD	0x20	/* Table field currently being unloadtabled */
#define	OS_PROCDEFINED	0x40	/* Local procedure has been defined */
#define	OS_TMPFROZEN	0x80	/* Temp is "frozen" (waiting to be freed).
				** s_tmpil/s_tmpseq is 0. See ostmpall.c */

#define OS_REFMASK	( OS_FLDREF | OS_TFLDREF | OS_TFCOLREF | OS_TFUNLOAD )

/*
** s_ref flag indicating that the symbol has "adopted" the children of all
** symbols marked as "bright" (see below).  Adopted children count as
** "children" of a parent symbol passed to ossympeek; see ossympeek for details.
** Note that the only symbol has this bit set is the top OSFORM symbol
** (representing the frame's form or the main procedure).
*/
#define	OS_ADOPT_CHILDREN_OF_BRIGHT	0x4000

/*
** Symbol "brightness" (s_brightness field).
** See ossymsearch for the usage of this field.
*/
#define	OS_DARK		0	/* non-OSFORM symbol */
#define	OS_DIM		1	/* OSFORM symbol that's *not* an ancestor
				** of the current form or procedure*/
#define	OS_BRIGHT	2	/* OSFORM symbol for the current form or
				** procedure, or one of its OSFORM ancestors */
#define	OS_EXTRA_BRIGHT	3	/* Special value used only by ossympeek */

/*
** declaration of return values of functions
*/
OSSYM	*ossymput(), *ossymfll();
OSSYM	*ossymlook(), *osfld(), *ostab();
OSSYM	*ossympeek();
OSSYM	*ossymsearch();
OSSYM	*ossymundef();
OSSYM	*osnodesym();
char	*osnodename();
OSSYM	*ostmpalloc();
OSSYM	*osdatalloc();
OSSYM	*osconstalloc();
OSSYM	*ostblcheck(), *osqryobjchk(), *osqryidchk();

#endif /* OSSYM_H_INCLUDED */
