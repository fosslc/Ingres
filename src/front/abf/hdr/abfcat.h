/*
** Copyright (c) 2004 Ingres Corporation
*/
/**
** Name:	abfcat.h -	ABF Catalog definitions file.
**
** History:
**	23-mar-1992 (mgw) Bug #36595 a.k.a. 41188
**		Changed ABFILSIZE in abfrt.h and found that it was
**		referenced in 2 obsolete structures here, ABCATRW, and
**		ABCATGBF. Since these aren't used anywhere, delete them.
**		That led me to check other structures here and find they
**		should be yanked too.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
** Special type that is common to all objects that can be pointed
** to by a ABOBJ.
*/
typedef i4	ABTYPE;

/*
** BUG 1793
** Changing source directory didn't cause the locations for the sfile
** to be changed.  Now, sfiles don't have a location, they are built
** on the fly.  fisdir points to the source directory, and abfilscr
** builds a location of references to the source file.
**
** Files for frames and OSL procedures can not be shared with other
** objects.  They contain a pointer to the frame of procedure they
** belong to.
*/
typedef struct abfileType {
	ABTYPE		fitype;
	char		*fisdir;	/* The source dir for the file */
	char		*fisname;	/* The name of the source file */
	char		*finoext;	/* file name with no ext */
	LOCATION	fiofile;	/* The object file */
	char		*fiobuf;	/* The buffer for the object file */
	i4		firefcnt;	/* files are reference counted */
	struct abobjType	*fiobj;
} ABFILE;

/*}
** Name:    APPL -	The Application Structure.
**
** Description:
**	This structure contains all the information for an application.
**
** History:
**	??? (Joe)
**		Written
**	1-sep-1986 (Joe)
**		Moved information about about test image into
**		a structure that is ifdef for different kinds
**		of test images.
*/
typedef struct {
	ABTYPE		abtype;
	char		abname[FE_MAXNAME+1];	/* application's name */
	i2		abstate;	/* whether the app has been created */
	bool		abmodified;	/* app modified ? */
	DATE		abcrdate;	/* creation date */
	DATE		abmoddate;	/* modify date */
	i4		abqlang;
	ABLIST		*abfrmlist;	/* a list of all frames */
	ABLIST		*abfolist;	/* a list of all forms */
	ABLIST		*abfilelist;	/* a list of all files */
	ABLIST		*abproclist;	/* a list of all procs */
	char		abcreate[FE_MAXNAME+1];	/* creator's username */
	char		abscrdir[181];	/* path to source directory */
	char		abexname[FE_MAXNAME+1];
	char		abdefstart[FE_MAXNAME+1];
	char		aboptfil[FE_MAXNAME+1];
	i4		abversion;
	i4		aboid;		/* object manager id */
} APPL;
