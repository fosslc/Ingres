/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: JFLOCAL.H - Local JF hdr file.
**
** Description:
**	Definitions internal to the UNIX implementation of JF.
**
** History: $Log-for RCS$
**      14-jul-1987 (mmm)
**          Created new for jupiter.
**	17-may-90 (blaise)
**	    Integrated changes from ingresug:
**		Remove superfluous "typedef" before struct.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

/*
**  Forward and/or External typedef/struct references.
*/

/*  Types defined in this file. */

typedef struct _JF_FILE_HDR JF_FILE_HDR;


/*
**  Defines of constants.
*/

# define	JF_MIN_BLKSIZE		512	/* blocksize to use for the DI 
						** file that JF is built on 
						** top of 
						*/

/*}
** Name: JF_FILE_HDR - Header for a JF file.
**
** Description:
**      This structure describes the header found in the front of all JF files
**	on unix.  This header is used currently to maintain end of file
**	information similar to the type stored by the system on VMS. This 
**	extra header allows the UNIX version to maintain an end-of-file 
**	separate from actual physical writes.
**
**	The size of this structure should be JF_MIN_BLKSIZE.
**
** History:
**     14-jul-1987 (mmm)
**          Created new for jupiter
*/

struct _JF_FILE_HDR
{
    i4	   jff_ascii_id;	    /* visible identifier */
    i4	   jff_eof_blk;		    /* end of file - updated only by 
					    ** JFupdate 
					    */
    i4	   jff_blk_size;	    /* block size file created with */
    char	   jff_not_used[JF_MIN_BLKSIZE - sizeof(i4)];
};
