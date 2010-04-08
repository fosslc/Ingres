
/*--------------------------- iicxinit.h -----------------------------------*/

/*
** Copyright (c) 2004 Ingres Corporation
*/

/**
**  Name: iicxinit.h - IICX entrypoints, called at startup and shutdown
**                     of the process that has linked in the IICX modules.
**
*/

#ifndef  IICXINIT_H
#define  IICXINIT_H

/*
**   Name: IIcx_startup()
**
**   Description:
**       This function initializes all global CBs/MAIN CBs for all the CB 
**       types that are "known" throughout the IICX subcomponent.This call
**       should be made *once* for the process, typically along with the
**       startup of other process sub-components (e.g. LIBQXA in the Ingres
**       F/E space). 
**
**   Inputs:
**       None.
**
**   Outputs:
**       Returns: E_DB_OK if successful initialization/startup.
*/

FUNC_EXTERN  DB_STATUS  IICXstartup(void);



/*
**   Name: IIcx_shutdown()
**
**   Description:
**       This function cleans up all IICX-related global structures and
**       fields. It is called at the time of shutdown of the process that
**       linked in the IICX modules.
**
**   Inputs:
**       None.
**
**   Outputs:
**       Returns: E_DB_OK if successful shutdown.
*/

FUNC_EXTERN  DB_STATUS  IICXshutdown(void);


/*
**
**  Global Structures.
**
*/

GLOBALREF  IICX_MAIN_CB   *IIcx_main_cb;


#endif  /* ifndef IICXINIT_H */

/*--------------------------- end of iicxinit.h ----------------------------*/

