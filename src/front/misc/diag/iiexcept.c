#include <compat.h>
/**
**	Copyright (c) 2004 Ingres Corporation
**      All rights reserved.
**
**  Name: iiexcept - Evidence analysis driver routine
**
**  Description:
**      This module forms the main driver for the analysis of evidence in an
**      evidence set. It outputs an evidence summary to stdout and stores it
**      in the evidence set.
**
**      The summary output to stdout is truncated at 128K which is the current
**      evidence summary limit
**
**      The iiexcept routine takes as its single parameter the evidence set
**      number to analyse.
**
**  History:
**	22-feb-1996 -- initial port to Open Ingres
**	13-Mch-96 (prida01)
**	    Remove all functionality and leave this empty for post processing
**	    of evidence sets on porting platforms.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
*/




/*{
**  Name: main - Entry point for iiexcept
**
**  Description:
**	This file is only used for porting, where post processing is
**	required on evidence sets.
**
**  Inputs:
**     i4  argc        Number of arguments
**     char *argv[]    Arguments
**
**  Returns:
**
**  History:
**	13-Mch-96 (prida01)
**	    Remove all functionality and leave this empty for post processing
**	    of evidence sets on porting platforms.
*/

main(argc,argv)
i4  argc;
char **argv;
{

   return;

}
   
