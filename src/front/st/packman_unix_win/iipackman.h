/*
** Copyright Ingres Corporation 2006. All rights reserved
*/

/*
** Name: iipackman.h
**
** Description:
**
**      Header file for respsonse file generation mode of the GUI 
**	installer.
**
** History:
**
**    05-Oct-2006 (hanje04)
**	SIR 116877
**      Created.
*/
STATUS
iipackman_main (int argc, char *argv[]);

STATUS
init_rfgen_mode(void);

STATUS
rfgen_cleanup( void );

