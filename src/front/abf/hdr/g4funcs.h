/*
** Copyright (c) 2004 Ingres Corporation
*/
/**
** Name:	g4funcs.h 
**
** Description:
**	Definitions used by the EXEC 4GL interface
**
** History:
**	15-dec-92 (davel)
**		 Initial version.
**	19-jun-93 (tomm)
**		Change ',' seperated typedefs to individual typedefs since
**		hp8 compiler *REALLY* doesn't like this.
**	21-jan-1999 (hanch04)
**	    replace nat and longnat with i4
**	31-aug-2000 (hanch04)
**	    cross change to main
**	    replace nat and longnat with i4
**/

# include <iisqlda.h>
# include <g4consts.h>

/* typedefs */
typedef struct {
    ER_MSGID 	errmsg;		
    i4  	numargs;
    PTR		args[10];
} G4ERRDEF;

typedef STATUS (STATUS_FUNC)(void);
typedef STATUS (STATUS_FUNC_AG4AC)(i4 array);
typedef STATUS (STATUS_FUNC_AG4BP)
	(i2 *ind, i4  isvar, i4  type, i4  length, PTR data, char *name);
typedef STATUS (STATUS_FUNC_AG4CC)(void);
typedef STATUS (STATUS_FUNC_AG4CH)(i4 object, i4  access, i4  index, i4  caller);
typedef STATUS (STATUS_FUNC_AG4DR)(i4 array, i4  index);
typedef STATUS (STATUS_FUNC_AG4FD)(i4 object, i4  language, IISQLDA *sqlda);
typedef STATUS (STATUS_FUNC_AG4GA)(i2 *ind, i4  isvar, i4  type, i4  length, 
			PTR data, char *name);
typedef STATUS (STATUS_FUNC_AG4GG)
	(i2 *ind, i4  isvar, i4  type, i4  length, PTR data, 
	 char *name, i4  gtype);
typedef STATUS (STATUS_FUNC_AG4GR)
	(i2 *rowind, i4  isvar, i4  rowtype, i4  rowlen, PTR rowptr, 
	 i4 array, i4  index);
typedef STATUS  (STATUS_FUNC_AG4I4)
	(i2 *ind, i4  isvar, i4  type, i4  length, PTR data, 
	 i4 object, i4  code);
typedef STATUS   (STATUS_FUNC_AG4IC)(char *name, i4  type);
typedef STATUS   (STATUS_FUNC_AG4IR)(i4 array, i4  index, i4 row, i4  state, i4  which);
typedef STATUS   (STATUS_FUNC_AG4RR)(i4 array, i4  index);
typedef STATUS  (STATUS_FUNC_AG4RV)(i2 *ind, i4  isvar, i4  type, i4  length, PTR data);
typedef STATUS   (STATUS_FUNC_AG4S4)
	(i4 object, i4  code, 
	 i2 *ind, i4  isvar, i4  type, i4  length, PTR data);
typedef STATUS   (STATUS_FUNC_AG4SA)(char *name, i2 *ind, i4  isvar, i4  type, 
			i4 length, PTR data);
typedef STATUS (STATUS_FUNC_AG4SE)(i4 frame);
typedef STATUS   (STATUS_FUNC_AG4SG)
	(char *name, i2 *ind, i4  isvar, i4  type, i4  length, PTR data);
typedef STATUS (STATUS_FUNC_AG4SR)(i4 array, i4  index, i4 row);
typedef STATUS  (STATUS_FUNC_AG4UD)
	(i4 lang, i4  dir, IISQLDA *sqlda);
typedef STATUS   (STATUS_FUNC_AG4VP)
	(char *name, i2 *ind, i4  isvar, i4  type, i4  length, PTR data);


/*
** Trasnfer vector type
*/
typedef struct
{
    STATUS_FUNC_AG4AC *arrayclear;
    STATUS_FUNC_AG4BP *byrefparam;
    STATUS_FUNC_AG4CC *callcomponent;
    STATUS_FUNC_AG4CH *checkobj;
    STATUS_FUNC_AG4DR *delrow;
    STATUS_FUNC_AG4FD *filldescr;
    STATUS_FUNC_AG4GA *getattr;
    STATUS_FUNC_AG4GG *getglobal;
    STATUS_FUNC_AG4GR *getrow;
    STATUS_FUNC_AG4I4 *inq4gl;
    STATUS_FUNC_AG4IC *initcall;
    STATUS_FUNC_AG4IR *insrow;
    STATUS_FUNC_AG4RR *remrow;
    STATUS_FUNC_AG4RV *retval;
    STATUS_FUNC_AG4S4 *set4gl;
    STATUS_FUNC_AG4SA *setattr;
    STATUS_FUNC_AG4SE *sendevent;
    STATUS_FUNC_AG4SG *setglobal;
    STATUS_FUNC_AG4SR *setrow;
    STATUS_FUNC_AG4UD *updatedescr;
    STATUS_FUNC_AG4VP *valparam;
} AG4XFERS;

/* extern's */
# define IIG4xfers		IIG4xfers_proto
FUNC_EXTERN VOID IIG4xfers(AG4XFERS *table);

