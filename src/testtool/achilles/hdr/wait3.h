/*
** History:
**    11-jan-93 (rkumar)
**      define union wait similar to the one on Sun platform. This
**      is not available on Sequent platforms.
**    23-jun-1997 (walro03)
**	Extend to ICL DRS 6000 (dr6_us5).
*/

#if defined(sqs_ptx) || defined(dr6_us5)
union wait {
	int w_status;		/* used in system call */
	struct {
		unsigned short w_Termsig:7;
		unsigned short w_Coredump:1;
		unsigned short w_Retcode:8;
	} w_T;
	struct {
		unsigned short w_Stopval:8;
		unsigned short w_Stopsig:8;
	} w_S;
};

#define w_termsig	w_T.w_Termsig
#define w_coredump	w_T.w_Coredump
#define w_retcode	w_T.w_Retcode
#define w_stopval	w_S.w_Stopval
#define w_stopsig	w_S.w_Stopsig

#endif	/* sqs_ptx dr6_us5 */
