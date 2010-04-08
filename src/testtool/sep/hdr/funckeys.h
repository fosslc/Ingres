/*
**	Header information for processing of function keys
**	in SEP
**
**	History:
**
**	Feb-26-1890 (Eduardo)
**	    Created it
**	14-jan-1992 (DonJ)
**	    Modified all quoted string constants to use the ERx("") macro.
**	    Reformatted variable declarations to one per line for clarity.
**	03-jul-1992	(dkhor)
**		func keys #define return values NOT consistent with those
**		in front/frontcl/hdr/tdkeys.h.  Sync these #defines (FOUND,
**		NOTFOUND, etc).  Also typo-error on frscommds 'previousfield'.
**
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/


#define FRSCMMDS	26	/* number of FRS commands */
#define FKINTERMCAP	40	/* number of function keys supported */

/* return values used by func keys routines */

/* return values used by func keys routines */
# define        FOUND                   0
# define        NOTFOUND                1
# define        ALLCOMMENT              2

# define        FDUNKNOWN               0
# define        FDCMD                   1
# define        FDFRSK                  2
# define        FDMENUITEM              3
# define        FDCTRL                  4
# define        FDPFKEY                 5
# define        FDOFF                   6
# define        FDGOODLBL               7
# define        FDNOLBL                 8
# define        FDBADLBL                9
# define        FDARROW                 10

#define CTRL_ESC	-1
#define CTRL_DEL	-2

/* name of FRS commands */

char	*frscmmds[FRSCMMDS] = {
                                ERx("clear"),
                                ERx("clearrest"),
                                ERx("deletechar"),
                                ERx("downline"),
                                ERx("duplicate"),
                                ERx("editor"),
                                ERx("leftchar"),
				ERx("menu"),
                                ERx("mode"),
                                ERx("newrow"),
                                ERx("nextfield"),
                                ERx("nextitem"),
                                ERx("nextword"),
				ERx("previousfield"),
				ERx("previousword"),
                                ERx("printscreen"),
				ERx("redraw"),
				ERx("rightchar"),
                                ERx("rubout"),
                                ERx("skipfield"),
                                ERx("scrolldown"),
                                ERx("scrollleft"),
                                ERx("scrollright"),
                                ERx("scrollup"),
                                ERx("shell"),
                                ERx("upline")
			      };

/* pointer to function key values defined in termcap file */

char	**fktermcap[FKINTERMCAP] = { &K0, &K1, &K2, &K3, &K4, 
				    &K5, &K6, &K7, &K8, &K9, 
				    &KAA, &KAB, &KAC, &KAD, &KAE, &KAF, 
				    &KAG, &KAH, &KAI, &KAJ, &KAK, &KAL, 
				    &KAM, &KAN, &KAO, &KAP, &KAQ, &KAR, 
				    &KAS, &KAT, &KAU, &KAV, &KAW, &KAX, 
				    &KAY, &KAZ, &KKA, &KKB, &KKC, &KKD };

