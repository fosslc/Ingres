/*
**Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <sl.h>
#include    <iicommon.h>
#include    <dbdbms.h>
#include    <duf.h>
#include    <lo.h>
#include    <nm.h>
#include    <st.h>
#include    <er.h>
#include    <duerr.h>

/**
**
**  Name: DUUINPTH.C -    routines which build various ingres paths needed
**			    by the system utilities.
**
**  Description:
**        This file contains routines which build Ingres system paths that
**	are needed by the various system utilities. 
**
**
**	    du_ajnl_ingpath() - build the path for the active journal dir.
**          du_ckp_ingpath() -  build the path for the checkpoint directory.
**	    du_db_ingpath() -   build the path for the default DB directory.
**	    du_ejnl_ingpath() - build the path for the expired journal
**				directory.
**	    du_extdb_ingpath() - build the path for an extended database
**				directory.
**	    du_work_ingpath() - build the path for a work location.
**	    du_fjnl_ingpath() - build the path for the full journal directory.
**	    du_jnl_ingpath() -  build the path for the journal directory.
**terminator
**	    du_dmp_ingpath() -	build the path for the dump directory
[@func_list@]...
**
**
**  History:    
**      3-Sep-86 (ericj)
**          Initial creation.
**      16-Apr-87 (ericj)
**          Added du_jnl_ingpath() to support new journaling directory
**	    structure.
**	27-jan-1989 (EdHsu)
**	    Added routine du_dmp_ingpath to support online backup
[@history_line@]...
[@history_template@]...
**	8-aug-93 (ed)
**	    unnest dbms.h
**	31-jan-94 (jrb)
**	    Added du_work_ingpath for work locations (MLSort project).
**/

/*
[@forward_type_references@]
[@forward_function_references@]
[@#defines_of_other_constants@]
[@type_definitions@]
[@global_variable_definitions@]
[@static_variable_or_function_definitions@]
[@function_definition@]...
*/


/*{
** Name: duu_ajnl_ingpath() - build an "active" journal directory path.
**	 duu_ckp_ingpath()  - build checkpoint directory path.
**	 duu_db_ingpath()   - build "default" data directory path.
**	 duu_extdb_ingpath() - build an "extended" data directory path.
**	 duu_work_ingpath() - build a work location directory path.
**	 duu_ejnl_ingpath() - build an "expired" journal directory path.
**	 duu_fjnl_ingpath() - build a "full" journal directory path.
**	 duu_jnl_ingpath()  - build an "active" journal directory path.
**
** Description:
**        These routines build paths to various DBMS directories.  They all
**	have the same interface.  ALL OF THESE ROUTINES WILL RETURN AN EMPTY
**	CHARACTER STRING if unable to build the path.
**
** Inputs:
**      area_name                       Buffer containing the area_name of
**					the directory being built.
**	db_name				Name of the database whose directories
**					are being accessed.
**	dbms_path			Pointer to a buffer that is DI_PATH_MAX
**					+ 1 in size to return the DBMS path in.
**	errcb				Error control block
**
** Outputs:
**      *dbms_path                      The DBMS path. -- WILL BY NULL IF
**					 LOingpath unsuccessful.
**	Returns:
**	    E_DU_OK
**	    duerror()
**	Exceptions:
**	    none
**
** Side Effects:
**	    none
**
** History:
**      29-Oct-86 (ericj)
**	    Initial creation.
**      16-Apr-87 (ericj)
**          Added du_jnl_ingpath() to support new journaling directory
**	    structure.
**	26-Jun-89 (teg)
**	    LOingpath constants must now be prefixed with LO_, as CL spec
**	    changed.
**      29-Mar-2005 (chopr01) (b113078)
**         Modified routine du_extdb_ingpath() to handle raw location
[@history_line@]...
[@history_template@]...
*/


STATUS
du_ajnl_ingpath(area_name, db_name, dbms_path,errcb)
char	    area_name[];
char	    db_name[];
char	    dbms_path[];
DU_ERROR    *errcb;
{
    LOCATION	    LOtmp;
    char	    *cp;

    errcb->du_clerr  = LOingpath(area_name, db_name, JNLACTIVE, &LOtmp);
    if (errcb->du_clerr  == OK)
    {
        LOtos(&LOtmp, &cp);
        STcopy(cp, dbms_path);
    }
    else
    {
	dbms_path[0] = '\0';
	return(du_error(errcb, E_DU2003_BAD_ACT_JNL_DIR, 4, 
			0, area_name, 0, db_name));
    }
    return ( (DU_STATUS) E_DU_OK);
}


STATUS
du_ckp_ingpath(area_name, db_name, dbms_path,errcb)
char	    area_name[];
char	    db_name[];
char	    dbms_path[];
DU_ERROR    *errcb;
{
    LOCATION	    LOtmp;
    char	    *cp;

    errcb->du_clerr  = LOingpath(area_name, db_name, LO_CKP, &LOtmp);
    if (errcb->du_clerr  == OK)
    {
        LOtos(&LOtmp, &cp);
        STcopy(cp, dbms_path);
    }
    else
    {
	dbms_path[0] = '\0';
	return(du_error(errcb, E_DU2004_BAD_CKPT_DIR, 4, 
			0, area_name, 0, db_name));
    }
    return ( (DU_STATUS) E_DU_OK);
}

