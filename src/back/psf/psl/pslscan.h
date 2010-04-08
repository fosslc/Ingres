/*
**Copyright (c) 2004 Ingres Corporation
*/

/**
** Name: PSLSCAN.H - Names and types used by the QUEL scanner.
**
** Description:
**      This file defines the names and types used by the QUEL scanner.
**
** History: $Log-for RCS$
**      04-feb-86 (jeff)
**          written
**/

/*
**  Defines of other constants.
*/

/*
**      Lvalues associated with tokens.
*/
#define                 PSL_GOVAL       1   /* Token can intro statement */
#define			PSL_ONSET	2   /* set "on" statement */
#define			PSL_OFFSET	3   /* set "off" statement */
#define			PSL_OR		4   /* or operator */
#define			PSL_AND		5   /* and operator */
#define			PSL_ANY		6   /* any aggregate */
#define			PSL_AVG		7   /* avg aggregate */
#define			PSL_MAX		8   /* max aggregate */
#define			PSL_MIN		9   /* min aggregate */
#define			PSL_NOT		10  /* not aggregate */
#define			PSL_SUM		11  /* sum aggregate */
#define			PSL_AVGU	12  /* avgu aggregate */
#define			PSL_SUMU	13  /* sumu aggregate */
#define			PSL_CNT		14  /* count aggregate */
#define			PSL_CNTU	15  /* countu aggregate */
/*[@defined_constants@]...*/

/*[@group_of_defined_constants@]...*/
/*[@type_definitions@]*/
