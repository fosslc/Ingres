/*
**  Lists.h
**
**  History:
**	##-###-#### (XXXX) Created.
**	14-jan-1992 (DonJ)
**	    Modified all quoted string constants to use the ERx("") macro.
**      26-apr-1993 (donj)
**	    Finally found out why I was getting these "warning, trigrah sequence
**	    replaced" messages. The question marks people used for unknown
**	    dates. (Thanks DaveB).
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/
#define FElist	  ERx("fesubs.lis")
#define BElist	  ERx("besubs.lis")
#define EQUELlist ERx("equelsubs.lis")
#define SQLlist	  ERx("esqlsubs.lis")
#define TYPElist  ERx("ingrestyp.lis")
#define LEVELlist ERx("ingreslvl.lis")
