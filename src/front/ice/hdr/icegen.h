/*
** Copyright (c) 2004 Ingres Corporation
*/
/*
**
** History:
**      05-Apr-2001 (fanra01)
**          SIR 103810
**          Created
**      01-Jun-2001 (peeje01)
**          SIR 103810
**          Wrap NT specific stuff in NT_GENERIC ifdefs
**      11-Jun-2001 (fanra01)
**          Sir 103810
**          Set the OUTHANDLER prototype back to char* from const char*
**          as the parameter type prevents compilation on NT.
**      25-Jul-2001 (peeje01)
**          Sir 103810
**          Add XML and XMLPDATA for XML
**          Add SYSTEM for ICE System functions
**    	30-jul-2001 (somsa01)
**          Changed size argument of igen_setgenln to SIZE_TYPE.
**      08-Aug-2002 (peeje01)
**          Bug 106445
**          Re-architect i3ce_if syntax to allow for defined and expression
**          tests.
**          Add elements i3ce_conditionExpression, i3ce_conditionDefined
**          There is no support for AND/OR/NOT in this change.
**    	02-Oct-2002 (drivi01)
**	    Due to lost change, modified a datatype for dlen variable in 
**	    igen_setgenln function from u_i4 to SIZE_TYPE because of the link 
**	    problem. ICETranslateHandlers.cpp expected dlen of type SIZE_TYPE 
**	    causing link problems. Put the lost comment for the lost change back.
*/
# ifndef icegen_h_INCLUDED
# define icegen_h_INCLUDED

# define E_ICE_MASK 0x00030000
# define E_IGEN_INVALID_PARAM    E_CL_MASK + E_ICE_MASK + 0x01

# define ICE_ROOT_LVL   0

typedef struct _tag_icegenln    ICEGENLN, *PICEGENLN;

# define ICE_LN_DATA( ln )  (char*)ln + sizeof(ICEGENLN)

enum ICEKEY {
	ATTR,
	CASE,
	COMMIT,
	CONDITION,
	CONDITIONdEFINED,
	CONDITIONeXPRESSION,
	DATABASE,
	DECLARE,
	DEFAULT,
	DESCRIPTION,
	EaTTRIBUTES,
	EaTTRIBUTE,
	EaTTRIBUTEnAME,
	EaTTRIBUTEvALUE,
	EcHILDREN,
	EcHILD,
	ELSE,
	EXT,
	EXTEND,
	FUNCTION,
	HEADERS,
	HEADER,
	HTML,
	HYPERLINK,
	HYPERLINKaTTRS,
	IF,
	INCLUDE,
	ICECLOSE,
	ICEOPEN,
	LINK,
	LINKS,
	NULLVAR,
	PARAMETER,
	PARAMETERS,
	PASSWORD,
	QUERY,
	RAW,
	RELATION_DISPLAY,
	ROLLBACK,
	ROWS,
	SQL,
	STATEMENT,
	SWITCH,
	SYSTEM,
	TARGET,
	THEN,
	TYPE,
	UNKNOWN,
	USERID,
	VAR,
	XML,
	XMLPDATA };

struct _tag_icegenln
{
    u_i4        flags;          /* set of indicators */
# define        ICE_ENTRY_INUSE     1
# define        ICE_REPEAT          2
# define        ICE_FLUSH_ENTRY     8
// ICE States
# define    ICE_EXTEND		    16
# define    ICE_FLAG_GRAVES_OPENED  32
# define        ICE_FLG_MASK    ICE_ENTRY_INUSE | ICE_REPEAT | ICE_FLUSH_ENTRY
    u_i4        nestlevel;      /* depth of macro levels */
    u_i4        linelen;
    PICEGENLN   parent;         /* parent entry if nestlevel is non-zero */
    enum ICEKEY handler;        /* handler identifier to process this entry */

};

# ifdef NT_GENERIC
typedef int ( __cdecl *OUTHANDLER)( char* outstr );
# else
typedef int ( *OUTHANDLER)( const char* outstr );
# endif

#ifdef __cplusplus
extern "C"
{
#endif

FUNC_EXTERN STATUS
igen_setgenln( PSTACK pstack, u_i4 flags, u_i4 lvl,
    enum ICEKEY handler, char* data, SIZE_TYPE dlen, PICEGENLN* igenln );

FUNC_EXTERN STATUS
igen_flush( PSTACK pstack, OUTHANDLER fnout );

FUNC_EXTERN STATUS
igen_getparent( PICEGENLN igenln, PICEGENLN* parent );

FUNC_EXTERN STATUS
igen_setflags( PICEGENLN igenln, u_i4 flags );

FUNC_EXTERN STATUS
igen_clrflags( PICEGENLN igenln, u_i4 flags );

FUNC_EXTERN STATUS
igen_testflags( PICEGENLN igenln, u_i4 flags );

FUNC_EXTERN STATUS
igen_getstart( PSTACK pstack, u_i4 flags, u_i4 level, char* tagName );

#ifdef __cplusplus
}
#endif

# endif /* icegen_h_INCLUDED */
