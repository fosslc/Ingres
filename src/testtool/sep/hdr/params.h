/*
**  Params.h
**
**  History:
**	##-###-#### (XXXX)
**	14-jan-1992 (DonJ)
**	    Reformatted variable declarations to one per line for clarity.
**      26-apr-1993 (donj)
**	    Finally found out why I was getting these "warning, trigrah sequence
**	    replaced" messages. The question marks people used for unknown
**	    dates. (Thanks DaveB).
**
*/
GLOBALREF    char          confFile [] ;
GLOBALREF    char          outDir [] ;
GLOBALREF    char          testList [] ;
GLOBALREF    char          testDir [] ;
#ifdef VMS
GLOBALREF    char          setFile [] ;
GLOBALREF    char          batchQueue [] ;
GLOBALREF    char          startTime [] ;
#endif
GLOBALREF    PART_NODE    *ing_parts [] ;
GLOBALREF    PART_NODE    *ing_types ;
GLOBALREF    PART_NODE    *ing_levels ;
