/*
** Copyright (c) 2004 Ingres Corporation
**
**  BUF.H -- buffer definitions
*/

/*
**	15-nov-91 (leighb) DeskTop Porting Change:
**		Changed 'BUFSIZE' to 'TMBUFSIZE' to avoid conflict with
**		'BUFSIZE' in compiler header.
**	18-jul-96 (chech02) bug # 77880, #77876, #77874
**		Changed *qb_char to __huge *qb_char in qbuf struct to allow
**		access data larger than 64K for windows 3.1 port. 
**		Added bufput() and m_bufput() function prototypes.
**	26-Aug-2009 (kschendel) b121804
**	    Add prototypes to keep gcc 4.3 happy.
*/

# define	TMBUFSIZE		256		 

struct buf
{
	struct buf	*nextb;
	char		buffer[TMBUFSIZE];		 
	char		*ptr;
};

# ifndef NULL
# define	NULL	0
# endif

/*
** 11/16/87 - increase QBUFSIZE to 8K for bug 1336
** 03/16/90 - changed query buffer from a static array to a dynamically
**	      allocated buffer for bug 9489 and 9037.  Query buffer will
**	      increase if more space is required.
*/
# define QBUFSIZE	4096	/* Initial query buffer size */
# define QBUFS		2

struct 	qbuf	{
	i2	qb_stat;
	i4	qb_count;
	i4	qb_end;
	i4	qb_bufsiz;
#ifdef WIN16
	char	__huge *qb_char;
#else 
	char	*qb_char;
#endif /* WIN16 */
	char	*qb_pbuf;
};

#ifdef WIN16
FUNC_EXTERN bufput(char, struct buf **);
FUNC_EXTERN m_bufput(i4, struct buf **);
#endif

FUNC_EXTERN struct qbuf *q_open(char);
FUNC_EXTERN struct qbuf *q_ropen(struct qbuf *, char);
