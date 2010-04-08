# ifndef array_h_def
# define array_h_def
/*
** Copyright (c) 1985, Ingres Corporation
*/

/**
** Name:	array.h		- Define ArrayOf
**
** Description:
**
**	ArrayOf(type)
**
** 	declares an array of arbitrary elements of the same type;
**	type may be a scalar, typedef or pointer type;
**	typically type is a pointer type so array
**	points to an array of pointers which point to the
**	elements.  This allows the size of the array to
**	change without moving the pointed at data.
**
**	It is up to the programmer to arrange for the allocation
**	of storage for the array elements and initialization of the
**	ArrayOf struct either thru compile-time definition and
**	initialization or thru dynamic allocation and assignment.
**
**	For example, ArrayOf(char *) declares a struct of the form:
**		struct {
**			int	size;
**			char *	*array;
**		}
**	which is a variable length array of char pointers.
**	It could be initialized as follows:
**	
**	char 		*strings[] = { "abc", "def", "ghi", "jkl"};
**	ArrayOf(char *)	stringArray = {
**		sizeof(strings)/sizeof(strings[0], strings
**	};
**
**	The expression involving sizeof's avoids having to count the
**	number of elements in the initialized array.
**
*/

# define	ArrayOf(type)			\
			struct {		\
				int	size;	\
				type	*array;	\
			}


typedef	ArrayOf(int *)	Array;		/* handy declaration for array
					** of arbitrarily typed elements
					*/

# endif /* array_h_def */
