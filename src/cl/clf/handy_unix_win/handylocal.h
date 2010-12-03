/*
** Copyright (c) 2010 Ingres Corporation
*/
#ifndef HANDYLOCAL_H
#define HANDYLOCAL_H

/**
** Name: handylocal.h - Handy Unix Windows definitions.
**
** Description:
**      This header contains the description of the types, constants and
**      functions used internally in handy_unix_win.
**
** History:
**      29-Nov-2010 (frima01) SIR 124685
**          Created.
**/

/* bstcpport.c */
FUNC_EXTERN STATUS BS_tcp_port(
	char *pin,
	i4 subport,
	char *pout,
	char *pout_symbolic);

/* bstcpaddr.c */
struct sockaddr_in;
struct addrinfo;
FUNC_EXTERN STATUS BS_tcp_addr(
	char *buf,
	bool outbound,
	struct sockaddr_in *s);
FUNC_EXTERN STATUS BS_tcp_addrinfo(
	char *buf,
	bool outbound,
	int ip_family,
	struct addrinfo **aiList);

/* bsunixaddr.c */
struct sockaddr_un;
FUNC_EXTERN STATUS BS_unix_addr(
	char *buf,
	struct sockaddr_un *s);

#endif
