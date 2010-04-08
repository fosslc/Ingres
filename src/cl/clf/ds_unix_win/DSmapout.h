# define	HASHTAB_SZ	256	/* hash table size */

/* DSset returns the address of the pointer field */
# define DSset(s, p)	(( (i4 *) ((s) + (p)->ds_off)))

/* DSaddrof returns the pointer value */
# define DSaddrof(s, p)	(* (i4 *) ((s) + (p)->ds_off))

/* DSsizeof returns number of array elements or union tag */
# define DSsizeof(s, p) (* (i4 *) ((s) + (p)->ds_sizeoff))

# define	hash(s)	(((s >> 16) ^ s)  & 0x000000ff)

# define	DSCIRCL		1
# define	OPENERR		2
# define	IDNTFND		3
# define	INSTERR		4


typedef struct DShash {
	i4	addr;		/* addr of the data strucutre */
	i4	offset;		/* position of the data structure in the shared
				** data region
				**/
	struct 	DShash	*next;	/* point to the next linked node */
} DSHASH;

/* Tag for memory allocation */
# define	DSMEMTAG	42
