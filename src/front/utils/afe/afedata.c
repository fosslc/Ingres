/*
** Copyright (c) 2004 Ingres Corporation
*/

/*Jam hints
**
LIBRARY = IMPEMBEDLIBDATA
**
*/
# include	<compat.h>
# include       <gl.h>
# include       <sl.h>
# include       <iicommon.h>
# include       <fe.h>
# include       <adf.h>
# include       <afe.h>
# include       <er.h>

/*
** Name:        afedata.c
**
** Description: Global data for afe facility.
**
** History:
**
**      26-sep-96 (mcgem01)
**          Created.
**	18-dec-1996 (canor01)
**	    Move definition of AGNAMES structure to afe.h. Add 
**	    AGNAMES_COUNT for size of Afe_agtab array.
**	29-Sep-2004 (drivi01)
**	    Added LIBRARY jam hint to put this file into IMPEMBEDLIBDATA
**	    in the Jamfile.
*/

/* afagname.c */

/*
** This contains the generic information for each of the known aggregates.
** The ag_fid for the record is not known until the applicable type
** is known.
*/
GLOBALDEF AGNAMES       Afe_agtab[AGNAMES_COUNT] =
{
        {{-1, ADI_NOOP, "any", "Any", "Any", FALSE}, {0,0,0,0}, {0, NULL}},
        {{-1, -1, "avg", "Average", "Avg", FALSE}, {0,0,0,0}, {0, NULL}},
        {{-1, -1, "avgu", "Average (Unique)", "Avg(U)", TRUE}, {0,0,0,0}, {0, NULL}},
        {{-1, -1, "count", "Count", "N", FALSE}, {0,0,0,0}, {0, NULL}},
        {{-1, -1, "countu", "Count (Unique)", "N(U)", TRUE}, {0,0,0,0}, {0, NULL
}},
        {{-1, -1, "max", "Maximum", "Max", FALSE}, {0,0,0,0}, {0, NULL}},
        {{-1, -1, "min", "Minimum", "Min", FALSE}, {0,0,0,0}, {0, NULL}},
        {{-1, -1, "sum", "Sum", "Sum", FALSE}, {0,0,0,0}, {0, NULL}},
        {{-1, -1, "sumu", "Sum (Unique)", "Sum(U)", TRUE}, {0,0,0,0}, {0, NULL}}
,
        {{-1, ADI_NOOP, "", "", "", FALSE}, {0,0,0,0}, {0, NULL}}
};