# define IIAG4acArrayClear	IIAG4acArrayClear_proto
FUNC_EXTERN STATUS_FUNC_AG4AC	IIAG4acArrayClear;
# define IIAG4bpByrefParam	IIAG4bpByrefParam_proto
FUNC_EXTERN STATUS_FUNC_AG4BP	IIAG4bpByrefParam;
# define IIAG4ccCallComp	IIAG4ccCallComp_proto
FUNC_EXTERN STATUS_FUNC_AG4CC	IIAG4ccCallComp;
# define IIAG4chkobj		IIAG4chkobj_proto
FUNC_EXTERN STATUS_FUNC_AG4CH	IIAG4chkobj;
# define IIAG4drDelRow		IIAG4drDelRow_proto
FUNC_EXTERN STATUS_FUNC_AG4DR	IIAG4drDelRow;
# define IIAG4fdFillDscr	IIAG4fdFillDscr_proto
FUNC_EXTERN STATUS_FUNC_AG4FD	IIAG4fdFillDscr;
# define IIAG4gaGetAttr		IIAG4gaGetAttr_proto
FUNC_EXTERN STATUS_FUNC_AG4GA	IIAG4gaGetAttr;
# define IIAG4ggGetGlobal	IIAG4ggGetGlobal_proto
FUNC_EXTERN STATUS_FUNC_AG4GG	IIAG4ggGetGlobal;
# define IIAG4grGetRow		IIAG4grGetRow_proto
FUNC_EXTERN STATUS_FUNC_AG4GR	IIAG4grGetRow;
# define IIAG4i4Inq4GL		IIAG4i4Inq4GL_proto
FUNC_EXTERN STATUS_FUNC_AG4I4	IIAG4i4Inq4GL;
# define IIAG4icInitCall	IIAG4icInitCall_proto
FUNC_EXTERN STATUS_FUNC_AG4IC	IIAG4icInitCall;
# define IIAG4irInsRow		IIAG4irInsRow_proto
FUNC_EXTERN STATUS_FUNC_AG4IR	IIAG4irInsRow;
# define IIAG4rrRemRow		IIAG4rrRemRow_proto
FUNC_EXTERN STATUS_FUNC_AG4RR	IIAG4rrRemRow;
# define IIAG4rvRetVal		IIAG4rvRetVal_proto
FUNC_EXTERN STATUS_FUNC_AG4RV	IIAG4rvRetVal;
# define IIAG4s4Set4GL		IIAG4s4Set4GL_proto
FUNC_EXTERN STATUS_FUNC_AG4S4	IIAG4s4Set4GL;
# define IIAG4saSetAttr		IIAG4saSetAttr_proto
FUNC_EXTERN STATUS_FUNC_AG4SA	IIAG4saSetAttr;
# define IIAG4seSendEvent	IIAG4seSendEvent_proto
FUNC_EXTERN STATUS_FUNC_AG4SE	IIAG4seSendEvent;
# define IIAG4sgSetGlobal	IIAG4sgSetGlobal_proto
FUNC_EXTERN STATUS_FUNC_AG4SG	IIAG4sgSetGlobal;
# define IIAG4srSetRow		IIAG4srSetRow_proto
FUNC_EXTERN STATUS_FUNC_AG4SR	IIAG4srSetRow;
# define IIAG4udUseDscr 	IIAG4udUseDscr_proto
FUNC_EXTERN STATUS_FUNC_AG4UD	IIAG4udUseDscr;
# define IIAG4vpValParam	IIAG4vpValParam_proto
FUNC_EXTERN STATUS_FUNC_AG4VP	IIAG4vpValParam;

/*
** The rest of the function declarations do not appear in code generated by
** the 3GL pre-processors.
*/

# define IIAG4dbvFromObject      IIAG4dbvFromObject_proto
FUNC_EXTERN VOID IIAG4dbvFromObject(PTR object, DB_DATA_VALUE *dbv);
# define IIAG4semSetErrMsg      IIAG4semSetErrMsg_proto
FUNC_EXTERN VOID IIAG4semSetErrMsg(G4ERRDEF *errparms, bool dump);
# define IIAG4Init              IIAG4Init_proto
FUNC_EXTERN STATUS IIAG4Init(void);
# define IIAG4get_data		IIAG4get_data_proto
FUNC_EXTERN STATUS IIAG4get_data(
			DB_DATA_VALUE *dbdv, i2 *ind, i4  type, i4  length, 
			PTR data);
# define IIAG4set_data		IIAG4set_data_proto
FUNC_EXTERN STATUS IIAG4set_data(
			DB_DATA_VALUE *dbdv, i2 *ind, i4  isvar, i4  type, 
			i4 length, PTR data, G4ERRDEF *g4errdef);
# define IIAG4iosInqObjectStack     IIAG4iosInqObjectStack_proto
FUNC_EXTERN STATUS IIAG4iosInqObjectStack(i4 *current_sp, i4  *current_alloc);
# define IIAG4fosFreeObjectStack     IIAG4fosFreeObjectStack_proto
FUNC_EXTERN STATUS IIAG4fosFreeObjectStack(i4 reset_sp);
# define IIAG4aoAddObject	IIAG4aoAddObject_proto
FUNC_EXTERN STATUS IIAG4aoAddObject(PTR object, i4 *objno);
# define IIAG4gkoGetKnownObject 	IIAG4gkoGetKnownObject_proto
FUNC_EXTERN STATUS IIAG4gkoGetKnownObject(i4 index, PTR *object);
