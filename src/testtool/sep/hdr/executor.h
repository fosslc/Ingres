/*
** History:
**	##-###-1989 (eduardo)
**	    Created.
**	31-jul-1990 (rog)
**	    Changed "SUBSYSTEM" to "FACILITY" and added History section.
**	14-jan-1992 (donj)
**	    Modified all quoted string constants to use the ERx("") macro.
**	    Reformatted variable declarations to one per line for clarity.
**      26-apr-1993 (donj)
**	    Finally found out why I was getting these "warning, trigrah sequence
**	    replaced" messages. The question marks people used for unknown
**	    dates. (Thanks DaveB).
*/

#define MAX_PARTS   10
#define MAX_SUBS    20

typedef struct part_node
{
    char                  *partname ;
    i4                     partnum ;
    char                  *subs [MAX_SUBS] ;
    i4                     subinit [MAX_SUBS] ;
    i4                     subrun [MAX_SUBS] ;
    i4                     subdif [MAX_SUBS] ;
    i4                     subabd [MAX_SUBS] ;
}   PART_NODE;

#define header1 ERx("\nPART      FACILITY      TEST                DIFS FOUND\n")
#define header2 ERx("====      ========      ====                ==========\n\n")

#define hcol2 10
#define hcol3 24
#define hcol4 44

#define header3 ERx("\n\nPART      FACILITY      #_INITIATED    #_NO_DIFS     #_W/DIFS     #_ABORTED\n")
#define header4 ERx("====      ========      ===========    =========     ========     =========\n\n")

#define h1col2 10
#define h1col3 24
#define h1col4 39
#define h1col5 53
#define h1col6 66

#define header5 ERx("\n\n*** JOB WAS ABORTED BECAUSE TOLERANCE LEVEL WAS REACHED ***\n")

#ifdef main
#undef main
#endif

