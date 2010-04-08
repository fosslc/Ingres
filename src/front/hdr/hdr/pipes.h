/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
**  PIPES.H -- definitions for pipe blocks.
*/

# ifndef PB_DBSIZE

/*
**  The 'pb_t' (pipe block type) should be arranged so that the
**  size of the structure excluding pb_xptr is some nice power
**  of two.
*/

# define	PB_DBSIZE	244
# define	PB_IOSIZE	256

typedef struct _pb_t
{
	char	pb_st;		/* the state to enter */
	char	pb_proc;	/* the proc to enter */
	char	pb_resp;	/* the proc to respond to */
	char	pb_padxx;	/* --- unused at this time --- */
	char	pb_from;	/* the immediate writer of this block */
	char	pb_type;	/* the block type, see below */
	i2	pb_stat;	/* a status word, see below */
	i2	pb_nused;	/* the number of bytes used in this block */
	i2	pb_nleft;	/* the number of bytes left in this block */
	char	pb_data[PB_DBSIZE];	/* the data area */
	char	*pb_xptr;	/* the data pointer (not written) */
}  pb_t;

/* possible values for pb_type */
# define	PB_NOTYPE	0	/* unknown type */
# define	PB_REG		1	/* regular block */
# define	PB_RESP		2	/* response block */
# define	PB_ERR		3	/* error message */
# define	PB_SYNC		4	/* interrupt sync */
# define	PB_RESET	5	/* system reset */
# define	PB_DSPLY	6	/* display on output */

/* meta definitions go here (start at 16) */

/* definitions for pb_stat */
# define	PB_EOF		00001	/* end of file block */
# define	PB_FRFR		00002	/* originated from front end */
# define	PB_INFO		00004	/* info purposes only, no response */
# define	PB_INTR		00010	/* interrupt sync block */

/* definitions for pb_proc */
# define	PB_WILD		-2	/* all processes */
# define	PB_UNKNOWN	-1	/* unknown */
# define	PB_FRONT	0	/* front end */
/* other processes are given numbers from 2 */

/* definitions for pb_st */
/* define	PB_UNKNOWN	-1	*/
# define	PB_NONE		0	/* response block */
/* return values for pb_geterr for indicating error types */
# define	PB_FATERR	-1	/* fatal error */

# endif /* PB_DBSIZE */
