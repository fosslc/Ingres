/*
**	Copyright (c) 2004 Ingres Corporation
*/
/*
**	constants for FElpxxxx routines dealing with list node pools
*/

#define FELP_MAGIC 666		/* unlikely value used for magic number */
#define FELP_NODES 100		/* minimum number of nodes to allocate */
#define PTRSIZE (sizeof(struct fl_node *))

/*
**	CEILING macro to calculate size in integral pointer boundaries
*/

#define CEILING(x) ((x + PTRSIZE - 1)/PTRSIZE)
