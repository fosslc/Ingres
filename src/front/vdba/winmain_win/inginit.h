#ifndef WIN32
#include <iiwnmain.h>   /* Must be included in ALL WinMain 3G/L modules */

#ifdef MAIN_ARGC_NAT    /* used internally for main(nat, char**) programs */
typedef long ARGCTYPE;
#else
typedef int  ARGCTYPE;
#endif
void	PCexit(long);
extern short WNCurrentSS;        /* Stack segment of the current task */

char    IIpnProgName[16]="";
short   IIshShowHistory  = 1;           /* 2 -> show History window as MAX'ED*/
					/* 1 -> show History window as NORMAL*/
					/* 0 -> keep History window as is    */

static	char	argv0[_MAX_PATH+1] = "";
static  char    buf[2048] = "";	/* place to hold cmdline args if from a file.*/
static	short	status = 0;	/* status of initalizing INGRES sub-system */
static  FARPROC UsrYield = NULL;


/*
**  Specifies the normal Windows 3.x queue size and the size preferred
**  for this app.
*/
# define NORMAL_QUEUE_SIZE      8
# define PREFERRED_QUEUE_SIZE  30
#endif


