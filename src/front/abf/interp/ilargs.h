/*
**Copyright (c) 1988, 2004 Ingres Corporation
*/

/*}
** Name:	ILARGS -	Structure to hold argument values.
**
** Description:
**	This structure holds the values of certain special command
**	line arguments. 
**
**	It currently contains the following command
**	line argument values:
**
**		database name
**		frame name to call first
**		executable file name
**		value for xflag (including leading -X)
**		value for -u flag (including leading -u)
**		value for +c flag (*not* including leading +c)
**		whether to enter in test mode
**		the debugging file to print output to.
**
**	It was created to allow the number of
**	arguments to grow without having to change the call
**	to get the arguments
**
** History:
**	19-mar-1990 (jhw)  Added 'ilstartframe'.
**	feb-1989 (bobm)  Added 'ilinfile' and 'ilstartproc'.
**	29-sep-1988 (Joe)
**	08/16/91 (emerson)
**		Added ilconnect (bug 37878).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
typedef struct
{
    char	*ildatabase;		/* Default value is NULL */
    char	*ilinfile;		/* Default value is NULL */
    char	*ilstartname;		/* Default value is NULL */
    char	*iluname;		/* Default value is "" */
    char	*ilxflag;		/* Default value is "" */
    i4		ilrunmode;		/* Default value is ILNRM... */
# define	ILNRMNORMALRUNMODE		1
# define	ILTRMTESTRUNMODE		2
    FILE	*ildbgfile;		/* Default value is stdout */
    bool	ilstartframe;		/* default value is FALSE */
    bool	ilstartproc;		/* default value is FALSE */
    char	*ilconnect;		/* Default value is "" */
} ILARGS;
