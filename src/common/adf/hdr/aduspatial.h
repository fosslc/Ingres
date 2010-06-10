#ifndef __ADUSPATIAL_H_INCLUDED
#define __ADUSPATIAL_H_INCLUDED
/*
** Copyright (c) 2008 Ingres Corporation
*/

/*
** Name: ADUSPATIAL.H - defines the internal representation for the new
**                    INGRES spatial datatypes. These datatypes are 
**                    open source, and OGC v 1.2 complaint.
**
** Description:
**        This file defines the interal representation for the Ingres
**	spatial datatypes:
**           - point   (DB_PT_TYPE)
**
** Notes:
**     1. Specific error codes for spatial are stored in adf.h and eradf.msg.
**     2. Trace point is ADF_013_PT_TRACE defined in adftrace.h.
**
** History: $Log-for RCS$
**      25-Nov-2008 (macde01)
**          Created.
 *      25-Aug-2009 (troal01)
 *          Added struct and table for geometry types
**/

/*
[@forward_type_references@]
[@forward_function_references@]
*/


/*
**  Defines of other constants.
*/

/* Constants required by OGC standards */
# define        ADF_PT_COORD_DIM        2
# define        ADF_PT_DIMENSION        0
# define        ADF_PT_GEOTYPE_STR      "ST_Point"

/*
[@global_variable_references@]
*/


/*}
** Name: AD_PT_INTRNL - Internal representation for point.
**
** Description:
**        This struct defines the internal representation for point.
**
** History:
**      25-Nov-2008 (macde01)
**          Created.
*/

typedef struct
{
    double    x;
    double    y;
} AD_PT_INTRNL;

/*  [@type_definitions@]    */

/* Define the length of internal point representation. */
# define	ADF_PT_LEN 		sizeof (AD_PT_INTRNL)

/* Define output length of string  */
/*    - first need to decide length of each number, width and precision */
/*    - then each string needs '('xcoordinate','ycoordinate')'          */
/* If this number changes, need to manually update all entries in       */
/* src/common/adf/adg/fi_defn.txt                                       */
#define         AD_1PT_OUTLENGTH        35 

#endif
