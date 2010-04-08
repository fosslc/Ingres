/*
**	Copyright (c) 2004 Ingres Corporation
**	All rights reserved.
*/

/**
** Name:	frmindex.h -	Form Index File Module Definitions File.
**
** Description:
**	Contains the definition of the structures that make up a Form Index
**	file and its class definition.  A Form Index file consist of an index
**	followed by a header and the encoded form structure for each form.
**
**	NOTE:  THE FORM INDEX FILE IS SYSTEM-DEPENDENT.  (Since
**	the encoded form structures are system-dependent.)
**
**	Defines:
**
**	FORM_INDEX	form index file class definition.
**	INDEX		form index file header and index table definition.
**	INDEX_HDR	form index file header definition.
**	INDEX_ENTRY	form index file index table entry definition.
**	FORM_HDR	form index file encoded form header.
**
** History:
**	Revision 6.1  88/03/30  wong
**	Initial revision of this interface.
**
**	Revision 6.0  87/09/10  peter
**	Change index page to contain size parameter as well for version "6.01".
**
**	Revision 5.0K  86/12/20  Kobayashi
**	Initial implementation for 5.0 Kanji.
**
**	11-apr-1989 (mgw)
**		Increased MAXINDEX to 170 to accomodate more forms in the
**		forms file.
**	04-jan-1990 (mgw)
**		Increased MAXINDEX to 284 to accomodate more forms in the
**		forms file.
**	04/16/91 (dkh) - Added support to handle different formindex
**			 file header sizes.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/

/* Latest version.
**
**	(A version change implies a change to the structures defined in this
**	this file that will be written to the file, which means the layout of
**	the file on disk changes.  Versions can be incompatible.)
*/
#define INDEX_VERSION  ERx("6.1")

/* Current maximum number of slots.
**
** (Note:  This can be set independently of the version.
** Currently set to about 10K worth of index slots.  See INDEX below.)
*/

#define MAXINDEX    284

/*}
** Name:	INDEX_HDR -	Index Header.
**
** Description:
**	The index header for the Form Index file.  Describes the version and
**	the size of the index table, which has at most MAXINDEX slots.
**
**	Note:  This structure will be written to disk as part of an INDEX.
*/
typedef struct {
	char	version[4];	/* version */
	u_i4	indexsize;	/* size of index table (<= MAXINDEX) */
} INDEX_HDR;

/*}
** Name:	INDEX_ENTRY -	Form Index Entry.
**
** Description:
**	The index entry for a single form in the index.  Lists the name
**	and offset in the Form Index file of the encoded form.
**
**	Note:  This structure will be written to disk as part of an INDEX.
*/
typedef struct {
	char	frmname[FE_MAXNAME];	/* form name */
	u_i4	offset;			/* offset in file */
} INDEX_ENTRY;

/*}
** Name:	INDEX -	Index for Form Index File.
**
** Description:
**	The index for the Form Index file, which consists of a header
**	describing the version and the size of the index table, and a
**	fixed size (MAXINDEX) index table of index entries for the forms.
**
**	Note:  This structure will be written to disk.
*/
typedef struct {
	INDEX_HDR	header;
	INDEX_ENTRY	itable[MAXINDEX];
	char		filler[8];	/* filler to (10K) page boundary */
} INDEX;

/*}
** Name:	FORM_INDEX -	Form Index File Class.
**
** Description:
**	The Form Index file module class.  Defines a Form Index file and
**	the methods used to access it.  (Internal only; not written.)
*/
typedef struct {
	FILE		*fp;		/* I/O reference for file */
	INDEX_ENTRY	*lastindex;	/* reference for last index */
	i4			fhdrsize;	/* size of encoded form header */
	u_i4		write : 1;	/* index written */
	u_i4		create : 1;	/* file was created */
	u_i4		allocated : 1;	/* structure was allocated */
/* public:  */
	char		fname[MAX_LOC + 1];	/* file name */
	INDEX		index;			/* index from file */
} FORM_INDEX;

/* methods: */
FORM_INDEX	*IIFDfiOpen(/* LOCATION iloc, bool create, bool read = FALSE */);
PTR			IIFDfiRead(/* FORM_INDEX *fip, char *form, i4 *frmsize */);
VOID		IIFDfiInsert(/* FORM_INDEX *fip, char *form, OOID id */);
VOID		IIFDfiDelete(/* FORM_INDEX *fip, char *form */);
VOID		IIFDfixReplace(/* FORM_INDEX *fip, char *form, OOID id */);
VOID		IIFDfiPrint(/* FORM_INDEX *fip */);
VOID		IIFDfiClose(/* FORM_INDEX *fip */);

/*}
** Name:	FORM_HDR -	Encoded Form Header.
**
** Description:
**	Header for an encoded form in the Form Index file.  The
**	encoded form should follow this header in the file.
**
**	Note:  This structure will be written to disk.
*/
typedef struct {
	char	frmname[FE_MAXNAME];	/* form name */
	u_i4	totsize;		/* encoded form size */
	char	date[25];		/* version date of form */
} FORM_HDR;
