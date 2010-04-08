/*
**  sepmenu.h
**
**	lines for main menu
**
**  History:
**	##-###-#### (XXXX)
**	    Created
**	14-jan-1992 (donj)
**	    Modified all quoted string constants to use the ERx("") macro.
**      26-apr-1993 (donj)
**	    Finally found out why I was getting these "warning, trigrah sequence
**	    replaced" messages. The question marks people used for unknown
**	    dates. (Thanks DaveB).
*/

#define Header1	  ERx(" MAIN MENU")
#define Header2	  ERx("Configuration File : %s")
#define Menuline1 ERx("(%d) Rename configuration file")
#define Menuline2 ERx("(%d) Enter output directory ( full pathname )")
#define Menuline3 ERx("(%d) Enter setup file name")
#define Menuline4 ERx("(%d) Enter batch queue name")
#define Menuline5 ERx("(%d) Enter starting time")
#define Menuline6 ERx("(%d) Enter list of tests ( full pathname )")
#define Menuline7 ERx("(%d) Enter directory where tests reside (if other than standard) ")
#define Menuline8 ERx("(%d) Select tests to be executed")

#define Menugo    ERx("(%d) SUBMIT")
#define Menuquit  ERx("(%d) QUIT")
#define Menuvar   ERx("     => %s")

/*
    values for main menu choices
*/

#define REN_CONFGN  1
#define OUTPUT_DIR  2
#ifdef VMS
#define SETUP_FILE  3
#define BATCH_QUEUE 4
#define START_TIME  5
#define TESTS_LIST  6
#define TESTS_DIR   7
#define SELECT_MENU 8
#define GO_RUN      9
#define QUIT_RUN    10
#else
#define TESTS_LIST  3
#define TESTS_DIR   4
#define SELECT_MENU 5
#define GO_RUN      6
#define QUIT_RUN    7
#endif

