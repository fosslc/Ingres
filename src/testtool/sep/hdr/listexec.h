/*
**  Listexec.h
**
**  History:
**	##-###-#### (XXXX)  Created
**	14-jan-1992 (DonJ)
**	    Modified all quoted string constants to use the ERx("") macro.
**	    Reformatted variable declarations to one per line for clarity.
**      26-apr-1993 (donj)
**	    Finally found out why I was getting these "warning, trigrah sequence
**	    replaced" messages. The question marks people used for unknown
**	    dates. (Thanks DaveB).
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
#define ING_PARTS   ERx("ingresprt.lis")
#define ING_TYPES   ERx("ingrestyp.lis")
#define ING_LEVELS  ERx("ingreslvl.lis")
#define MAX_PARTS   10
#define MAX_SUBS    20

typedef struct part_node
{
    char                  *partname ;
    char                  *subs [MAX_SUBS] ;
    char                   runkey [MAX_SUBS] ;
    char                  *substorun ;
}   PART_NODE;


#ifdef main
#undef main
#endif

