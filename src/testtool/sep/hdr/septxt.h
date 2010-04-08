/*
**  Septxt.h
**
**	SEP text resources
**
**  History:
**	##-###-#### (XXXX)
**	    Created.
**	14-jan-1992 (donj)
**	    Modified all quoted string constants to use the ERx("") macro.
**	15-sep-1992 (donj)
**	    Took out some commented out definitions. Added YES_NO.
**	21-oct-1992 (donj)
**	    Added Abort option to DIFF menus and answers. Had to shorten
**	    some of the message lines.
**      26-apr-1993 (donj)
**	    Finally found out why I was getting these "warning, trigrah sequence
**	    replaced" messages. The question marks people used for unknown
**	    dates. (Thanks DaveB).
**
*/

#define WELCOME	    	ERx("Welcome to SEP !!!")
#define NEW_CANON   	ERx("should canon be ?: (R)esults (E)mpty (I)gnored (C)omment edit")
#define NEW_CANON_A 	ERx("RrEeIiCc")
#define DIFF_MENU   	ERx("(P)rev or (N)ext screen (C)omments (Q)uit (D)isconnect (A)bort")
#define DIFF_MENU_A 	ERx("PpNnCcQqDdAa")
#define DIFF_MENU_M   	ERx("(P)rev or (N)ext screen (C)omments (E)dit_canon (Q)uit (D)isconnect (A)bort")
#define DIFF_MENU_AM 	ERx("PpNnCcEeQqDdAa")
#define UPD_CANON   	ERx("make results (M)ain (A)lternate (O)nly or (I)gnored canon; (E)xit")
#define UPD_CANON_A 	ERx("MmAaOoIiEe")
#define YES_NO          ERx("YyNn")
