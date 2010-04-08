/* Copyright (c) 2004 Ingres Corporation */
/*----------------  iixa.h  ---------------------------------------------*/


/*
** The exported variable declaration for the Ingres' xa_switch structure.
**
*/

typedef struct  xa_switch_t  IIXA_SWITCH;

#ifdef _WIN32
__declspec(dllimport)
#endif
extern  IIXA_SWITCH  iixa_switch;


/*---------------  end of iixa.h  ----------------------------------------*/
