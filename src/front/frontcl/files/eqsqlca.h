/*
** Copyright (c) 2004 Ingres Corporation
*/

#ifndef EQSQLCA_H_INCLUDED
#define EQSQLCA_H_INCLUDED

/************************* esqlca definition ***************************/

/*
** SQLCA - Structure to hold the error and status information returned 
**		by INGRES runtime routines
**    04-Dec-1998 (merja01)
**      Change longs to ints for Digital UNIX.  Note, this change
**      had been previously sparse branched.
**    04-May-1999 (hweho01)
**      Change longs to ints for ris_u64 (AIX 64-bit port).
**    19-apr-1999 (hanch04)
**      Change longs to ints for everything
**    11-Oct-2007 (kiria01) b118421
**      Include guard against repeated inclusion.
*/


typedef struct {
    char	sqlcaid[8];
    int 	sqlcabc;
    int 	sqlcode;
    struct {
	short	sqlerrml;
	char	sqlerrmc[70];
    } sqlerrm;
    char	sqlerrp[8];
    int 	sqlerrd[6];
    struct {
	char	sqlwarn0;
	char	sqlwarn1;
	char	sqlwarn2;
	char	sqlwarn3;
	char	sqlwarn4;
	char	sqlwarn5;
	char	sqlwarn6;
	char	sqlwarn7;
    } sqlwarn;
    char	sqlext[8];
} IISQLCA;

#endif /* EQSQLCA_H_INCLUDED */
