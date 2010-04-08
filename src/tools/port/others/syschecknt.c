/*
**Copyright (c) 2004 Ingres Corporation
*/
/*
**  syschecknt - dummy syscheck for NT
**
**  History:
**	20-jul-95 (sarjo01)
**		Created,
**	07-sep-1995 (canor01)
**		Exit with OK.
**	01-Dec-1995 (walro03/mosjo01)
**		In previous change (07-sep), use of PCexit() should have been
**		exit().  At the time this file is compiled, the cl is not yet
**		available, nor does ming include libingres in the compile.
**	17-oct-97 (mcgem01)
**		Change product name to OpenIngres.
**	20-apr-98 (mcgem01)
**		Product name change to Ingres.
**
*/

main()
{
	printf("Ingres SYSCHECK\n");
	exit(0);
}

