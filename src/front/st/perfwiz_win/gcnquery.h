/*
** Copyright (c) 1999 Ingres Corporation all rights reserved
*/

/*
**  Name: gcnquery.h
**
**  Description:
**	This file contains defines needed for using the functions in
**	gcnquery.c .
**
**  History:
**	27-aug-1999 (somsa01)
**	    Created.
*/

struct serverlist
{
    char		server_class[33];
    char		server_id[64];
    struct serverlist	*next;
};

struct nodelist {
    char		nodename[33];
    char		hostname[33];
    struct serverlist	*svrptr;
    struct nodelist	*next;
};

