/*
** History:
**	17-Jan-92	(francel)
**		Terminal I/O portability changes for HP
**	29-apr-92	(purusho)
**		Added some amd_us5 specific entries
**      30-dec-92       (rkumar)
**             Define _ECHO, _NOECHO etc. for sqs_ptx  These are used in
**             accompat.c.
**	21-May-1997 (allmi01)
**		added support for sos_us5 (x-integ from 1.2/01)
**	10-may-1999 (walro03)
**		Remove obsolete version string amd_us5.
*/

     struct tchars {
	char    t_intrc;        /* interrupt */
	char    t_quitc;        /* quit */
	char    t_startc;       /* start output */
	char    t_stopc;        /* stop output */
	char    t_eofc;         /* end-of-file */
	char    t_brkc;         /* input delimiter (like nl) */
	};



#ifdef sos_us5
#define TIOCNOTTY       (tIOC|113)            /* void tty association */
#else /* not sos_us5 */
#ifdef sqs_ptx
#define _ECHO           0x1
#define _NOECHO         0x2
#define _DRAIN          0x80            /* like a TIOCSETN call */
#else /* none of the above */
#define TIOCSETC       _IOW('t',17,struct tchars)/* set special characters */
#define TIOCSETN       _IOW('t',10,struct sgttyb)/* as above, but no flushtty */
#define TIOCGETC       _IOR('t',18,struct tchars)/* get special characters */
#define TIOCNOTTY      _IO('t', 113)             /* void tty association */
#endif /* sqs_ptx */
#endif /* sos_us5 */