STATUS
du_db_ingpath(area_name, db_name, dbms_path,errcb)
char	    *area_name;
char	    *db_name;
char	    *dbms_path;
DU_ERROR    *errcb;
{
    LOCATION	    LOtmp;
    char	    *cp;

    errcb->du_clerr  = LOingpath(area_name, db_name, LO_DB, &LOtmp);
    if (errcb->du_clerr  == OK)
    {
        LOtos(&LOtmp, &cp);
        STcopy(cp, dbms_path);
    }
    else
    {
	dbms_path[0] = '\0';
	return(du_error(errcb, E_DU2005_BAD_DFLT_DB_DIR, 4, 
			0, area_name, 0, db_name));
    }
    return ( (DU_STATUS) E_DU_OK);
}

STATUS
du_ejnl_ingpath(area_name, db_name, dbms_path,errcb)
char	    area_name[];
char	    db_name[];
char	    dbms_path[];
DU_ERROR    *errcb;
{
    LOCATION	    LOtmp;
    char	    *cp;

    errcb->du_clerr  = LOingpath(area_name, db_name, JNLEXPIRED, &LOtmp);
    if (errcb->du_clerr  == OK)
    {
        LOtos(&LOtmp, &cp);
        STcopy(cp, dbms_path);
    }
    else
    {
	dbms_path[0] = '\0';
	return(du_error(errcb, E_DU2006_BAD_XPIR_JNL_DIR, 4, 
			0, area_name, 0, db_name));
    }
    return ( (DU_STATUS) E_DU_OK);
}

/* Build an extended location for a database. */
STATUS
du_extdb_ingpath(area_name, db_name, dbms_path,errcb)
char        area_name[];
char	    db_name[];
char	    dbms_path[];
DU_ERROR    *errcb;
{
    LOCATION	    LOtmp;
    char	    *cp;

    if (STcmp(db_name, LO_RAW) == 0)
    {
      errcb->du_clerr  = LOingpath(area_name, LO_RAW, LO_DB, &LOtmp);
      if (errcb->du_clerr  == LO_IS_RAW)
      {
	LOtos(&LOtmp, &cp);
	STcopy(cp, dbms_path);
      }
      else
      {
	dbms_path[0] = '\0';
	return(du_error(errcb, E_DU2007_BAD_EXT_DB_DIR, 4,
			0, area_name, 0, db_name));
      }
    }
    else
    {
      errcb->du_clerr  = LOingpath(area_name, db_name, LO_DB, &LOtmp);
      if (errcb->du_clerr  == OK)
      {
          LOtos(&LOtmp, &cp);
          STcopy(cp, dbms_path);
      }
      else
      {
       	  dbms_path[0] = '\0';
	  return(du_error(errcb, E_DU2007_BAD_EXT_DB_DIR, 4, 
	                 	0, area_name, 0, db_name));
      }
    }
    return ( (DU_STATUS) E_DU_OK);
}


/* Build a work location path for a database. */
STATUS
du_work_ingpath(area_name, db_name, dbms_path,errcb)
char        area_name[];
char	    db_name[];
char	    dbms_path[];
DU_ERROR    *errcb;
{
    LOCATION	    LOtmp;
    char	    *cp;

    errcb->du_clerr  = LOingpath(area_name, db_name, LO_WORK, &LOtmp);
    if (errcb->du_clerr  == OK)
    {
        LOtos(&LOtmp, &cp);
        STcopy(cp, dbms_path);
    }
    else
    {
	dbms_path[0] = '\0';
	return(du_error(errcb, E_DU2007_BAD_EXT_DB_DIR, 4, 
			0, area_name, 0, db_name));
    }
    return ( (DU_STATUS) E_DU_OK);
}

STATUS
du_fjnl_ingpath(area_name, db_name, dbms_path,errcb)
char	    area_name[];
char	    db_name[];
char	    dbms_path[];
DU_ERROR    *errcb;
{
    LOCATION	    LOtmp;
    char	    *cp;

    errcb->du_clerr  = LOingpath(area_name, db_name, JNLFULL, &LOtmp);
    if (errcb->du_clerr  == OK)
    {
        LOtos(&LOtmp, &cp);
        STcopy(cp, dbms_path);
    }
    else
    {
	dbms_path[0] = '\0';
	return(du_error(errcb, E_DU2008_BAD_FULL_JNL_DIR, 4, 
			0, area_name, 0, db_name));
    }
    return ( (DU_STATUS) E_DU_OK);
}

STATUS
du_jnl_ingpath(area_name, db_name, dbms_path,errcb)
char	    area_name[];
char	    db_name[];
char	    dbms_path[];
DU_ERROR    *errcb;
{
    LOCATION	    LOtmp;
    char	    *cp;

    errcb->du_clerr  = LOingpath(area_name, db_name, LO_JNL, &LOtmp);
    if (errcb->du_clerr  == OK)
    {
        LOtos(&LOtmp, &cp);
        STcopy(cp, dbms_path);
    }
    else
    {
	dbms_path[0] = '\0';
	return(du_error(errcb, E_DU2009_BAD_JNL_DIR, 4, 
			0, area_name, 0, db_name));
    }
    return ( (DU_STATUS) E_DU_OK);
}

/*terminator*/
STATUS
du_dmp_ingpath(area_name, db_name, dbms_path,errcb)
char	    area_name[];
char	    db_name[];
char	    dbms_path[];
DU_ERROR    *errcb;
{
    LOCATION	    LOtmp;
    char	    *cp;

    errcb->du_clerr  = LOingpath(area_name, db_name, LO_DMP, &LOtmp);
    if (errcb->du_clerr  == OK)
    {
        LOtos(&LOtmp, &cp);
        STcopy(cp, dbms_path);
    }
    else
    {
	dbms_path[0] = '\0';
	return(du_error(errcb, E_DU2415_BAD_DMP_DIR, 4, 
			0, area_name, 0, db_name));
    }
    return ( (DU_STATUS) E_DU_OK);
}
