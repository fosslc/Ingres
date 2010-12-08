/*
** Copyright (c) 2004 Ingres Corporation
*/

#include    <compat.h>
#include    <gl.h>
#include    <cm.h>
#include    <ex.h>
#include    <me.h>
#include    <st.h>
#include    <tr.h>
#include    <cv.h>
#include    <mh.h>
#include    <nm.h>
#include    <lo.h>
#include    <iicommon.h>
#include    <adf.h>
#include    <adp.h>
#include    <ulf.h>
#include    <adftrace.h>
#include    <adfint.h>
#include    <aduint.h>
#include    <spatial.h>
#ifdef _WITH_GEO
#include    <geos_c.h>
#include    <proj_api.h>
#endif
/*
**  Name: ADUSPATIAL.C - implements ADF function instances for spatial datatypes.
**
**  Description:
**        This file contains the routines that implement the ADF function
**        instances for the point datatype (DB_PT_TYPE).
**
**  History:
**      01-Dec-2008 (macde01)
**          Created.
**      28-Feb-2009 (thich01)
**          Added geo_fromText and geos reporting function stubs.
**      02-Mar-2009 (thich01)
**          GEOS errors to tracepoint logs.
**      06-Mar-2009 (thich01)
**          Added type specific routines to match function calls and wkt passed
**      13-Mar-2009 (thich01)
**          Added asText and asBinary and supporting functions
**      20-Mar-2009 (thich01)
**          Added fromWkb and changed to store in Wkb.
**      16-Apr-2009 (thich01)
**          Add bounding box to the blob.  Stream the bounding box and the
**          geometry together.  Add all supporting funcions for the streaming.
**      20-Aug-2009 (thich01)
**          Allow a geometry type to be generic and accept all other spatial
**          data.
**      25-Aug-2009 (troal01)
**          Added adu_geom_name and adu_geom_dimensions functions for the SQL
**          functions iigeomname and iigeomdimensions. Also added a table that
**          maps geometry type codes to a const char * string and dimensions
**      18-Sep-2009 (thich01)
**          Fix up fromWKB to be the same as fromWKT.
**      05-Nov-2009 (thich01)
**          Created blob to cstr and wkb to blob to allow large geometries.
**      24-Nov-2009 (thich01)
**          Fix up asBinary and asText to use the new wkb to blob.
**      15-Dec-2009 (troal01)
**          Added geosToDataValue and rewrote several functions to use the
**          same internal functions to limit the amount of maintenance required.
**      09-Mar-2010 (thich01)
**          Clean up.  Warnings fixed. NBR and Hilbert functions implemented
**          for rtree.  Memory functions changed to Ingres calls.  Envelope
**          function now extracts from the data, instead of recalculating.
**          Changed file name from adupoint.c to aduspatial.c
**      16-Mar-2010 (troal01)
**          Added populateSRS function to load a row from spatial_ref_sys
**          using one SRID.
**      22-Mar-2010 (troal01)
**          Cleanup for Windows port.
**      xx-Apr-2010 (iwawong)
**          Added issimple isempty x y
**          for the coordinate space.
**      02-Apr-2010 (thich01)
**          Updates for rtree indexing.
**      07-Apr-2010 (troal01)
**          Changes and bugfixes to many SQL functions
**      31-Aug-2010 (thich01)
**          Allow geos and proj.4 calls to be compiled out for platforms where
**          spatial is not supported.
**      18-Oct-2010 (troal01)
**          adu_within had the argument order reversed, fixed it.
**	02-Dec-2010 (gupsh01) SIR 124685
**	    Prototype cleanup.
*/

/*
 * For getting spatial_ref_sys info
 */
DB_STATUS
getSRS(ADF_CB *adf_scb, DB_SPATIAL_REF_SYS *srs, i4 srid);

/* 
 * Structure to make it easier to stream the geometry data into and out of
 * storage.  The bounding box (envelope) and actual geometry are stored in WKB
 * format in a combined stream.  The size of each stream is specified.
 */
typedef struct _storedGeom
{
    i2 version;
    i4 srid;
    i4 envelopeSize;
    i4 geomSize;
    unsigned char *combinedWKB;
} storedGeom_t;

typedef struct _geomNBR
{
    i4 x_low;
    i4 y_low;
    i4 x_hi;
    i4 y_hi;
} geomNBR_t;

/* STATIC function declarations */
static void printHEX(const unsigned char *bytes, size_t n);

static DB_STATUS adu_long_to_cstr( 
	ADF_CB	*adf_scb,
	DB_DATA_VALUE *dv_in,
	unsigned char **output,
	i4		*length);

static DB_STATUS
adu_wkbDV_to_long( 
	ADF_CB	 *adf_scb,
	DB_DATA_VALUE *dv_in,
	DB_DATA_VALUE *dv_out);

static DB_STATUS dataValueToStoredGeom( 
	ADF_CB		*adf_scb, 
	DB_DATA_VALUE	*dv_in, 
        storedGeom_t	*geomData, 
	bool		isWKB);

static DB_STATUS storedGeomToDataValue( 
	ADF_CB		*adf_scb, 
	storedGeom_t	geomData,
        DB_DATA_VALUE	*dv_out);

static DB_STATUS geosToStoredGeom( 
	ADF_CB		*adf_scb, 
	GEOSGeometry		*geometry, 
        GEOSContextHandle_t	handle, 
	storedGeom_t		*geomData,
        bool			includeEnvelope);

static DB_STATUS storedGeomToGeos( 
	ADF_CB		*adf_scb, 
	GEOSContextHandle_t	handle,
        storedGeom_t		*geomData, 
	GEOSGeometry		**geometry,
	bool			getEnv);

static DB_STATUS adu_nbrToGeos( 
	ADF_CB 			*adf_scb, 
	GEOSContextHandle_t	handle, 
	geomNBR_t		*nbr,
        GEOSGeometry		**geometry);

static DB_STATUS dataValueToGeos( 
	ADF_CB			*adf_scb,
	DB_DATA_VALUE 		*dv1, 
	GEOSContextHandle_t 	handle,
	GEOSGeometry 		**geometry,
	bool 			isWKB);

static DB_STATUS geosToDataValue( 
	ADF_CB			*adf_scb,
	GEOSGeometry 		*geometry,
	GEOSContextHandle_t	handle,
	DB_DATA_VALUE		*dv_out,
	i4			*combinedLength,
	bool			includeEnvelope);

static int interleaveBits(int odd, int even);

static int encode(int x, int y);

#ifdef _WITH_GEO

static DB_STATUS geomToGeomComparison( 
    ADF_CB           *adf_scb,
    DB_DATA_VALUE    *dv1,
    DB_DATA_VALUE    *dv2,
    DB_DATA_VALUE    *rdv,
    /* GEOS function to call */
    char (*GEOSFunction) (GEOSContextHandle_t, const GEOSGeometry *, 
                      const GEOSGeometry *));

static DB_STATUS geomToGeomReturnsGeom(
    ADF_CB        *adf_scb,
    DB_DATA_VALUE *dv1,
    DB_DATA_VALUE *dv2,
    DB_DATA_VALUE *rdv,
    /* GEOS function to call */
    GEOSGeometry * (*GEOSFunction) (GEOSContextHandle_t, const GEOSGeometry *,
                const GEOSGeometry *));

static DB_STATUS adu_geo_fromWKB(
    ADF_CB          *adf_scb,
    DB_DATA_VALUE   *dv_in,
    DB_DATA_VALUE   *dv_srid,
    DB_DATA_VALUE   *dv_out,
    i4 expectedGeoType);

static DB_STATUS adu_geo_fromText(
    ADF_CB          *adf_scb,
    DB_DATA_VALUE   *dv_in,
    DB_DATA_VALUE   *dv_srid,
    DB_DATA_VALUE   *dv_out,
    i4 expectedGeoType);

static i4 coordSeq_cmp(
    GEOSContextHandle_t handle, const GEOSGeometry *g1,
                const GEOSGeometry *g2);

static i4 polygon_cmp(
    GEOSContextHandle_t handle, const GEOSGeometry *g1, 
               const GEOSGeometry *g2);

static i4 geom_cmp(
    GEOSContextHandle_t handle, const GEOSGeometry *g1, 
            const GEOSGeometry *g2, i4 g1Type, i4 g2Type);

#endif /* _WITH_GEO */

static DB_STATUS geom_to_text(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_out,
i4 trim,
i4 precision);

/* Debuging function that will make WKB more human readable */
static void
printHEX(const unsigned char *bytes, size_t n)
{
    static char hex[] = "0123456789ABCDEF";
    int i;

    for (i=0; i<n; i++)
    {
        TRdisplay("%c%c", hex[bytes[i]>>4], hex[bytes[i]&0x0F]);
    }
    TRdisplay("\n");
}

void
geos_Notice(const char *fmt, ...);
void
geos_Error(const char *fmt, ...);

/* This function allows GEOS to display notification messages. */
void
geos_Notice(const char *fmt, ...)
{
    va_list ap;
    char *s;

    va_start(ap, fmt);
    s = va_arg(ap, char *);    
    TRdisplay("GEOS notice: %s\n", s);
    va_end(ap);
    return;
}

/* This function allows GEOS to display error messages. */
void
geos_Error(const char *fmt, ...)
{
    va_list ap;
    char *s;

    va_start(ap, fmt);
    s = va_arg(ap, char *);    
    TRdisplay("GEOS error: %s\n", s);
    va_end(ap);
    return;
}

bool db_datatype_is_geom(i4 db_datatype)
{
    i4 i;
    i4 dt = abs(db_datatype);

    for(i = 0; geom_type_mapping[i].geom_type != GEOM_TYPE_UNDEFINED; i++)
    {
        if(dt == geom_type_mapping[i].db_type)
            return TRUE;
    }

    return FALSE;
}


/* 
 * Combine a blob into a char*
 * output is allocated here but is the responsibility of the caller to free.
*/
static DB_STATUS
adu_long_to_cstr(
ADF_CB *adf_scb,
DB_DATA_VALUE *dv_in,
unsigned char **output,
i4 *length)
{
    DB_STATUS           status = E_DB_OK;
    ADP_POP_CB          pop_cb;
    DB_DATA_VALUE       segment_dv;
    DB_DATA_VALUE       under_dv;
    DB_DATA_VALUE       cpn_dv;
    DB_DATA_VALUE       loc_dv;
    ADP_PERIPHERAL      cpn;
    ADP_PERIPHERAL      *p;
    ADF_AG_STRUCT       work;
    i4                  inbits;
    char                *segspace = NULL;
    i4                  outputPos = 0;
    i4                  lastSegLen = 0;
    unsigned char       *result;
    char                *temp;
    i4                  tempLen;
    i4                  realLen = 0;
    char                buffer[DB_GW4_MAXSTRING + DB_CNTSIZE];

    /* Get the total length of the incoming dv. */
    status = adu_size(adf_scb, dv_in, length);
    if (status != E_DB_OK)
    {
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
            "adu_long_to_cstr: couldn't get incoming length."));
    }

    /* 
     * Allocate memory for the output string.
     * +1 for the NULL delim which is not included
     * in the length
     */
    result = MEmalloc(*length + 1);
    if(result == NULL)
    {
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
            "adu_long_to_cstr: out of memory."));
    }

    /* Use a for loop to allow break outs on error. */
    for (;;)
    {
        /* Get some info on the incoming datatype. */
        status = adi_dtinfo(adf_scb, dv_in->db_datatype, &inbits);
        if (status)
            break;
        if (inbits & AD_LOCATOR)
        {
            loc_dv = *dv_in;
            loc_dv.db_length = sizeof(ADP_PERIPHERAL);
            loc_dv.db_data = (PTR)&cpn;
            loc_dv.db_datatype = abs(dv_in->db_datatype)==DB_LCLOC_TYPE
                                ? DB_LVCH_TYPE
                                : abs(dv_in->db_datatype)==DB_LNLOC_TYPE
                                ? DB_LNVCHR_TYPE : DB_LBYTE_TYPE;
            if (status = adu_locator_to_cpn(adf_scb, dv_in, &loc_dv))
                break;
            dv_in = &loc_dv;
            if (status = adi_dtinfo(adf_scb, dv_in->db_datatype, &inbits))
                break;
        }
        /* Get the underlying data type of the blob segments. */
        status = adi_per_under(adf_scb, dv_in->db_datatype, &under_dv);
        cpn_dv.db_datatype = dv_in->db_datatype;
        if (status)
            break;

        /* Setup the basics for the segments */
        pop_cb.pop_length = sizeof(pop_cb);
        pop_cb.pop_type = ADP_POP_TYPE;
        pop_cb.pop_ascii_id = ADP_POP_ASCII_ID;
        pop_cb.pop_temporary = ADP_POP_TEMPORARY;
        pop_cb.pop_underdv = &under_dv;
        under_dv.db_data = 0;
        pop_cb.pop_segment = &segment_dv;
        STRUCT_ASSIGN_MACRO(under_dv, segment_dv);
        pop_cb.pop_coupon = &cpn_dv;
        cpn_dv.db_length = sizeof(ADP_PERIPHERAL);
        cpn_dv.db_prec = 0;
        cpn_dv.db_data = (PTR) 0;
        pop_cb.pop_info = 0;

        /*
         * Allocate a work area for the segments.  The internal maximum is fine
         * here as each segement will be smaller than the max.
         */
        if (segspace == NULL)
        {
            segspace = (char *)MEreqmem(0, DB_GW4_MAXSTRING + DB_CNTSIZE,
                                        FALSE, NULL);
        }
        work.adf_agwork.db_data = segspace;

        /* More segment setup. */
        p = (ADP_PERIPHERAL *) dv_in->db_data;
        cpn_dv.db_data = (PTR) p;

        pop_cb.pop_segment = &work.adf_agwork;
        pop_cb.pop_continuation = ADP_C_BEGIN_MASK;
        pop_cb.pop_segment->db_datatype = under_dv.db_datatype;
        pop_cb.pop_segment->db_length = under_dv.db_length;
        pop_cb.pop_segment->db_prec = under_dv.db_prec;

        /* Start looping through the segments. */
        do
        {
            /* Get a segment from disk. */
            status = (*Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)(
                                ADP_GET, &pop_cb);
            if (status)
            {
                if (DB_FAILURE_MACRO(status) ||
                        (pop_cb.pop_error.err_code != E_AD7001_ADP_NONEXT))
                {
                    adf_scb->adf_errcb.ad_errcode =
                                    pop_cb.pop_error.err_code;
                    break;
                }
            }
            pop_cb.pop_continuation &= ~ADP_C_BEGIN_MASK;

            /*
             * Get the actual data out of the segment since some types include
             * other information.
             */
            if (status = adu_3straddr(adf_scb, pop_cb.pop_segment, &temp))
                break;

            /* Get the size of the actual data. */
            if (status = adu_size(adf_scb, pop_cb.pop_segment, &tempLen))
                break;

            /* Keep track of the size of the acutal data. */
            realLen += tempLen;

            /* Concat each segment into one location */
            if((outputPos + tempLen) > *length)
            {
                /*
                 * Ensure the last segment doesn't go over the allocated
                 * size.
                 */
                lastSegLen = *length - outputPos;
                MEcopy((unsigned char*)temp, 
                       lastSegLen,
                       &result[outputPos]);
                outputPos += lastSegLen;
            }
            else
            {
                MEcopy((unsigned char*)temp, 
                       tempLen,
                       &result[outputPos]);
                outputPos += (tempLen);
            }
        } while (DB_SUCCESS_MACRO(status) &&
                    (pop_cb.pop_error.err_code != E_AD7001_ADP_NONEXT));

        if (pop_cb.pop_error.err_code == E_AD7001_ADP_NONEXT)
            status = E_DB_OK;
        break;
    }

    /* Clean up the work area. */
    if (segspace)
        MEfree((PTR)segspace);

    /* Set the output. */
    *length = realLen;
    /* NULL delimit the result: */
    result[realLen] = '\0';
    *output = result;

    return(status);
}

/* Take a wkb stream in data value form and turn it into a segmented blob.*/
static DB_STATUS
adu_wkbDV_to_long(
ADF_CB *adf_scb,
DB_DATA_VALUE *dv_in,
DB_DATA_VALUE *dv_out)
{
    DB_STATUS       status;
    ADP_POP_CB      pop_cb;
    DB_DATA_VALUE   segment_dv;
    DB_DATA_VALUE   under_dv;
    DB_DATA_VALUE   cpn_dv;
    DB_DATA_VALUE   loc_dv;
    ADP_PERIPHERAL  cpn;
    ADP_PERIPHERAL  *p;
    ADF_AG_STRUCT   work;
    i4              length;
    i4              inbits;
    i4              outbits;
    char            *segspace = NULL;
    i4              done;
    i4              bytes;

    if (Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi == NULL)
        return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0,
            "lvch fexi null"));

    /* Use a for loop to allow break outs on error. */
    for (;;)
    {
        /* 
         * Initialize this to null as it may be allocated later and this will
         * ensure proper cleanup on break.
         */
        segment_dv.db_data = NULL;
        /* Get some info on the incoming datatype. */
        status = adi_dtinfo(adf_scb, dv_in->db_datatype, &inbits);
        if (status)
            break;
        if (inbits & AD_LOCATOR)
        {
            loc_dv = *dv_in;
            loc_dv.db_length = sizeof(ADP_PERIPHERAL);
            loc_dv.db_data = (PTR)&cpn;
            loc_dv.db_datatype = abs(dv_in->db_datatype) == DB_LCLOC_TYPE ?
                DB_LVCH_TYPE : abs(dv_in->db_datatype) == DB_LNLOC_TYPE ?
                DB_LNVCHR_TYPE : DB_LBYTE_TYPE;
            if (status = adu_locator_to_cpn(adf_scb, dv_in, &loc_dv))
                break;
            dv_in = &loc_dv;
            if (status = adi_dtinfo(adf_scb, dv_in->db_datatype, &inbits))
                break;
        }
        /* Get some info on the outgoing datatype. */
        status = adi_dtinfo(adf_scb, dv_out->db_datatype, &outbits);
        if (status)
            break;

        /* Get the underlying data type of the blob segments. */
        if (inbits & AD_PERIPHERAL)
        {
            status = adi_per_under(adf_scb, dv_in->db_datatype, &under_dv);
            cpn_dv.db_datatype = dv_in->db_datatype;
        }
        else
        {
            status = adi_per_under(adf_scb, dv_out->db_datatype, &under_dv);
            cpn_dv.db_datatype = dv_out->db_datatype;
        }
        if (status)
            break;

        /* Setup the basics for the segments */
        pop_cb.pop_length = sizeof(pop_cb);
        pop_cb.pop_type = ADP_POP_TYPE;
        pop_cb.pop_ascii_id = ADP_POP_ASCII_ID;
        /* This is irrelevant if the output side isn't long */ 
        pop_cb.pop_temporary = ADP_POP_TEMPORARY;
        pop_cb.pop_underdv = &under_dv;
        under_dv.db_data = 0;
        pop_cb.pop_segment = &segment_dv;
        STRUCT_ASSIGN_MACRO(under_dv, segment_dv);
        pop_cb.pop_coupon = &cpn_dv;
        cpn_dv.db_length = sizeof(ADP_PERIPHERAL);
        cpn_dv.db_prec = 0;
        cpn_dv.db_data = (PTR) 0;
        pop_cb.pop_info = 0;

        /*
         * Allocate a work area for the segments. Do not use the max size here
         * as some geometries will be larger than the internal max.
         */
        if (segspace == NULL)
        {
            segspace = (char *)MEreqmem(0, dv_in->db_length, FALSE, NULL);
        }
        work.adf_agwork.db_data = segspace;
    
        if ((inbits & AD_PERIPHERAL) == 0)
        {
            /* not-long --> long */

            /* More segment setup. */
            p = (ADP_PERIPHERAL *) dv_out->db_data;
            p->per_tag = ADP_P_COUPON;
            cpn_dv.db_data = dv_out->db_data;
            MEfill(sizeof(p->per_value.val_coupon), 0, 
                (char *)&(p->per_value.val_coupon));

            work.adf_agwork.db_length = dv_in->db_length;
            work.adf_agwork.db_datatype = under_dv.db_datatype;
            work.adf_agwork.db_prec = under_dv.db_prec;

            /*
             * No coercion required here as all wkb streams are vbytes
             * effectively.
             */
            MECOPY_VAR_MACRO((PTR) dv_in->db_data, (u_i4)dv_in->db_length,
                (PTR) work.adf_agwork.db_data);
            work.adf_agwork.db_length = dv_in->db_length;
            work.adf_agwork.db_prec = dv_in->db_prec;
            dv_in = &work.adf_agwork;

            pop_cb.pop_continuation = ADP_C_BEGIN_MASK;
            /*
             * Copy a piece into segment memory.  This is to ensure there are no
             * missed or doubled up bytes.
             */
            segment_dv.db_data = MEmalloc(under_dv.db_length);
            if(segment_dv.db_data == NULL)
                break;
            if((dv_in->db_length + DB_CNTSIZE) > under_dv.db_length)
            {
                MEcopy(dv_in->db_data, under_dv.db_length - DB_CNTSIZE,
                       &segment_dv.db_data[DB_CNTSIZE]);
            }
            else
            {
                MEcopy(dv_in->db_data, dv_in->db_length,
                       &segment_dv.db_data[DB_CNTSIZE]);
            }
            p->per_length0 = 0;
            /* Set the actual length of the data in the coupon information. */
            p->per_length1 = dv_in->db_length;
            /* 
             * Initialize a counter so it can keep track of how much more data
             * is to be segmented.
             */
            length = dv_in->db_length;
            /* Loop through each segment. */
            do
            {
                if (segment_dv.db_datatype == DB_NVCHR_TYPE)
                    bytes = length * sizeof(UCS2);
                else
                    bytes = length;
    
                /*
                 * If more data is left than can be held in the under lying data
                 * type, use the under type length.
                 */
                if ((bytes + 2) > under_dv.db_length)
                {
                    /* Set the size of the segment. */
                    segment_dv.db_length = under_dv.db_length;
                    /* Set the size of the actual data in the segment. */
                    if (segment_dv.db_datatype == DB_NVCHR_TYPE)
                        ((DB_TEXT_STRING *) segment_dv.db_data)->db_t_count =
                            (under_dv.db_length - 2) / sizeof(UCS2);
                    else
                        ((DB_TEXT_STRING *) segment_dv.db_data)->db_t_count =
                            under_dv.db_length - 2;
                }
                else
                {
                    /*
                     * Last segment is smaller than underlying data type so use
                     * its size.
                     */
                    segment_dv.db_length = bytes + 2;
                    ((DB_TEXT_STRING *) segment_dv.db_data)->db_t_count = bytes;
                    pop_cb.pop_continuation |= ADP_C_END_MASK;
                }
                /* Write the segment to disk. */
                status = (*Adf_globs->Adi_fexi[ADI_01PERIPH_FEXI].adi_fcn_fexi)
                            (ADP_PUT, &pop_cb);
                pop_cb.pop_continuation &= ~ADP_C_BEGIN_MASK;
                if (status)
                {
                    adf_scb->adf_errcb.ad_errcode = pop_cb.pop_error.err_code;
                    break;
                }
                if (pop_cb.pop_continuation & ADP_C_END_MASK)
                    break;

                /* Subtract the length of the last segment from the total. */
                done = ((DB_TEXT_STRING *) segment_dv.db_data)->db_t_count;
                length = length - done;

                /* Copy the next bit of data into the segment memory. */
                if(length + 2 > under_dv.db_length)
                    MEcopy(&dv_in->db_data[dv_in->db_length - length], 
                           under_dv.db_length - DB_CNTSIZE,
                           &segment_dv.db_data[DB_CNTSIZE]);
                else                
                    MEcopy(&dv_in->db_data[dv_in->db_length - length], 
                           length,
                           &segment_dv.db_data[DB_CNTSIZE]);
            } while (DB_SUCCESS_MACRO(status) &&
                    ((pop_cb.pop_continuation & ADP_C_END_MASK) == 0));
        }
        else
        {
            if (segspace)
                MEsysfree((PTR)segspace);
            return(adu_error(adf_scb, E_AD9998_INTERNAL_ERROR, 2, 0,
                "lvch dt diff"));
        }
        break;
    }

    /* Clean up allocated memory. */
    if (segspace)
        MEfree((PTR)segspace);
    if (segment_dv.db_data)
        MEsysfree(segment_dv.db_data);

    return(status);
}

/* Convert a blob stream into a stored geometry structure. */
static DB_STATUS
dataValueToStoredGeom(ADF_CB *adf_scb, DB_DATA_VALUE *dv_in, 
                      storedGeom_t *geomData, bool isWKB)
{
    DB_STATUS status = E_DB_OK;
    i4 geomDataSize;
    unsigned char *geoWKB = NULL;

    /*
     * Set the combinedWKB holder to NULL to ensure it's not cleaned up 
     * later.
     */
    geomData->combinedWKB = NULL;

    /* 
     * Since blobs are not guaranteed to store the WKB stream in one concurrent
     * variable convert from blob to char so that the WKB stream is all 
     * together.
     */
    status = adu_long_to_cstr(adf_scb, dv_in, &geoWKB, &geomDataSize);
    if(status != E_DB_OK)
    {
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2,0,
                    "dataValueToStoredGeom: Bad copy from lbyte to char."));
    }

    if(!isWKB)
    {
        /* 
         * Break the stream up into the appropriate stucture.
         * First the version so we know how to proceed
         */
        I2ASSIGN_MACRO(geoWKB[0], geomData->version);

        switch(geomData->version)
        {
        case SPATIAL_DATA_V1:
            I4ASSIGN_MACRO(geoWKB[sizeof(i2)], geomData->srid);
            I4ASSIGN_MACRO(geoWKB[sizeof(i2) + sizeof(i4)], geomData->envelopeSize);
            I4ASSIGN_MACRO(geoWKB[sizeof(i2) + sizeof(i4) * 2], geomData->geomSize);
            break;
        default:
            if(geoWKB != NULL)
            {
                free(geoWKB);
                geoWKB = NULL;
            }
            return adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
                    "dataValueToStoredGeom: invalid data version");
        }
    }
    else
    {
        geomData->version = CURRENT_SPATIAL_DATA_V;
        geomData->envelopeSize = 0;
        geomData->srid = -1;
        geomData->geomSize = geomDataSize;
    }
    /* Move the pointer past the two i4s and get the combined WKB stream. */
    geomData->combinedWKB = MEmalloc(geomData->envelopeSize + 
                                     geomData->geomSize);
    if(geomData->combinedWKB == NULL)
    {
        if(geoWKB != NULL)
        {
            MEsysfree(geoWKB);
            geoWKB = NULL;
        }
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2,0,
   "dataValueToStoredGeom: couldn't allocate memory to geomData->combinedWKB."
               ));
    }

    if(!isWKB)
    {
        switch(geomData->version)
        {
        case SPATIAL_DATA_V1:
            MEcopy(&geoWKB[sizeof(i2) + sizeof(i4)*3],
                   (geomData->envelopeSize + geomData->geomSize),
                   geomData->combinedWKB);
            break;
        default:
            if(geoWKB != NULL)
            {
                free(geoWKB);
                geoWKB = NULL;
            }
            return adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
                    "dataValueToStoredGeom: invalid data version");
        }
    }
    else
    {
        MEcopy(geoWKB, geomData->geomSize, geomData->combinedWKB);
    }

    /* Clean up the memory. */
    if(geoWKB != NULL)
    {
        MEsysfree(geoWKB);
        geoWKB = NULL;
    }
    return status;
}

/* Convert a stored geometry structure into a blob stream. */
static DB_STATUS
storedGeomToDataValue(ADF_CB *adf_scb, storedGeom_t geomData,
                      DB_DATA_VALUE *dv_out)
{
    DB_STATUS status = E_DB_OK;
    DB_DATA_VALUE dv_loc, *dv_wkb = &dv_loc;
    i4 geomDataSize, noEnvelope = 0;
    i4 srid;

    /* Set the db_data holder to NULL to ensure it's not cleaned up later. */
    dv_wkb->db_data = NULL;

    geomDataSize = geomData.envelopeSize + geomData.geomSize;

    switch(geomData.version)
    {
    case SPATIAL_DATA_V1:
        /* 
         * The blob stream includes the sizes of the two WKB streams, three i4s
         * and one i2.
         */
        dv_wkb->db_length = geomDataSize + (sizeof(i4) * 3 + sizeof(i2)) +
                            DB_CNTSIZE;
        break;
    default:
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "storedGeomToDataValue: invalid version in data."));
    }

    dv_wkb->db_datatype = DB_VBYTE_TYPE;

    dv_wkb->db_data = MEmalloc(dv_wkb->db_length);
    if(dv_wkb->db_data == NULL)
    {
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
            "storedGeomToDataValue: out of memory."));
    }

    /* Copy all the data into one continuous stream. */
    switch(geomData.version)
    {
    case SPATIAL_DATA_V1:
        MEcopy((unsigned char*)&geomData.version, sizeof(i2),
                dv_wkb->db_data);
        MEcopy((unsigned char*)&geomData.srid, sizeof(i4),
                &dv_wkb->db_data[sizeof(i2)]);        
        MEcopy((unsigned char*)&geomData.envelopeSize, sizeof(i4),
               &dv_wkb->db_data[sizeof(i2) + sizeof(i4)]);
        MEcopy((unsigned char*)&geomData.geomSize, sizeof(i4),
               &dv_wkb->db_data[sizeof(i2) + sizeof(i4)*2]);
        MEcopy(geomData.combinedWKB, geomDataSize,
               &dv_wkb->db_data[sizeof(i2) + sizeof(i4)*3]);
        break;
    default:
        if(dv_wkb->db_data != NULL)
        {
            free(dv_wkb->db_data);
            dv_wkb->db_data = NULL;
        }
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "storedGeomToDataValue: invalid version in data."));
    }

    status = adu_wkbDV_to_long( adf_scb, dv_wkb, dv_out );
    if(dv_wkb->db_data != NULL)
    {
        MEsysfree(dv_wkb->db_data);
        dv_wkb->db_data = NULL;
    }
    return status;
}

#ifdef _WITH_GEO
/* Convert a geos geometry into a stored geometry structure. */
static DB_STATUS
geosToStoredGeom(ADF_CB *adf_scb, GEOSGeometry *geometry, 
                 GEOSContextHandle_t handle, storedGeom_t *geomData,
                 bool includeEnvelope)
{
    DB_STATUS status = E_DB_OK;
    GEOSWKBWriter *wkbWriter = NULL;
    GEOSGeometry *envelopeGeometry = NULL;
    unsigned char *envelopeWKB = NULL, *geomWKB = NULL;
    i4 geomDataSize;

    /*
     * Set the combinedWKB holder to NULL to ensure it's not cleaned up 
     * later.
     */
    geomData->combinedWKB = NULL;

    /* Set version to current */
    geomData->version = CURRENT_SPATIAL_DATA_V;

    /* Get the SRID */
    geomData->srid = GEOSGetSRID_r(handle, geometry);

    /* Create a new WKB writer. */
    wkbWriter = GEOSWKBWriter_create_r(handle);

    /* Allow 3D */
    GEOSWKBWriter_setOutputDimension_r( handle, wkbWriter, 3 );

    /*
     * If the geometry is empty, the envelope would be an empty point
     * and this can not be represented as a WKB, hence we disallow
     * envelopes for these cases
     */
    if(GEOSisEmpty_r(handle, geometry))
    {
        includeEnvelope = FALSE;
    }

    if(includeEnvelope)
    {
        /* Calculate the bounding box (envelope) and convert it to WKB. */
        envelopeGeometry = GEOSEnvelope_r(handle, geometry);
        envelopeWKB = GEOSWKBWriter_write_r(handle, wkbWriter,
                                            envelopeGeometry,
                                            &geomData->envelopeSize);
        if(envelopeWKB == NULL)
        {
            GEOSWKBWriter_destroy_r(handle, wkbWriter);
            GEOSGeom_destroy_r(handle, envelopeGeometry);
            return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
                "geosToStoredGeom: Bad conversion from geom to WKB."));
        }

        /* Clean up the GEOS geometry no longer required. */
        GEOSGeom_destroy_r(handle, envelopeGeometry);
        envelopeGeometry = NULL;
    }
    else
    {
        geomData->envelopeSize = 0;
    }

    /*
     * Special handling for empty points since they have no WKB representation
     */
    if(GEOSisEmpty_r(handle, geometry) &&
            GEOSGeomTypeId_r(handle, geometry) == GEOS_POINT)
    {
        geomData->geomSize = 0;
        geomData->combinedWKB = NULL;
    }
    else
    {
        /* Convert the geometry into WKB. */
        geomWKB = GEOSWKBWriter_write_r(handle, wkbWriter, geometry,
                                        &geomData->geomSize);
        if(geomWKB == NULL)
        {
            GEOSWKBWriter_destroy_r(handle, wkbWriter);
            if(envelopeWKB != NULL)
            {
                MEsysfree(envelopeWKB);
                envelopeWKB = NULL;
            }
            /*
             * We need to figure out if it failed because the geometry had
             * empty points within it.
             */
            if(GEOSGeomTypeId_r(handle, geometry) == GEOS_GEOMETRYCOLLECTION ||
                    GEOSGeomTypeId_r(handle, geometry) == GEOS_MULTIPOINT)
            {
                const GEOSGeometry *ngeom;

                i4 i, n = GEOSGetNumGeometries_r(handle, geometry);
                for(i = 0; i < n; i++)
                {
                    ngeom = GEOSGetGeometryN_r(handle, geometry, i);
                    /*
                     * We have ourselves an empty point!
                     */
                    if(GEOSGeomTypeId_r(handle, ngeom) == GEOS_POINT &&
                            GEOSisEmpty_r(handle, ngeom))
                    {
                        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
                            "Empty Points are illegal within GEOMETRYCOLLECTION and MULTIPOINT."));
                    }
                }
            }
            return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
                "geosToStoredGeom: Bad conversion from geom to WKB."));
        }

        /* Clean up the WKB writer. */
        GEOSWKBWriter_destroy_r(handle, wkbWriter);

        geomDataSize = geomData->envelopeSize + geomData->geomSize;

        /* Put both WKB streams into one continuous stream. */
        geomData->combinedWKB = MEmalloc(geomDataSize);
        if(geomData->combinedWKB == NULL)
        {
            if(envelopeWKB != NULL)
            {
                MEsysfree(envelopeWKB);
                envelopeWKB = NULL;
            }
            if(geomWKB != NULL)
            {
                MEsysfree(geomWKB);
                geomWKB = NULL;
            }
            return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
                "geosToStoredGeom: out of memory"));
        }

        if(includeEnvelope)
        {
            MEcopy(envelopeWKB, geomData->envelopeSize, geomData->combinedWKB);
        }
        MEcopy(geomWKB, geomData->geomSize,
               &geomData->combinedWKB[geomData->envelopeSize]);
    }

    /* Clean up the separate streams. */
    if(envelopeWKB != NULL)
    {
        MEsysfree(envelopeWKB);
        envelopeWKB = NULL;
    }
    if(geomWKB != NULL)
    {
        MEsysfree(geomWKB);
        geomWKB = NULL;
    }
    return status;
}

/*
 ** Convert a storedGeometry into a GEOS geometry
 */
static DB_STATUS
storedGeomToGeos(ADF_CB *adf_scb, GEOSContextHandle_t handle,
        storedGeom_t *geomData, GEOSGeometry **geometry, bool getEnv)
{
    unsigned char *geomWKB = NULL;
    i4 geomSize = 0;
    GEOSWKBReader *wkbReader;

    if( getEnv )
    {
        /* Only convert the envelope. */
        geomSize = geomData->envelopeSize;
        geomWKB = &geomData->combinedWKB[0];
    }
    else 
    {
        geomSize = geomData->geomSize;
        geomWKB = &geomData->combinedWKB[geomData->envelopeSize];
    }

    /*
     * Special case when geomSize == 0 we have an empty point
     */
    if(geomData->geomSize == 0)
    {
        *geometry = GEOSGeom_createEmptyPoint_r(handle);
        if(*geometry == NULL)
        {
            return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
                "storedGeomToGeos: Bad conversion from POINT EMPTY to geom"));
        }
    }
    else
    {
        /* create a WKB reader. */
        wkbReader = GEOSWKBReader_create_r(handle);

        /* Create a GEOS geometry structure from the geometry WKB. */
        *geometry = GEOSWKBReader_read_r(handle, wkbReader, geomWKB, geomSize);

        /* Clean up the allocated memory that we're done with now. */
        GEOSWKBReader_destroy_r(handle, wkbReader);

        if(*geometry == NULL)
        {
            return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
                "storedGeomToGeos: Bad conversion from WKB to geom"));
        }
    }

    /* Set the SRID */
    GEOSSetSRID_r(handle, *geometry, geomData->srid);

    return E_DB_OK;
}

static DB_STATUS
adu_nbrToGeos(ADF_CB *adf_scb, GEOSContextHandle_t handle, geomNBR_t *nbr,
              GEOSGeometry **geometry)
{
    GEOSCoordSequence *coordSeq;
    GEOSGeometry *linegeo = NULL;

    /* 
     * Create a coordinate sequence to hold the NBR data so we can calculate
     * a centroid.
     */
    coordSeq = GEOSCoordSeq_create_r(handle, 2, 2);
    if(coordSeq == NULL)
    {
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
            "adu_geom_nbrToGeos: Cannot create coordinate sequence."));
    }

    /* 
     * Add the NBR data to the coord seq and create a line string from the
     * data.
     */
    GEOSCoordSeq_setX_r(handle, coordSeq, 0, nbr->x_low);
    GEOSCoordSeq_setY_r(handle, coordSeq, 0, nbr->y_low);
    GEOSCoordSeq_setX_r(handle, coordSeq, 1, nbr->x_hi);
    GEOSCoordSeq_setY_r(handle, coordSeq, 1, nbr->y_hi);
    linegeo = GEOSGeom_createLineString_r(handle, coordSeq);
    if(linegeo == NULL)
    {
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
            "adu_geom_nbrToGeos: Cannot create linestring"));
    }
    *geometry = GEOSEnvelope_r(handle, linegeo);
    if(*geometry == NULL)
    {
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
            "adu_geom_nbrToGeos: Cannot create envelope"));
    }
    GEOSGeom_destroy_r(handle, linegeo);

    return E_DB_OK;
}

/*
 * Takes a geometry type DB_DATA_VALUE and converts it into a GEOS geometry
 * adf_scb is just passed through
 * dv1 is the data value you want converted into a GEOSGeometry
 * handle is the GEOS handle you must have initialized with initGEOS_r
 * a GEOSGeometry pointer that will be filled
 * isWKB should be set to true if the data in dv1 is in WKB format and not
 *   Ingres' internal combinedWKB format.
 */
static DB_STATUS
dataValueToGeos(
ADF_CB *adf_scb,
DB_DATA_VALUE *dv1,
GEOSContextHandle_t handle,
GEOSGeometry **geometry,
bool isWKB)
{
    DB_STATUS status = E_DB_OK;
    storedGeom_t geomData;
    geomNBR_t *nbr;

    if (dv1->db_datatype == DB_NBR_TYPE)
    {
        nbr = (geomNBR_t *)dv1->db_data;
        status = adu_nbrToGeos(adf_scb, handle, nbr, geometry);
        if( status != E_DB_OK )
        {
            return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "dataValueToGeos: Failed to convert nbr to geos geometry."));
        }
        return status;    
    }

    status = dataValueToStoredGeom(adf_scb, dv1, &geomData, isWKB);
    if(status != E_DB_OK)
    {
        if(geomData.combinedWKB != NULL)
        {
            MEsysfree(geomData.combinedWKB);
            geomData.combinedWKB = NULL;
        }
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
        "dataValueToGeos: Failed to destream data value into stored geometry"));
    }

    status = storedGeomToGeos(adf_scb, handle, &geomData, geometry, FALSE);

    if(geomData.combinedWKB != NULL)
    {
        MEsysfree(geomData.combinedWKB);
        geomData.combinedWKB = NULL;
    }
    if(status != E_DB_OK)
    {
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "dataValueToGeos: Failed to convert storedGeom to GEOSGeometry"));
    }

    return E_DB_OK;
}

/*
 * Takes a GEOSGeometry pointer and turns it into a DB_DATA_VALUE
 * adf_scb, just pass through
 * geometry, contains the geometry you want to convert
 * handle is a handle you must have run initGEOS_r on already
 * dv_out is a pointer to a DB_DATA_VALUE struct you want to load
 *   the geometry into
 * combinedLength, a i4 pointer which will be used to write the length
 *   of geomSize and envelopeSize to, may be NULL if this value isn't
 *   needed
 * includeEnvelope, whether or not you want the envelope to be included
 *   in dv_out->db_data. If the data is to be directly inserted it should
 *   be included.
 */
static DB_STATUS
geosToDataValue(
ADF_CB *adf_scb,
GEOSGeometry *geometry,
GEOSContextHandle_t handle,
DB_DATA_VALUE *dv_out,
i4 *combinedLength,
bool includeEnvelope)
{
    DB_STATUS status = E_DB_OK;
    storedGeom_t geomData;

    status = geosToStoredGeom(adf_scb, geometry, handle, &geomData,
                              includeEnvelope);
    if(status != E_DB_OK)
    {
        if(geomData.combinedWKB != NULL)
        {
            MEsysfree(geomData.combinedWKB);
            geomData.combinedWKB = NULL;
        }
        /* Error code/message handled in geosToStoredGeom */
        return status;
    }

    /* calculate combined length of stored geometry */
    if(combinedLength != NULL)
    {
        *combinedLength = geomData.envelopeSize + geomData.geomSize;
    }

    status = storedGeomToDataValue(adf_scb, geomData, dv_out);
    if(status != E_DB_OK)
    {
        if(geomData.combinedWKB != NULL)
        {
            MEsysfree(geomData.combinedWKB);
            geomData.combinedWKB = NULL;
        }
        /* Error code/message handled in storedGeomToDataValue */
        return status;
    }

    if(geomData.combinedWKB != NULL)
    {
        MEsysfree(geomData.combinedWKB);
        geomData.combinedWKB = NULL;
    }

    return E_DB_OK;
}
#endif

/*
 * Convert the envelope included in the streamed geom data into an NBR used for
 * rtree indexing.
 */
DB_STATUS
adu_geom_nbr(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1_in,
DB_DATA_VALUE   *dv2_in,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    storedGeom_t geomData;
    GEOSContextHandle_t handle;
    GEOSWKBReader *wkbReader;
    GEOSGeometry *geometry = NULL;
    const GEOSGeometry *ring = NULL;
    const GEOSCoordSequence *coordSeq = NULL;
    double x_low, y_low, x_hi, y_hi;
    geomNBR_t nbr;
    i4 geoType;
    unsigned int cs_size;

    /*
     * If either input is NULL, return NULL
     */
    if(ADI_ISNULL_MACRO(dv1_in) || ADI_ISNULL_MACRO(dv2_in))
    {
        ADF_SETNULL_MACRO(dv_out);
        return E_DB_OK;
    }

    /* 
     * If an NBR is coming in, then there's no work to be done.  This can happen
     * during index creation.  After the initial call using the GEOM data this
     * function is called again when writing the index table.  The second call
     * uses the NBR returned from the first call.
     */
    if (dv1_in->db_datatype == DB_NBR_TYPE)
    {
        dv_out = dv1_in;
    }
    else
    {
        /* Convert the streamed data into a more usable structure. */
        status = dataValueToStoredGeom(adf_scb, dv1_in, &geomData, FALSE);
        if(status != E_DB_OK)
        {
            return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_geom_nbr: Failed to convert data value to stored geometry."));
        }

        /*
         * The envelope size is zero if we have an empty geometry
         * just return NULL
         */
        if(geomData.envelopeSize == 0)
        {
            ADF_SETNULL_MACRO(dv_out);
            return E_DB_OK;
        }
        /* Initialize geos */
        handle = initGEOS_r(geos_Notice, geos_Error);

        /* create a WKB reader. */
        wkbReader = GEOSWKBReader_create_r(handle);

        /* Create a GEOS geometry structure from the geometry WKB. */
        geometry = GEOSWKBReader_read_r(handle, wkbReader, 
                                        geomData.combinedWKB,
                                        geomData.envelopeSize);
        /* Done with the reader and geometry structure, clean up. */
        GEOSWKBReader_destroy_r(handle, wkbReader);
        if(geomData.combinedWKB != NULL)
        {
           MEsysfree(geomData.combinedWKB);
           geomData.combinedWKB = NULL;
        }
        if(geometry == NULL)
        {
            finishGEOS_r(handle);
            return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
                    "adu_geom_nbr: Bad conversion from WKB to geom"));
        }

        geoType = GEOSGeomTypeId_r(handle, geometry);
        if(geoType == GEOS_POLYGON)
        {
            /* 
             * Turn the envelope polygon into a ring type so we can extract the
             * coordinate sequence.
             */
            ring = GEOSGetExteriorRing_r(handle, geometry);
            if(ring == NULL)
            {
                GEOSGeom_destroy_r(handle, geometry); 
                finishGEOS_r(handle);
                return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
                    "adu_geom_nbr: Failed to extract ring."));
            }
        }
        /* It's an evelope of a point so make the ring equal to the geometry. */
        else
            ring = geometry;

        /* Get the coord seq so we can extract the x and y values. */
        coordSeq = GEOSGeom_getCoordSeq_r(handle, ring);
        if(coordSeq == NULL)
        {
            GEOSGeom_destroy_r(handle, geometry); 
            finishGEOS_r(handle);
            return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
                    "adu_geom_nbr: Failed to extract coord seq."));
        }

        /* 
         * Get the first and third points for the min x,y and max x,y of an
         * NBR.  Convert the doubles to integers as NBR is int based.
         */
        GEOSCoordSeq_getX_r(handle, coordSeq, 0, &x_low);
        GEOSCoordSeq_getY_r(handle, coordSeq, 0, &y_low);
        nbr.x_low = x_low;
        nbr.y_low = y_low;

        if(geoType == GEOS_POLYGON)
        {
            GEOSCoordSeq_getX_r(handle, coordSeq, 2, &x_hi);
            GEOSCoordSeq_getY_r(handle, coordSeq, 2, &y_hi);
            nbr.x_hi = x_hi;
            nbr.y_hi = y_hi;
        }
        else
        {
            /* 
             * An envelope of a point will only be a point.  Make the nbr hi
             * equal to the lows.
             */
            nbr.x_hi = x_low;
            nbr.y_hi = y_low;
        }
        /* 
         * Simple round up.  Since the nbr is stored in integers, any decimals
         * should be rounded up to include them in the nbr. e.g. 2.12 becomes
         * 3.  But 2.00 stays 2.
         */
        if(x_low > nbr.x_low) nbr.x_low += 1;
        if(y_low > nbr.y_low) nbr.y_low += 1;
        if(x_hi > nbr.x_hi) nbr.x_hi += 1;
        if(y_hi > nbr.y_hi) nbr.y_hi += 1;

        dv_out->db_length = sizeof(geomNBR_t);
        MEcopy((unsigned char*)&nbr, dv_out->db_length, dv_out->db_data);

        /* Clean up all the geos items hanging around. */
        GEOSGeom_destroy_r(handle, geometry); 
        finishGEOS_r(handle);
    }

    return status;
#endif
}

/*
 * Interleave the bits from two input integer values
 * @param odd integer holding bit values for odd bit positions
 * @param even integer holding bit values for even bit positions
 * @return the integer that results from interleaving the input bits
 */ 
static int interleaveBits(int odd, int even)
{
    int val = 0;
    int m = (odd >= even) ? odd:even;
    int n = 0;
    int i, bitMask, a, b;

    while (m > 0)
    {
      n++;
      m >>= 1;
    }

    for (i = 0; i < n; i++)
    {
        int bitMask = 1 << i;
        int a = (even & bitMask) > 0 ? (1 << (2*i)) : 0;
        int b = (odd & bitMask) > 0 ? (1 << (2*i+1)) : 0;
        val += a + b;
    }

    return val;
}

/*
 * Find the Hilbert order (=vertex index) for the given grid cell 
 * coordinates.
 * @param x cell column (from 0)
 * @param y cell row (from 0)
 * @param r resolution of Hilbert curve (grid will have Math.pow(2,r) 
 * rows and cols)
 * @return Hilbert order 
 */ 
static int encode(int x, int y)
{
    int r = 24;
    int mask = (1 << r) - 1;
    int hodd = 0;
    int heven = x ^ y;
    int notx = ~x & mask;
    int noty = ~y & mask;
    int temp = notx ^ y;
    int k;
    int v0 = 0, v1 = 0;

    for (k = 1; k < r; k++)
    {
        v1 = ((v1 & heven) | ((v0 ^ noty) & temp)) >> 1;
        v0 = ((v0 & (v1 ^ notx)) | (~v0 & (v1 ^ noty))) >> 1;
    }
    hodd = (~v0 & (v1 ^ x)) | (v0 & (v1 ^ noty));

    return interleaveBits(hodd, heven);
}

/* Calculate a hilbert value based on an NBR. */
DB_STATUS
adu_geom_hilbert(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    GEOSContextHandle_t handle;
    GEOSGeometry *tempLine = NULL, *geometry = NULL, *centroid = NULL;
    geomNBR_t *nbr;
    double x, y;
    i8 hilbertVal;

    /*
     * If input is NULL, return NULL
     */
    if(ADI_ISNULL_MACRO(dv_in))
    {
        ADF_SETNULL_MACRO(dv_out);
        return E_DB_OK;
    }

    /* Cast the data into the known NBR structure. */
    nbr = (geomNBR_t *)dv_in->db_data;
    /* Initialize GEOS */
    handle = initGEOS_r(geos_Notice, geos_Error);

    status = adu_nbrToGeos(adf_scb, handle, nbr, &geometry);
    if( status != E_DB_OK )
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
        "adu_geom_hilbert: Failed to convert nbr to geos geometry."));
    }    

    /* 
     * Calculate the centroid.  This works from a line since we're always
     * working with rectangles in 2D.
     */
    centroid = GEOSGetCentroid_r(handle, geometry);

    /* Get the x and y values of the centroid to calculate hilbert value. */
    GEOSGeomGetX_r(handle, centroid, &x);
    GEOSGeomGetY_r(handle, centroid, &y);

    /* 
     * A hilbert value is a calculation of where a given point would 'fit' on
     * a space filling hilbert curve.  The curve should be of a sufficient order
     * to fit in enough granularity.
     */
    hilbertVal = encode(x, y);
    MEcopy((unsigned char*)&hilbertVal, sizeof(i8), dv_out->db_data);

    /* Clean up the geos items. */
    GEOSGeom_destroy_r(handle, geometry);
    GEOSGeom_destroy_r(handle, centroid);
    finishGEOS_r(handle);

    return status;
#endif
}

#ifdef _WITH_GEO
static DB_STATUS
geomToGeomComparison(
ADF_CB           *adf_scb,
DB_DATA_VALUE    *dv1,
DB_DATA_VALUE    *dv2,
DB_DATA_VALUE    *rdv,
/* GEOS function to call */
char (*GEOSFunction) (GEOSContextHandle_t, const GEOSGeometry *, 
                      const GEOSGeometry *))
{
    DB_STATUS status = E_DB_OK;
    GEOSContextHandle_t handle;
    GEOSGeometry *g1 = NULL, *g2 = NULL;
    i4 returnValue;

    /*
     * If either input is NULL, return NULL
     */
    if(ADI_ISNULL_MACRO(dv1) || ADI_ISNULL_MACRO(dv2))
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    handle = initGEOS_r( geos_Notice, geos_Error );

    status = dataValueToGeos(adf_scb, dv1, handle, &g1, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
   "geomToGeomComparison: Failed to convert data value 1 to GEOSGeometry."));
    }

    status = dataValueToGeos(adf_scb, dv2, handle, &g2, FALSE);

    if(status != E_DB_OK)
    {
        GEOSGeom_destroy_r(handle, g1);
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
    "geomToGeomComparison: Failed to convert data value 2 to GEOSGeometry."));
    }

    /*
     * The GEOS comparison function returns char type, cast it to i4.
     */
    returnValue = (i4) (*GEOSFunction)(handle, g1, g2);
    /*
     * Put result in rdv
     */
    MEcopy(&returnValue, sizeof(i4), rdv->db_data);

    /*
     * Clean up
     */
    GEOSGeom_destroy_r(handle, g1);
    GEOSGeom_destroy_r(handle, g2);
    finishGEOS_r(handle);
    return status;
}

static DB_STATUS
geomToGeomReturnsGeom(
ADF_CB        *adf_scb,
DB_DATA_VALUE *dv1,
DB_DATA_VALUE *dv2,
DB_DATA_VALUE *rdv,
/* GEOS function to call */
GEOSGeometry * (*GEOSFunction) (GEOSContextHandle_t, const GEOSGeometry *,
                const GEOSGeometry *))
{
    DB_STATUS status = E_DB_OK;
    GEOSContextHandle_t handle;
    GEOSGeometry *g1 = NULL, *g2 = NULL, *gr = NULL;

    /*
     * If either input is NULL, return NULL
     */
    if(ADI_ISNULL_MACRO(dv1) || ADI_ISNULL_MACRO(dv2))
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    handle = initGEOS_r( geos_Notice, geos_Error );

    status = dataValueToGeos(adf_scb, dv1, handle, &g1, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
    "geomToGeomReturnsGeom: Failed to convert data value 1 to GEOSGeometry."));
    }

    status = dataValueToGeos(adf_scb, dv2, handle, &g2, FALSE);

    if(status != E_DB_OK)
    {
        GEOSGeom_destroy_r(handle, g1);
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
    "geomToGeomReturnsGeom: Failed to convert data value 2 to GEOSGeometry."));
    }

    /*
     * The GEOSFunction returns another geometry or NULL on error.
     */
    gr = (*GEOSFunction)(handle, g1, g2);

    if(gr == NULL)
    {
        GEOSGeom_destroy_r(handle, g1);
        GEOSGeom_destroy_r(handle, g2);
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
            "Failed to get result Geometry."));
    }

    /*
     * Put result in rdv
     */
    geosToDataValue(adf_scb, gr, handle, rdv, NULL, FALSE);

    /*
     * Clean up
     */
    GEOSGeom_destroy_r(handle, g1);
    GEOSGeom_destroy_r(handle, g2);
    GEOSGeom_destroy_r(handle, gr);
    finishGEOS_r(handle);
    return status;
}
#endif

/* Specify a point is expected from the input WKB. */
DB_STATUS
adu_point_fromWKB(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromWKB(adf_scb, dv_in, NULL, dv_out, GEOS_POINT);
#endif
}

/* Specify a linestring is expected from the input WKB. */
DB_STATUS
adu_linestring_fromWKB(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromWKB(adf_scb, dv_in, NULL, dv_out, GEOS_LINESTRING);
#endif
}

/* Specify a polygon is expected from the input WKB. */
DB_STATUS
adu_polygon_fromWKB(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromWKB(adf_scb, dv_in, NULL, dv_out, GEOS_POLYGON);
#endif
}

/* Specify a multipoint is expected from the input WKB. */
DB_STATUS
adu_multipoint_fromWKB(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromWKB(adf_scb, dv_in, NULL, dv_out, GEOS_MULTIPOINT);
#endif
}

/* Specify a multilinestring is expected from the input WKB.*/
DB_STATUS
adu_multilinestring_fromWKB(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromWKB(adf_scb, dv_in, NULL, dv_out, GEOS_MULTILINESTRING);
#endif
}

/* Specify a multipoygon is expected from the input WKB. */
DB_STATUS
adu_multipolygon_fromWKB(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromWKB(adf_scb, dv_in, NULL, dv_out, GEOS_MULTIPOLYGON);
#endif
}

/* Specify a geometry collection is expected from the input WKB. */
DB_STATUS
adu_geometrycollection_fromWKB(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromWKB(adf_scb, dv_in, NULL, dv_out,
                           GEOS_GEOMETRYCOLLECTION);
#endif
}

/* Specify a geometry is expected from the input WKB. */
DB_STATUS
adu_geometry_fromWKB(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromWKB(adf_scb, dv_in, NULL, dv_out, 
    		GENERIC_GEOMETRY);
#endif
}

/* 
 * Specify a point is expected from the input WKB.
 * SRID specified
 */
DB_STATUS
adu_point_srid_fromWKB(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_srid,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromWKB(adf_scb, dv_in, dv_srid, dv_out, GEOS_POINT);
#endif
}

/*
 * Specify a linestring is expected from the input WKB.
 * SRID specified
 */
DB_STATUS
adu_linestring_srid_fromWKB(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_srid,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromWKB(adf_scb, dv_in, dv_srid, dv_out, GEOS_LINESTRING);
#endif
}

/* 
 * Specify a polygon is expected from the input WKB.
 * SRID specified
 */
DB_STATUS
adu_polygon_srid_fromWKB(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_srid,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromWKB(adf_scb, dv_in, dv_srid, dv_out, GEOS_POLYGON);
#endif
}

/*
 * Specify a multipoint is expected from the input WKB.
 * SRID specified
 */
DB_STATUS
adu_multipoint_srid_fromWKB(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_srid,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromWKB(adf_scb, dv_in, dv_srid, dv_out, GEOS_MULTIPOINT);
#endif
}

/* 
 * Specify a multilinestring is expected from the input WKB.
 * SRID specified
 */
DB_STATUS
adu_multilinestring_srid_fromWKB(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_srid,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromWKB(adf_scb, dv_in, dv_srid, dv_out, 
                           GEOS_MULTILINESTRING);
#endif
}

/* 
 * Specify a multipoygon is expected from the input WKB.
 * SRID specified
 */
DB_STATUS
adu_multipolygon_srid_fromWKB(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_srid,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromWKB(adf_scb, dv_in, dv_srid, dv_out, GEOS_MULTIPOLYGON);
#endif
}

/* 
 * Specify a geometry collection is expected from the input WKB.
 * SRID specified
 */
DB_STATUS
adu_geometrycollection_srid_fromWKB(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_srid,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromWKB(adf_scb, dv_in, dv_srid, dv_out,
                           GEOS_GEOMETRYCOLLECTION);
#endif
}

/* 
 * Specify a geometry is expected from the input WKB.
 * SRID specified
 */
DB_STATUS
adu_geometry_srid_fromWKB(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_srid,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromWKB(adf_scb, dv_in, dv_srid, dv_out, 
    		GENERIC_GEOMETRY);
#endif
}

#ifdef _WITH_GEO
/*
 * Accept a geometry specified with WKB as input.  Compute a bounding box
 * (envelope) and convert it to WKB.  Stream both WKB into storage.
 */
static DB_STATUS
adu_geo_fromWKB(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_srid,
DB_DATA_VALUE   *dv_out,
i4 expectedGeoType)
{
    DB_STATUS status = E_DB_OK;
    i4 len, combLen;
    i4 processedGeoType;
    i4 srid;

    GEOSContextHandle_t handle;
    GEOSGeometry *geometry = NULL;

    /*
     * If input is NULL, return NULL
     */
    if(ADI_ISNULL_MACRO(dv_in))
    {
        ADF_SETNULL_MACRO(dv_out);
        return E_DB_OK;
    }

    /* Initialize GEOS */
    handle = initGEOS_r( geos_Notice, geos_Error );
    /* Convert the input WKB into a geos geometry. */
    status = dataValueToGeos(adf_scb, dv_in, handle, &geometry, TRUE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD1021_BAD_ERROR_PARAMS, 2, 0, 
            "adu_geo_fromWKB: Could not convert input into geos geometry."));
    }

    /* 
     * Check if we processed the expected geometry type.  If we're expecting
     * a geometry collection then all types are valid.
     */
    processedGeoType = GEOSGeomTypeId_r(handle, geometry);
    if(expectedGeoType != GENERIC_GEOMETRY &&
       expectedGeoType != processedGeoType)
    {
        GEOSGeom_destroy_r(handle, geometry);
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD1014_BAD_VALUE_FOR_DT, 2, 0, 
            "adu_geo_fromWKB: Wrong geometry type result."));
    }

    /* Two argument version called */
    if(dv_srid != NULL)
    {
        switch(dv_srid->db_length)
        {
            case 1:
                srid = *(i1 *) dv_srid->db_data;
                break;
            case 2:
                srid = *(i2 *) dv_srid->db_data;
                break;
            case 4:
                srid = *(i4 *) dv_srid->db_data;
                break;
            case 8:
                srid = *(i8 *) dv_srid->db_data;
                break;
            default:
                return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
                "adu_geo_fromWKB: invalid length passed with data value srid"));
        }
    }
    else
    {
        srid = -1;
    }

    GEOSSetSRID_r(handle, geometry, srid);

    /* 
     * Convert the geos geometry into our GEOM structure and stream it into
     * the return data value.
     */
    status = geosToDataValue(adf_scb, geometry, handle, dv_out, &combLen,
                              TRUE);
    if(status != E_DB_OK)
    {
        GEOSGeom_destroy_r(handle, geometry);
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_geo_fromWKB: Failed to convert geos to datavalue."));
    }

    /* Clean up the geometry and finalize GEOS. */
    GEOSGeom_destroy_r(handle, geometry);
    finishGEOS_r(handle);

    return status;
}
#endif

/* Specify a point is expected from the input WKT. */
DB_STATUS
adu_point_fromText(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromText(adf_scb, dv_in, NULL, dv_out, GEOS_POINT);
#endif
}

/* Specify a linestring is expected from the input WKT. */
DB_STATUS
adu_linestring_fromText(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromText(adf_scb, dv_in, NULL, dv_out, GEOS_LINESTRING);
#endif
}

/* Specify a polygon is expected from the input WKT. */
DB_STATUS
adu_polygon_fromText(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromText(adf_scb, dv_in, NULL, dv_out, GEOS_POLYGON);
#endif
}

/* Specify a multipoint is expected from the input WKT. */
DB_STATUS
adu_multipoint_fromText(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromText(adf_scb, dv_in, NULL, dv_out, GEOS_MULTIPOINT);
#endif
}

/* Specify a multilinestring is expected from the input WKT. */
DB_STATUS
adu_multilinestring_fromText(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromText(adf_scb, dv_in, NULL, dv_out, GEOS_MULTILINESTRING);
#endif
}

/* Specify a multipolygon is expected from the input WKT. */
DB_STATUS
adu_multipolygon_fromText(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromText(adf_scb, dv_in, NULL, dv_out, GEOS_MULTIPOLYGON);
#endif
}

/* Specify a geometry collection is expected from the input WKT. */
DB_STATUS
adu_geometrycollection_fromText(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromText(adf_scb, dv_in, NULL, dv_out,
                            GEOS_GEOMETRYCOLLECTION);
#endif
}

/* Specify a geometry is expected from the input WKT. */
DB_STATUS
adu_geometry_fromText(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromText(adf_scb, dv_in, NULL, dv_out, 
    		GENERIC_GEOMETRY);
#endif
}

/* 
 * Specify a point is expected from the input WKT.
 * Speficies SRID
 */
DB_STATUS
adu_point_srid_fromText(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_srid,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromText(adf_scb, dv_in, dv_srid, dv_out, GEOS_POINT);
#endif
}

/*
 * Specify a linestring is expected from the input WKT.
 * Speficies SRID
 */
DB_STATUS
adu_linestring_srid_fromText(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_srid,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromText(adf_scb, dv_in, dv_srid, dv_out, GEOS_LINESTRING);
#endif
}

/*
 * Specify a polygon is expected from the input WKT.
 * Speficies SRID
 */
DB_STATUS
adu_polygon_srid_fromText(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_srid,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromText(adf_scb, dv_in, dv_srid, dv_out, GEOS_POLYGON);
#endif
}

/*
 * Specify a multipoint is expected from the input WKT.
 * Speficies SRID
 */
DB_STATUS
adu_multipoint_srid_fromText(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_srid,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromText(adf_scb, dv_in, dv_srid, dv_out, GEOS_MULTIPOINT);
#endif
}

/*
 * Specify a multilinestring is expected from the input WKT.
 * Speficies SRID
 */
DB_STATUS
adu_multilinestring_srid_fromText(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_srid,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromText(adf_scb, dv_in, dv_srid, dv_out, 
                            GEOS_MULTILINESTRING);
#endif
}

/*
 * Specify a multipolygon is expected from the input WKT.
 * Speficies SRID
 */
DB_STATUS
adu_multipolygon_srid_fromText(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_srid,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromText(adf_scb, dv_in, dv_srid, dv_out,
                            GEOS_MULTIPOLYGON);
#endif
}

/* 
 * Specify a geometry collection is expected from the input WKT.
 * Speficies SRID
 */
DB_STATUS
adu_geometrycollection_srid_fromText(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_srid,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromText(adf_scb, dv_in, dv_srid, dv_out,
                            GEOS_GEOMETRYCOLLECTION);
#endif
}

/* 
 * Specify a geometry collection is expected from the input WKT.
 * Speficies SRID
 */
DB_STATUS
adu_geometry_srid_fromText(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_srid,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return adu_geo_fromText(adf_scb, dv_in, dv_srid, dv_out,
    		GENERIC_GEOMETRY);
#endif
}

#ifdef _WITH_GEO
/*
 * Accept a geometry specified with WKT as input.  Convert the geometry to WKB.
 * Compute a bounding box (envelope) and convert it to WKB.  Stream both WKB
 * into storage.
 */
static DB_STATUS
adu_geo_fromText(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_srid,
DB_DATA_VALUE   *dv_out,
i4 expectedGeoType)
{
    DB_STATUS status = E_DB_OK;
    i4 len, processedGeoType, combLen, srid;
    unsigned char *c_ptr;

    GEOSContextHandle_t handle;
    GEOSWKTReader *wktReader;
    GEOSGeometry *geometry = NULL;

    /*
     * If input is NULL, return NULL
     */
    if(ADI_ISNULL_MACRO(dv_in))
    {
        ADF_SETNULL_MACRO(dv_out);
        return E_DB_OK;
    }

    /* Convert the incoming data to a unsigned char */
    status = adu_long_to_cstr(adf_scb, dv_in, &c_ptr, &len);
    if(status != E_DB_OK)
    {
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2,0,
                    "geo_fromText: Bad copy from lvch to char."));
    }

    /* Initialize GEOS and create a new WKT reader. */
    handle = initGEOS_r( geos_Notice, geos_Error );
    wktReader = GEOSWKTReader_create_r(handle);
    /* Convert the WKT into a GEOS geometry. */
    geometry = GEOSWKTReader_read_r(handle, wktReader, c_ptr);
    if(geometry == NULL)
    {
        GEOSWKTReader_destroy_r(handle, wktReader);
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
            "adu_geo_fromText: Bad conversion from WKT into geom"));
    }
    /* Clean up the WKT reader. */
    GEOSWKTReader_destroy_r(handle, wktReader);

    /*
     * Check if the processed WKT resulted in the type that was expected.  If
     * a geometry is expected then all types are valid.
     */
    processedGeoType = GEOSGeomTypeId_r(handle, geometry);
    if(expectedGeoType != GENERIC_GEOMETRY &&
       expectedGeoType != processedGeoType)
    {
        GEOSGeom_destroy_r(handle, geometry);
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD1014_BAD_VALUE_FOR_DT, 2, 0, 
            "adu_geo_fromText: Wrong geometry type result."));
    }

    /* Two argument version called */
    if(dv_srid != NULL)
    {
        switch(dv_srid->db_length)
        {
            case 1:
                srid = *(i1 *) dv_srid->db_data;
                break;
            case 2:
                srid = *(i2 *) dv_srid->db_data;
                break;
            case 4:
                srid = *(i4 *) dv_srid->db_data;
                break;
            case 8:
                srid = *(i8 *) dv_srid->db_data;
                break;
            default:
                return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
               "adu_geo_fromText: invalid length passed with data value srid"));
        }
    }
    else
    {
        srid = -1;
    }

    GEOSSetSRID_r(handle, geometry, srid);

    /* 
     * Convert the geos geometry into our GEOM structure and stream it into
     * the return data value.
     */
    status = geosToDataValue(adf_scb, geometry, handle, dv_out, &combLen,
                              TRUE);
    if(status != E_DB_OK)
    {
        GEOSGeom_destroy_r(handle, geometry);
        finishGEOS_r(handle);
        /* Error code/message set iin geosToDataValue */
        return status;
    }

    /* Clean up the geometry and finalize GEOS. */
    GEOSGeom_destroy_r(handle, geometry);
    finishGEOS_r(handle);
    return status;
}
#endif

/* 
 * This function takes the stored data, splits out the geometry from the stream
 * and returns it.
 */
DB_STATUS
adu_geom_asBinary(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    i4 geomDataSize;
    DB_DATA_VALUE dv_txt, dv_wkb;
    i4 numRings, numPoints;
    storedGeom_t geomData;

    if(ADI_ISNULL_MACRO(dv_in))
    {
        ADF_SETNULL_MACRO(dv_out);
        return E_DB_OK;
    }

    /* De-stream the data value into a stored geometry stucture. */
    status = dataValueToStoredGeom(adf_scb, dv_in, &geomData, FALSE);
    if(status != E_DB_OK)
    {
        if(geomData.combinedWKB != NULL)
        {
            MEsysfree(geomData.combinedWKB);
            geomData.combinedWKB = NULL;
        }
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "asBinary: Failed to convert datavalue to geom data."));
    }

    /* 
     * Create space for the geometry WKB and copy out just the geometry WKB by
     * starting the pointer at the size of the envelope WKB.
     */
    dv_wkb.db_data = MEmalloc(geomData.geomSize);
    if(dv_wkb.db_data == NULL)
    {
        if(geomData.combinedWKB != NULL)
        {
            MEsysfree(geomData.combinedWKB);
            geomData.combinedWKB = NULL;
        }
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2,0,
                "asBinary: couldn't allocate memory to dv_wkb.db_data."));
    }
    MEcopy(&geomData.combinedWKB[geomData.envelopeSize], geomData.geomSize, 
           dv_wkb.db_data);

    /* Clean up the memory no longer required. */
    if(geomData.combinedWKB != NULL)
    {
        MEsysfree(geomData.combinedWKB);
        geomData.combinedWKB = NULL;
    }

    /* Fill out the rest of the data structure. */
    dv_wkb.db_length = geomData.geomSize;
    dv_wkb.db_datatype = DB_VBYTE_TYPE;

    /* Convert back to blob in case the data is too large. */
    status = adu_wkbDV_to_long( adf_scb, &dv_wkb, dv_out );
    if (status != E_DB_OK)
    {
        if(dv_wkb.db_data != NULL)
        {
            MEsysfree(dv_wkb.db_data);
            dv_wkb.db_data = NULL;
        }
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "asBinary: Failed to convert WKB to long data value."));
    }

    /* Clean up the data structure and finalize GEOS. */
    if(dv_wkb.db_data != NULL)
    {
        MEsysfree(dv_wkb.db_data);
        dv_wkb.db_data = NULL;
    }

    return status;
#endif
}

static DB_STATUS geom_to_text(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_out,
i4 trim,
i4 precision)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    DB_DATA_VALUE dv_wkt;
    GEOSContextHandle_t handle;
    GEOSWKTWriter *wktWriter;
    GEOSGeometry *geometry = NULL;

    if(ADI_ISNULL_MACRO(dv_in))
    {
        ADF_SETNULL_MACRO(dv_out);
        return E_DB_OK;
    }

    /* Initialize geos and convert the input data into a geos geometry */
    handle = initGEOS_r( geos_Notice, geos_Error );
    status = dataValueToGeos(adf_scb, dv_in, handle, &geometry, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
                "adu_geo_asTest: Bad conversion data value to GEOS"));
    }

    /* Create a WKT writer and convert the GEOS geometry to WKT. */
    wktWriter = GEOSWKTWriter_create_r(handle);
    GEOSWKTWriter_setTrim_r(handle, wktWriter, trim);
    GEOSWKTWriter_setRoundingPrecision_r(handle, wktWriter, precision);
    GEOSWKTWriter_setOutputDimension_r(handle, wktWriter, 3 ); /* allow 3D */
    dv_wkt.db_data = GEOSWKTWriter_write_r(handle, wktWriter, geometry);
    /* Clean up the WKT writer and the GEOS geometry. */
    GEOSWKTWriter_destroy_r(handle, wktWriter);
    GEOSGeom_destroy_r(handle, geometry);
    if( dv_wkt.db_data == NULL)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
            "adu_geo_asText: Bad conversion from geom to WKT"));
    }

    /* Fill out the rest of the data structure. */
    dv_wkt.db_length = STlen(dv_wkt.db_data);
    dv_wkt.db_datatype = DB_VBYTE_TYPE;

    /* Convert back to blob in case the data is too large. */
    status = adu_wkbDV_to_long( adf_scb, &dv_wkt, dv_out );
    /* Clean up the non long wkt db_data */
    if(dv_wkt.db_data != NULL)
    {
        MEsysfree(dv_wkt.db_data);
        dv_wkt.db_data = NULL;
    }
    if (status != E_DB_OK)
    {
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "asText: Failed to convert WKT to long data value."));
    }

    /* Clean up GEOS. */
    finishGEOS_r(handle);
    return status;
#endif
}

/* 
 * This function takes the stored data, splits out the geometry from the stream
 * and converts it to WKT.
 */
DB_STATUS
adu_geom_asText(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return geom_to_text(adf_scb, dv_in, dv_out, TRUE, FULL_PRECISION);
#endif
}

DB_STATUS
adu_geom_asTextRaw(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv_in,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return geom_to_text(adf_scb, dv_in, dv_out, FALSE, FULL_PRECISION);
#endif
}

DB_STATUS
adu_geom_asTextRound(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *dv2,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    i4 precision = 0;

    switch(dv2->db_length)
    {
        case 1:
            precision = *(i1 *) dv2->db_data;
            break;
        case 2:
            precision = *(i2 *) dv2->db_data;
            break;
        case 4:
            precision = *(i4 *) dv2->db_data;
            break;
        case 8:
            precision = *(i8 *) dv2->db_data;
            break;
        default:
            return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_geom_asTextRound: invalid length passed with data value precision"));
    }

    return geom_to_text(adf_scb, dv1, dv_out, FALSE, precision);
#endif
}

DB_STATUS
adu_dimension(
ADF_CB           *adf_scb,
DB_DATA_VALUE    *dv1,
DB_DATA_VALUE    *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    GEOSContextHandle_t handle;
    GEOSGeometry *geometry = NULL;
    i2 dimensions;

    if(ADI_ISNULL_MACRO(dv1))
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    handle = initGEOS_r(geos_Notice, geos_Error);

    status = dataValueToGeos(adf_scb, dv1, handle,&geometry, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_dimension: Failed to convert data value to GEOSGeometry"));
    }

    dimensions = GEOSGeom_getDimensions_r(handle, geometry);
    MEcopy(&dimensions, rdv->db_length, rdv->db_data);
    /*
     * Clean up
     */
    GEOSGeom_destroy_r(handle, geometry);
    finishGEOS_r(handle);
    return status;
#endif
}

DB_STATUS
adu_geometry_type(
ADF_CB           *adf_scb,
DB_DATA_VALUE    *dv1,
DB_DATA_VALUE    *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    GEOSContextHandle_t handle;
    GEOSGeometry *geometry = NULL;
    char *geomType;

    if(ADI_ISNULL_MACRO(dv1))
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    handle = initGEOS_r(geos_Notice, geos_Error);

    status = dataValueToGeos(adf_scb, dv1, handle, &geometry, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_geometry_type: Failed to convert data value to GEOSGeometry"));
    }

    geomType = GEOSGeomType_r(handle, geometry);
    if(geomType == NULL)
    {
        GEOSGeom_destroy_r(handle, geometry);
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
                "adu_geometry_type: Failed to get geometry type string."));
    }
    status = adu_movestring(adf_scb, geomType, strlen(geomType),
                            rdv->db_datatype ,rdv);
    /*
     * Clean up
     */
    GEOSGeom_destroy_r(handle, geometry);
    GEOSFree_r(handle, geomType);
    finishGEOS_r(handle);
    return status;
#endif
}

/*
 * The OGC Boundary SQL function.
 * It should not operate on GeometryCollection, update this later.
 */
DB_STATUS
adu_boundary(
ADF_CB        *adf_scb,
DB_DATA_VALUE *dv1,
DB_DATA_VALUE *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    GEOSContextHandle_t handle;
    GEOSGeometry *geometry = NULL, *boundary = NULL;

    if(ADI_ISNULL_MACRO(dv1))
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    handle = initGEOS_r( geos_Notice, geos_Error );

    /*
     * We need to handle nullable data types, adu expects them to be
     * non-nullable
     */
    dv1->db_datatype = abs(dv1->db_datatype);

    status = dataValueToGeos(adf_scb, dv1, handle, &geometry, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_boundary: Failed to convert data value to GEOSGeometry."));
    }

    /*
     * Boundary is not supported on GeometryCollections, throw error
     * NOTE: This is how PostGis handles it
     */
    if(GEOSGeomTypeId_r(handle, geometry) == GEOS_GEOMETRYCOLLECTION)
    {
        GEOSGeom_destroy_r(handle, geometry);
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
            "Boundary does not support GeometryCollections."));
    }

    /*
     * Boundary on point or multipoint called, return NULL
     * Is this okay according to the spec? It doesn't quite say
     * NOTE: PostGIS returns an empty geometrycollection in this case
     */
    if(GEOSGeomTypeId_r(handle, geometry) == GEOS_POINT ||
            GEOSGeomTypeId_r(handle, geometry) == GEOS_MULTIPOINT)
    {
        ADF_SETNULL_MACRO(rdv);
        GEOSGeom_destroy_r(handle, geometry);
        finishGEOS_r(handle);
        return E_DB_OK;
    }

    boundary = GEOSBoundary_r(handle, geometry);

    if(boundary == NULL)
    {
        GEOSGeom_destroy_r(handle, geometry);
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_boundary: Failed to get boundary of GEOSGeometry."));
    }

    status = geosToDataValue(adf_scb, boundary, handle, rdv, NULL, FALSE);

    /*
     * Clean up
     */
    GEOSGeom_destroy_r(handle, geometry);
    GEOSGeom_destroy_r(handle, boundary);
    finishGEOS_r(handle);
    return status;
#endif
}

/*
 * Returns the envelope of a geometry
 * This will return a POLYGON type unless used on a POINT type
 * or a MULTIPOINT with only one data point.
 */
DB_STATUS
adu_envelope(
ADF_CB           *adf_scb,
DB_DATA_VALUE    *dv1,
DB_DATA_VALUE    *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    storedGeom_t geomData;
    GEOSContextHandle_t handle;
    GEOSGeometry *geometry = NULL;

    if(ADI_ISNULL_MACRO(dv1))
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    handle = initGEOS_r( geos_Notice, geos_Error );

    status = dataValueToStoredGeom(adf_scb, dv1, &geomData, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_envelope: Failed to convert data value to stored geometry."));
    }

    /* Make sure to convert the envelope, pass TRUE. */
    status = storedGeomToGeos(adf_scb, handle, &geomData, &geometry, TRUE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_envelope: Failed to extract envelope."));
    }

    status = geosToDataValue(adf_scb, geometry, handle, rdv, NULL, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_envelope: Failed to convert envelope to data value."));
    }


    /*
     * Clean up
     */
    GEOSGeom_destroy_r(handle, geometry);
    finishGEOS_r(handle);
    return status;
#endif
}

/*
 * Checks whether two geometries are equal or not
 * returns 0 for false, 1 for true and NULL on NULL input
 */
DB_STATUS
adu_equals(
ADF_CB           *adf_scb,
DB_DATA_VALUE    *dv1,
DB_DATA_VALUE    *dv2,
DB_DATA_VALUE    *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return geomToGeomComparison(adf_scb, dv1, dv2, rdv, &GEOSEquals_r);
#endif
}

/*
 * Checks whether two geometries are disjoint
 * returns 0 for false, 1 for true and NULL on NULL input
 */
DB_STATUS
adu_disjoint(
ADF_CB           *adf_scb,
DB_DATA_VALUE    *dv1,
DB_DATA_VALUE    *dv2,
DB_DATA_VALUE    *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return geomToGeomComparison(adf_scb, dv1, dv2, rdv, &GEOSDisjoint_r);
#endif
}

/*
 * Checks whether two geometries intersects
 * returns 0 for false, 1 for true and NULL on NULL input
 */
DB_STATUS
adu_intersects(
ADF_CB           *adf_scb,
DB_DATA_VALUE    *dv1,
DB_DATA_VALUE    *dv2,
DB_DATA_VALUE    *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return geomToGeomComparison(adf_scb, dv1, dv2, rdv, &GEOSIntersects_r);
#endif
}

/*
 * Checks whether two geometries touch
 * returns 0 for false, 1 for true and NULL on NULL input
 */
DB_STATUS
adu_touches(
ADF_CB           *adf_scb,
DB_DATA_VALUE    *dv1,
DB_DATA_VALUE    *dv2,
DB_DATA_VALUE    *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return geomToGeomComparison(adf_scb, dv1, dv2, rdv, &GEOSTouches_r);
#endif
}

/*
 * Checks whether two geometries cross
 * returns 0 for false, 1 for true and NULL on NULL input
 */
DB_STATUS
adu_crosses(
ADF_CB           *adf_scb,
DB_DATA_VALUE    *dv1,
DB_DATA_VALUE    *dv2,
DB_DATA_VALUE    *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return geomToGeomComparison(adf_scb, dv1, dv2, rdv, &GEOSCrosses_r);
#endif
}

/*
 * Checks whether g2 is within g1
 * returns 0 for false, 1 for true and NULL on NULL input
 * GEOS has it reversed from the OGC standard
 */
DB_STATUS
adu_within(
ADF_CB           *adf_scb,
DB_DATA_VALUE    *dv1,
DB_DATA_VALUE    *dv2,
DB_DATA_VALUE    *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return geomToGeomComparison(adf_scb, dv1, dv2, rdv, &GEOSWithin_r);
#endif
}

/*
 * Checks whether g1 contains g2
 * returns 0 for false, 1 for true and NULL on NULL input
 */
DB_STATUS
adu_contains(
ADF_CB           *adf_scb,
DB_DATA_VALUE    *dv1,
DB_DATA_VALUE    *dv2,
DB_DATA_VALUE    *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return geomToGeomComparison(adf_scb, dv1, dv2, rdv, &GEOSContains_r);
#endif
}

/*
 * Checks whether g1 is inside g2
 * returns 0 for false, 1 for true and NULL on NULL input
 */
DB_STATUS
adu_inside(
ADF_CB           *adf_scb,
DB_DATA_VALUE    *dv1,
DB_DATA_VALUE    *dv2,
DB_DATA_VALUE    *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return geomToGeomComparison(adf_scb, dv2, dv1, rdv, &GEOSContains_r);
#endif
}

DB_STATUS
adu_geom_overlaps(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *dv2,
DB_DATA_VALUE   *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    if(dv1->db_datatype == DB_NBR_TYPE && dv2->db_datatype == DB_NBR_TYPE)
    {
        geomNBR_t *nbr1, *nbr2;
        *rdv->db_data = 0;;

        nbr1 = (geomNBR_t *)dv1->db_data;
        nbr2 = (geomNBR_t *)dv2->db_data;

        if(nbr1->x_low > nbr2->x_hi) return E_DB_OK;
        if(nbr1->x_hi < nbr2->x_low) return E_DB_OK;
        if(nbr1->y_low > nbr2->y_hi) return E_DB_OK;
        if(nbr1->y_hi < nbr2->y_low) return E_DB_OK;
        *rdv->db_data = 1;
        return E_DB_OK;
    }
    return geomToGeomComparison(adf_scb, dv1, dv2, rdv, &GEOSOverlaps_r);
#endif
}

DB_STATUS
adu_geom_perimeter(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    GEOSGeometry *geom = NULL;
    GEOSContextHandle_t handle;
    f8 length = 0;

    if(ADI_ISNULL_MACRO(dv1))
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    handle = initGEOS_r( geos_Notice, geos_Error );

    status = dataValueToGeos(adf_scb, dv1, handle, &geom, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_perimeter: Failed to convert data value to GEOSGeometry."));
    }

    if(GEOSLength_r(handle, geom, &length) == 1)
    {
        MEcopy(&length, rdv->db_length, rdv->db_data);
    }
    else
    {
        status = adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_perimeter: geos could not calculate perimeter.");
    }
    /*
     * Clean up
     */
    GEOSGeom_destroy_r(handle, geom);
    finishGEOS_r(handle);

    return status;
#endif
}

/*
 * Returns true or false depending on relate matrix pattern
 */
DB_STATUS
adu_relate(
ADF_CB           *adf_scb,
DB_DATA_VALUE    *dv1,
DB_DATA_VALUE    *dv2,
DB_DATA_VALUE    *dv3,
DB_DATA_VALUE    *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    GEOSContextHandle_t handle;
    GEOSGeometry *g1 = NULL, *g2 = NULL;
    i4 returnValue;

    if(ADI_ISNULL_MACRO(dv1) || ADI_ISNULL_MACRO(dv2) || ADI_ISNULL_MACRO(dv3))
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    if(dv3->db_length != 9)
    {
        return(adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
            "adu_relate: Matrix string has to be exactly 9 characters long."));
    }

    handle = initGEOS_r( geos_Notice, geos_Error );

    status = dataValueToGeos(adf_scb, dv1, handle, &g1, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_relate: Failed to convert data value 1 to GEOSGeometry."));
    }
    status = dataValueToGeos(adf_scb, dv2, handle, &g2, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_relate: Failed to convert data value 2 to GEOSGeometry."));
    }

    returnValue = (i4) GEOSRelatePattern_r(handle, g1, g2, 
                                           (char *) dv3->db_data);

    MEcopy(&returnValue, rdv->db_length, rdv->db_data);

    /*
     * Clean up
     */
    GEOSGeom_destroy_r(handle, g1);
    GEOSGeom_destroy_r(handle, g2);
    finishGEOS_r(handle);
    return status;
#endif
}

/*
 * Returns distance between two geometries
 */
DB_STATUS
adu_distance(
ADF_CB           *adf_scb,
DB_DATA_VALUE    *dv1,
DB_DATA_VALUE    *dv2,
DB_DATA_VALUE    *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    GEOSContextHandle_t handle;
    GEOSGeometry *g1 = NULL, *g2 = NULL;
    f8 distance;

    if(ADI_ISNULL_MACRO(dv1) || ADI_ISNULL_MACRO(dv2))
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    handle = initGEOS_r(geos_Notice, geos_Error);

    /* 
     * We need to handle nullable data types, adu expects them to be 
     * non-nullable
     */
    dv1->db_datatype = abs(dv1->db_datatype);

    status = dataValueToGeos(adf_scb, dv1, handle,&g1, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_distance: Failed to convert data value 1 to GEOSGeometry"));
    }

    /* 
     * We need to handle nullable data types, adu expects them to be 
     * non-nullable
     */
    dv2->db_datatype = abs(dv2->db_datatype);

    status = dataValueToGeos(adf_scb, dv2, handle,&g2, FALSE);
    if(status != E_DB_OK)
    {
        GEOSGeom_destroy_r(handle, g1);
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_distance: Failed to convert data value 2 to GEOSGeometry"));
    }

    /*
     * If either Geometry is empty, return NULL
     * NOTE: This is how PostGIS handles it
     */
    if(GEOSisEmpty_r(handle, g1) || GEOSisEmpty_r(handle, g2))
    {
        GEOSGeom_destroy_r(handle, g1);
        GEOSGeom_destroy_r(handle, g2);
        finishGEOS_r(handle);
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    GEOSDistance_r(handle, g1, g2, &distance);

    MEcopy(&distance, sizeof(f8), rdv->db_data);
    /*
     * Clean up
     */
    GEOSGeom_destroy_r(handle, g1);
    GEOSGeom_destroy_r(handle, g2);
    finishGEOS_r(handle);
    return status;
#endif
}

/*
 * Returns the intersection between two geometries as a geometry
 */
DB_STATUS
adu_intersection(
ADF_CB           *adf_scb,
DB_DATA_VALUE    *dv1,
DB_DATA_VALUE    *dv2,
DB_DATA_VALUE    *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return geomToGeomReturnsGeom(adf_scb, dv1, dv2, rdv, &GEOSIntersection_r);
#endif
}

/*
 * Returns the difference between two geometries as a geometry
 */
DB_STATUS
adu_difference(
ADF_CB           *adf_scb,
DB_DATA_VALUE    *dv1,
DB_DATA_VALUE    *dv2,
DB_DATA_VALUE    *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return geomToGeomReturnsGeom(adf_scb, dv1, dv2, rdv, &GEOSDifference_r);
#endif
}

/*
 * Returns the union between two geometries as a geometry
 */
DB_STATUS
adu_geom_union(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1_in,
DB_DATA_VALUE   *dv2_in,
DB_DATA_VALUE   *dv_out)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    /* If these are NBR types, then a union is a new NBR that contains both.*/
    if(dv1_in->db_datatype == DB_NBR_TYPE && dv2_in->db_datatype == DB_NBR_TYPE)
    {
        geomNBR_t *nbr1, *nbr2, *nbrOut;

        nbr1 = (geomNBR_t *)dv1_in->db_data;
        nbr2 = (geomNBR_t *)dv2_in->db_data;
        nbrOut = (geomNBR_t *)dv_out->db_data;

        nbrOut->x_low = min(nbr1->x_low, nbr2->x_low);
        nbrOut->y_low = min(nbr1->y_low, nbr2->y_low);
        nbrOut->x_hi = max(nbr1->x_hi, nbr2->x_hi);
        nbrOut->y_hi = max(nbr1->y_hi, nbr2->y_hi);
        return E_DB_OK;
    }
    else
        return geomToGeomReturnsGeom(adf_scb, dv1_in, dv2_in, dv_out,
                                     &GEOSUnion_r);
#endif
}

/*
 * Returns the symdifference between two geometries as a geometry
 */
DB_STATUS
adu_sym_difference(
ADF_CB           *adf_scb,
DB_DATA_VALUE    *dv1,
DB_DATA_VALUE    *dv2,
DB_DATA_VALUE    *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    return geomToGeomReturnsGeom(adf_scb, dv1, dv2, rdv, &GEOSSymDifference_r);
#endif
}

/*
 * Returns a buffer using a given distance
 */
DB_STATUS
adu_buffer_one(
ADF_CB           *adf_scb,
DB_DATA_VALUE    *dv1,
DB_DATA_VALUE    *dv2,
DB_DATA_VALUE    *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    const int default_quadsegs = 8;
    DB_STATUS status;
    GEOSGeometry *geometry, *buffer;
    GEOSContextHandle_t handle;
    f8 distance;

    if(ADI_ISNULL_MACRO(dv1) || ADI_ISNULL_MACRO(dv2))
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    handle = initGEOS_r(geos_Notice, geos_Error);

    status = dataValueToGeos(adf_scb, dv1, handle, &geometry, FALSE);

    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_buffer_one: Failed to convert data value 1 to GEOSGeometry"));
    }

    switch(dv2->db_length)
    {
    case 4:
        distance = *(f4 *)dv2->db_data;
        break;
    case 8:
        distance = *(f8 *)dv2->db_data;
        break;
    default:
        GEOSGeom_destroy_r(handle, geometry);
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
                "adu_buffer_one: wrong length for dv2."));
    }

    buffer = GEOSBuffer_r(handle, geometry, distance, default_quadsegs);

    if(buffer == NULL)
    {
        GEOSGeom_destroy_r(handle, geometry);
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
                "adu_buffer_one: couldn't get buffer for geometry."));
    }

    geosToDataValue(adf_scb, buffer, handle, rdv, NULL, FALSE);

    /*
     * Clean up
     */
    GEOSGeom_destroy_r(handle, geometry);
    GEOSGeom_destroy_r(handle, buffer);
    finishGEOS_r(handle);
    return E_DB_OK;
#endif
}

/*
 * OGC 1.2 function, not finished yet.
 */
DB_STATUS
adu_buffer_two(
ADF_CB           *adf_scb,
DB_DATA_VALUE    *dv1,
DB_DATA_VALUE    *dv2,
DB_DATA_VALUE    *dv3,
DB_DATA_VALUE    *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    const int default_quadsegs = 8;
    DB_STATUS status;
    GEOSGeometry *geometry, *buffer;
    GEOSContextHandle_t handle;
    f8 distance;

    return E_DB_OK;
#endif
}

/*
 * Returns the convexhull of a geometry
 */
DB_STATUS
adu_convexhull(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    GEOSGeometry *geometry, *convexhull;
    GEOSContextHandle_t handle;

    if(ADI_ISNULL_MACRO(dv1))
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    handle = initGEOS_r( geos_Notice, geos_Error );

    status = dataValueToGeos(adf_scb, dv1, handle, &geometry, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_convexhull: Failed to convert data value to GEOSGeometry."));
    }

    convexhull = GEOSConvexHull_r(handle, geometry);

    if(convexhull == NULL)
    {
        GEOSGeom_destroy_r(handle, geometry);
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_convexhull: Failed to get ConvexHull."));
    }

    status = geosToDataValue(adf_scb, convexhull, handle, rdv, NULL, FALSE);

    /*
     * clean up
     */
    GEOSGeom_destroy_r(handle, geometry);
    GEOSGeom_destroy_r(handle, convexhull);
    finishGEOS_r(handle);

    return status;
#endif
}

/*
 * returns the nth point in a LineString
 */
DB_STATUS
adu_pointn(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *dv2,
DB_DATA_VALUE   *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    GEOSGeometry *linestring = NULL, *point = NULL;
    GEOSContextHandle_t handle;
    i4 position, points;

    if(ADI_ISNULL_MACRO(dv1) || ADI_ISNULL_MACRO(dv2))
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    /*
     * If it's not a line type, return NULL
     * NOTE: This is how PostGIS handles it
     */
    if(abs(dv1->db_datatype) != DB_LINE_TYPE)
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    switch(dv2->db_length)
    {
        case 1:
            position = *(i1 *) dv2->db_data;
            break;
        case 2:
            position = *(i2 *) dv2->db_data;
            break;
        case 4:
            position = *(i4 *) dv2->db_data;
            break;
        case 8:
            position = *(i8 *) dv2->db_data;
            break;
        default:
            return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
                "adu_pointn: invalid length passed with data value 2"));
    }

    handle = initGEOS_r( geos_Notice, geos_Error );

    /* 
     * We need to handle nullable data types, adu expects them to be 
     * non-nullable
     */
    dv1->db_datatype = abs(dv1->db_datatype);

    status = dataValueToGeos(adf_scb, dv1, handle, &linestring, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_pointn: Failed to convert data value to GEOSGeometry."));
    }

    points = GEOSGeomGetNumPoints_r(handle, linestring);
    /* OGC starts arrays at 1, GEOS does not, subtract */
    position--;

    /*
     * Out of bounds, return NULL
     */
    if(position >= points || position < 0)
    {
        GEOSGeom_destroy_r(handle, linestring);
        finishGEOS_r(handle);
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    point = GEOSGeomGetPointN_r(handle, linestring, position);

    if(point == NULL)
    {
        GEOSGeom_destroy_r(handle, linestring);
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
            "PointN: Failed to get point on LineString."));
    }

    status = geosToDataValue(adf_scb, point, handle, rdv, NULL, FALSE);

    /*
     * clean up
     */
    GEOSGeom_destroy_r(handle, linestring);
    GEOSGeom_destroy_r(handle, point);
    finishGEOS_r(handle);

    return status;
#endif
}

/*
 * Returns the first point in a LineString
 */
DB_STATUS
adu_startpoint(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    GEOSGeometry *curve = NULL, *point = NULL;
    GEOSContextHandle_t handle;

    if(ADI_ISNULL_MACRO(dv1))
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    /*
     * If invalid type is passed, return NULL
     * NOTE: This is how PostGIS handles it
     */
    if(abs(dv1->db_datatype) != DB_LINE_TYPE &&
            abs(dv1->db_datatype) != DB_MLINE_TYPE)
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    handle = initGEOS_r( geos_Notice, geos_Error );
    /* 
     * We need to handle nullable data types, adu expects them to be 
     * non-nullable
     */
    dv1->db_datatype = abs(dv1->db_datatype);

    status = dataValueToGeos(adf_scb, dv1, handle, &curve, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_startpoint: Failed to convert data value to GEOSGeometry."));
    }

    point = GEOSGeomGetStartPoint_r(handle, curve);

    if(point == NULL)
    {
        GEOSGeom_destroy_r(handle, curve);
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
            "StartPoint: Failed to get point on Curve."));
    }

    status = geosToDataValue(adf_scb, point, handle, rdv, NULL, FALSE);

    /*
     * clean up
     */
    GEOSGeom_destroy_r(handle, curve);
    GEOSGeom_destroy_r(handle, point);
    finishGEOS_r(handle);

    return status;
#endif
}

/*
 * Returns the last point in a LineString
 */
DB_STATUS
adu_endpoint(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    GEOSGeometry *curve = NULL, *point = NULL;
    GEOSContextHandle_t handle;

    if(ADI_ISNULL_MACRO(dv1))
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    /*
     * If invalid type is passed, return NULL
     * NOTE: This is how PostGIS handles it
     */
    if(abs(dv1->db_datatype) != DB_LINE_TYPE &&
            abs(dv1->db_datatype) != DB_MLINE_TYPE)
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    handle = initGEOS_r( geos_Notice, geos_Error );
    /* 
     * We need to handle nullable data types, adu expects them to be 
     * non-nullable
     */
    dv1->db_datatype = abs(dv1->db_datatype);

    status = dataValueToGeos(adf_scb, dv1, handle, &curve, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_endpoint: Failed to convert data value to GEOSGeometry."));
    }

    point = GEOSGeomGetEndPoint_r(handle, curve);
    if(point == NULL)
    {
        GEOSGeom_destroy_r(handle, curve);
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
            "EndPoint: Failed to get point on Curve."));
    }

    status = geosToDataValue(adf_scb, point, handle, rdv, NULL, FALSE);

    /*
     * clean up
     */
    GEOSGeom_destroy_r(handle, curve);
    GEOSGeom_destroy_r(handle, point);
    finishGEOS_r(handle);

    return status;
#endif

}

/*
 * Returns 1 if LineString is closed
 */
DB_STATUS
adu_isclosed(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    GEOSGeometry *curve = NULL;
    const GEOSGeometry *pcurve;
    GEOSContextHandle_t handle;
    i4 isClosed = 1;
    i4 i, curves = 0;

    if(ADI_ISNULL_MACRO(dv1))
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    /*
     * If not a linestring always return TRUE
     * NOTE: This is how PostGIS handles it
     */
    if(abs(dv1->db_datatype) != DB_LINE_TYPE && 
       abs(dv1->db_datatype) != DB_MLINE_TYPE)
    {
        MEcopy(&isClosed, sizeof(i4), rdv->db_data);
        return E_DB_OK;
    }

    handle = initGEOS_r( geos_Notice, geos_Error );

    status = dataValueToGeos(adf_scb, dv1, handle, &curve, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_isclosed: Failed to convert data value to GEOSGeometry."));
    }

    /*
     * If this is a multilinestring and ANY of the linestrings
     * aren't closed this will return false.
     * NOTE: This is how PostGIS does it.
     */
    if(abs(dv1->db_datatype) == DB_MLINE_TYPE)
    {
        curves = GEOSGetNumGeometries_r(handle, curve);
        for(i = 0; i < curves; i++)
        {
            pcurve = GEOSGetGeometryN_r(handle, curve, i);
            isClosed = GEOSisClosed_r(handle, pcurve);
            if(!isClosed)
                break;
        }
    }
    else
    {
        isClosed = GEOSisClosed_r(handle, curve);
    }

    MEcopy(&isClosed, sizeof(i4), rdv->db_data);
    /*
     * Clean up
     */
    GEOSGeom_destroy_r(handle, curve);
    finishGEOS_r(handle);

    return status;
#endif
}

/*
 * Returns 1 if LineString is a ring
 */
DB_STATUS
adu_isring(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    GEOSGeometry *curve = NULL;
    GEOSContextHandle_t handle;
    i4 isRing = 0;

    if(ADI_ISNULL_MACRO(dv1))
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    if(abs(dv1->db_datatype) != DB_LINE_TYPE)
    {
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
                "adu_isring: Only LineString is allowed."));
    }
    handle = initGEOS_r( geos_Notice, geos_Error );

    status = dataValueToGeos(adf_scb, dv1, handle, &curve, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_isring: Failed to convert data value to GEOSGeometry."));
    }


    isRing = GEOSisRing_r(handle, curve);

    MEcopy(&isRing, sizeof(i4), rdv->db_data);
    /*
     * Clean up
     */
    GEOSGeom_destroy_r(handle, curve);
    finishGEOS_r(handle);

    return status;
#endif
}

/*
 * Returns the length of a LineString
 */
DB_STATUS
adu_stlength(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    GEOSGeometry *curve = NULL;
    const GEOSGeometry *pcurve;
    GEOSContextHandle_t handle;
    f8 length = 0, templ = 0;
    i4 i;

    if(ADI_ISNULL_MACRO(dv1))
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    /*
     * If not a linestring type return 0
     * NOTE: This is how PostGIS handles it
     */
    if(abs(dv1->db_datatype) != DB_LINE_TYPE && 
       abs(dv1->db_datatype) != DB_MLINE_TYPE)
    {
        MEcopy(&length, sizeof(f8), rdv->db_data);
        return E_DB_OK;
    }
    handle = initGEOS_r( geos_Notice, geos_Error );

    status = dataValueToGeos(adf_scb, dv1, handle, &curve, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_stlength: Failed to convert data value to GEOSGeometry."));
    }

    /*
     * If this is a multilinestring take each linestring
     * calculate it's length and add them all together.
     * NOTE: This is how PostGIS does it
     */
    if(abs(dv1->db_datatype == DB_MLINE_TYPE))
    {
        i4 geoms = GEOSGetNumGeometries_r(handle, curve);
        for(i = 0; i < geoms; i++)
        {
            pcurve = GEOSGetGeometryN_r(handle, curve, i);
            GEOSGeomGetLength_r(handle, pcurve, &templ);
            length += templ;
        }
    }
    else
    {
        GEOSGeomGetLength_r(handle, curve, &length);
    }

    MEcopy(&length, sizeof(f8), rdv->db_data);

    /*
     * Clean up
     */
    GEOSGeom_destroy_r(handle, curve);
    finishGEOS_r(handle);

    return status;
#endif
}

/*
 * Return centre point of a surface
 */
DB_STATUS
adu_centroid(
ADF_CB           *adf_scb,
DB_DATA_VALUE    *dv1,
DB_DATA_VALUE    *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    GEOSGeometry *surface = NULL, *point = NULL;
    GEOSContextHandle_t handle;

    if(ADI_ISNULL_MACRO(dv1))
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    handle = initGEOS_r( geos_Notice, geos_Error );

    status = dataValueToGeos(adf_scb, dv1, handle, &surface, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_centroid: Failed to convert data value to GEOSGeometry."));
    }

    point = GEOSGetCentroid_r(handle, surface);

    if(point == NULL)
    {
        GEOSGeom_destroy_r(handle, surface);
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
            "Centroid: Failed to get centroid on Surface."));
    }

    status = geosToDataValue(adf_scb, point, handle, rdv, NULL, FALSE);

    /*
     * Clean up
     */
    GEOSGeom_destroy_r(handle, surface);
    GEOSGeom_destroy_r(handle, point);
    finishGEOS_r(handle);

    return status;
#endif
}

/*
 * Returns a point on a surface
 */
DB_STATUS
adu_pointonsurface(
ADF_CB           *adf_scb,
DB_DATA_VALUE    *dv1,
DB_DATA_VALUE    *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    GEOSGeometry *surface = NULL, *point = NULL;
    GEOSContextHandle_t handle;

    if(ADI_ISNULL_MACRO(dv1))
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    handle = initGEOS_r( geos_Notice, geos_Error );

    status = dataValueToGeos(adf_scb, dv1, handle, &surface, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
        "adu_pointonsurface: Failed to convert data value to GEOSGeometry."));
    }

    point = GEOSPointOnSurface_r(handle, surface);

    if(point == NULL)
    {
        GEOSGeom_destroy_r(handle, surface);
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
            "PointOnSurface: Failed to get PointOnSurface."));
    }

    status = geosToDataValue(adf_scb, point, handle, rdv, NULL, FALSE);

    /*
     * Clean up
     */
    GEOSGeom_destroy_r(handle, surface);
    GEOSGeom_destroy_r(handle, point);
    finishGEOS_r(handle);

    return status;
#endif
}

/*
 * Returns the area of a surface
 */
DB_STATUS
adu_area(
ADF_CB           *adf_scb,
DB_DATA_VALUE    *dv1,
DB_DATA_VALUE    *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    GEOSGeometry *surface = NULL;
    GEOSContextHandle_t handle;
    f8 area = 0;

    if(ADI_ISNULL_MACRO(dv1))
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    if(abs(dv1->db_datatype) != DB_POLY_TYPE && 
       abs(dv1->db_datatype) != DB_MPOLY_TYPE)
    {
        /*
         * If not a polygon, the area is 0
         * NOTE: This is how PostGIS handles it
         */
        MEcopy(&area, sizeof(f8), rdv->db_data);
        return E_DB_OK;
    }
    handle = initGEOS_r( geos_Notice, geos_Error );

    status = dataValueToGeos(adf_scb, dv1, handle, &surface, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_area: Failed to convert data value to GEOSGeometry."));
    }

    if(GEOSArea_r(handle, surface, &area) == 0)
    {
        GEOSGeom_destroy_r(handle, surface);
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
            "adu_area: Failed to calculate area."));
    }

    MEcopy(&area, sizeof(f8), rdv->db_data);

    /*
     * Clean up
     */
    GEOSGeom_destroy_r(handle, surface);
    finishGEOS_r(handle);

    return status;
#endif
}

/*
 * Returns the exterior ring of a polygon
 */
DB_STATUS
adu_exteriorring(
ADF_CB           *adf_scb,
DB_DATA_VALUE    *dv1,
DB_DATA_VALUE    *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    GEOSGeometry *polygon = NULL;
    const GEOSGeometry *linestring = NULL;
    GEOSContextHandle_t handle;

    if(ADI_ISNULL_MACRO(dv1))
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    if(abs(dv1->db_datatype) != DB_POLY_TYPE)
    {
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
                "adu_exteriorring: Only Polygon allowed."));
    }
    handle = initGEOS_r( geos_Notice, geos_Error );

    status = dataValueToGeos(adf_scb, dv1, handle, &polygon, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_exterioring: Failed to convert data value to GEOSGeometry."));
    }

    linestring = GEOSGetExteriorRing_r(handle, polygon);

    if(linestring == NULL)
    {
        GEOSGeom_destroy_r(handle, polygon);
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
            "adu_exteriorring: Failed to get exterior ring."));
    }

    status = geosToDataValue(adf_scb, (GEOSGeometry *)linestring, handle, rdv, 
                             NULL, FALSE);

    /*
     * Clean up
     */
    GEOSGeom_destroy_r(handle, polygon);
    finishGEOS_r(handle);

    return status;
#endif
}

/*
 * Returns the number of interior rings of a polygon
 */
DB_STATUS
adu_numinteriorring(
ADF_CB           *adf_scb,
DB_DATA_VALUE    *dv1,
DB_DATA_VALUE    *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    GEOSGeometry *polygon = NULL;
    const GEOSGeometry *one_poly;
    GEOSContextHandle_t handle;
    i4 numRings = 0;

    if(ADI_ISNULL_MACRO(dv1))
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    /*
     * If it's not a polygon, return NULL
     * NOTE: This is how PostGIS handles it
     */
    if(abs(dv1->db_datatype) != DB_POLY_TYPE &&
       abs(dv1->db_datatype) != DB_MPOLY_TYPE)
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    handle = initGEOS_r( geos_Notice, geos_Error );

    /* 
     * We need to handle nullable data types, adu expects them to be 
     * non-nullable
     */
    dv1->db_datatype = abs(dv1->db_datatype);

    status = dataValueToGeos(adf_scb, dv1, handle, &polygon, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
        "adu_numinteriorring: Failed to convert data value to GEOSGeometry."));
    }

    /*
     * If it's a multipolygon only count the first polygon's
     * interior rings
     * NOTE: This is how PostGIS handles it
     */
    if(GEOSGeomTypeId_r(handle, polygon) == GEOS_MULTIPOLYGON)
    {
        if(GEOSGetNumGeometries_r(handle, polygon) >= 1)
        {
            one_poly = GEOSGetGeometryN_r(handle, polygon, 0);
            numRings = GEOSGetNumInteriorRings_r(handle, one_poly);
        }
        else
        {
            numRings = 0;
        }
    }
    else
    {
        numRings = GEOSGetNumInteriorRings_r(handle, polygon);
    }

    MEcopy(&numRings, sizeof(i4), rdv->db_data);

    /*
     * Clean up
     */
    GEOSGeom_destroy_r(handle, polygon);
    finishGEOS_r(handle);

    return status;
#endif
}

/*
 * Returns the nth interior ring of a polygon
 */
DB_STATUS
adu_interiorringn(
ADF_CB           *adf_scb,
DB_DATA_VALUE    *dv1,
DB_DATA_VALUE    *dv2,
DB_DATA_VALUE    *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    GEOSGeometry *polygon = NULL;
    const GEOSGeometry *linestring = NULL;
    GEOSContextHandle_t handle;
    i4 n = 0, numRings = 0;

    if(ADI_ISNULL_MACRO(dv1) || ADI_ISNULL_MACRO(dv2))
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    if(abs(dv1->db_datatype) != DB_POLY_TYPE)
    {
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
                "adu_interiorring: Only Polygon allowed."));
    }

    switch(dv2->db_length)
    {
    case 1:
        n = *(i1 *) dv2->db_data;
        break;
    case 2:
        n = *(i2 *) dv2->db_data;
        break;
    case 4:
        n = *(i4 *) dv2->db_data;
        break;
    case 8:
        n = *(i8 *) dv2->db_data;
        break;
    default:
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_interiorring: invalid length passed with data value 2"));
    }
    handle = initGEOS_r( geos_Notice, geos_Error );

    /*
     * We need to handle nullable data types, adu expects them to be 
     * non-nullable
     */
    dv1->db_datatype = abs(dv1->db_datatype);

    status = dataValueToGeos(adf_scb, dv1, handle, &polygon, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
        "adu_interiorringn: Failed to convert data value to GEOSGeometry."));
    }

    /*
     * Make sure there are rings to get and n is within bounds
     */
    numRings = GEOSGetNumInteriorRings_r(handle, polygon);

    /* OGC starts arrays at 1, GEOS does not, subtract */
    n--;

    /*
     * Out of bounds, return NULL
     */
    if(n >= numRings || n < 0)
    {
        GEOSGeom_destroy_r(handle, polygon);
        finishGEOS_r(handle);
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    linestring = GEOSGetInteriorRingN_r(handle, polygon, n);

    if(linestring == NULL)
    {
        GEOSGeom_destroy_r(handle, polygon);
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
            "adu_interiorringn: Failed to get interior ring."));
    }

    status = geosToDataValue(adf_scb, (GEOSGeometry *)linestring, handle, rdv,
                             NULL, FALSE);

    /*
     * Clean up
     */
    GEOSGeom_destroy_r(handle, polygon);
    finishGEOS_r(handle);

    return status;
#endif
}

/*
 * Returns the number of geometries in a collection
 */
DB_STATUS
adu_numgeometries(
ADF_CB           *adf_scb,
DB_DATA_VALUE    *dv1,
DB_DATA_VALUE    *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    GEOSGeometry *geomcoll = NULL;
    GEOSContextHandle_t handle;
    i4 numGeoms = 0;
    i4 dt = abs(dv1->db_datatype);

    if(ADI_ISNULL_MACRO(dv1))
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    /*
     * If not a type of geometrycollection, return NULL
     * NOTE: This is how PostGIS handles it
     */
    if(dt != DB_GEOMC_TYPE && dt != DB_MPOINT_TYPE && dt != DB_MPOLY_TYPE
            && dt != DB_MLINE_TYPE)
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }
    handle = initGEOS_r( geos_Notice, geos_Error );

    /* 
     * We need to handle nullable data types, adu expects them to be 
     * non-nullable
     */
    dv1->db_datatype = abs(dv1->db_datatype);

    status = dataValueToGeos(adf_scb, dv1, handle, &geomcoll, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
        "adu_numgeometries: Failed to convert data value to GEOSGeometry."));
    }

    numGeoms = GEOSGetNumGeometries_r(handle, geomcoll);

    MEcopy(&numGeoms, sizeof(i4), rdv->db_data);

    /*
     * Clean up
     */
    GEOSGeom_destroy_r(handle, geomcoll);
    finishGEOS_r(handle);

    return status;
#endif
}

/*
 * Returns the nth geometry in a collection
 */
DB_STATUS
adu_geometryn(
ADF_CB           *adf_scb,
DB_DATA_VALUE    *dv1,
DB_DATA_VALUE    *dv2,
DB_DATA_VALUE    *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    GEOSGeometry *geomcoll = NULL;
    const GEOSGeometry *geom = NULL;
    GEOSContextHandle_t handle;
    i4 dt = abs(dv1->db_datatype), numGeoms = 0, n = 0;

    if(ADI_ISNULL_MACRO(dv1) || ADI_ISNULL_MACRO(dv2))
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    /*
     * If a non-collection type is passed, return NULL
     * NOTE: This is how PostGIS handles it
     */
    if(dt != DB_GEOMC_TYPE && dt != DB_MPOINT_TYPE && dt != DB_MPOLY_TYPE
            && dt != DB_MLINE_TYPE)
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    switch(dv2->db_length)
    {
    case 1:
        n = *(i1 *) dv2->db_data;
        break;
    case 2:
        n = *(i2 *) dv2->db_data;
        break;
    case 4:
        n = *(i4 *) dv2->db_data;
        break;
    case 8:
        n = *(i8 *) dv2->db_data;
        break;
    default:
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_geometryn: invalid length passed with data value 2"));
    }

    /* 
     * We need to handle nullable data types, adu expects them to be 
     * non-nullable
     */
    dv1->db_datatype = abs(dv1->db_datatype);

    handle = initGEOS_r( geos_Notice, geos_Error );

    status = dataValueToGeos(adf_scb, dv1, handle, &geomcoll, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_geometryn: Failed to convert data value to GEOSGeometry."));
    }

    numGeoms = GEOSGetNumGeometries_r(handle, geomcoll);
    /* OGC starts arrays at 1, GEOS does not, subtract */
    n--;

    /*
     * Out of bounds, return NULL
     */
    if(n >= numGeoms || n < 0)
    {
        GEOSGeom_destroy_r(handle, geomcoll);
        finishGEOS_r(handle);
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    geom = GEOSGetGeometryN_r(handle, geomcoll, n);

    if(geom == NULL)
    {
        GEOSGeom_destroy_r(handle, geomcoll);
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
                "adu_geometryn: Failed to get Nth Geometry."));
    }

    status = geosToDataValue(adf_scb, (GEOSGeometry *)geom, handle, rdv, NULL,
                             FALSE);

    /*
     * Clean up
     */
    GEOSGeom_destroy_r(handle, geomcoll);
    finishGEOS_r(handle);

    return status;
#endif
}

/*
 * Returns the SRID of a geometry
 */
DB_STATUS
adu_srid(
ADF_CB            *adf_scb,
DB_DATA_VALUE    *dv1,
DB_DATA_VALUE    *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    i4 srid = 0;
    GEOSGeometry *geometry;
    GEOSContextHandle_t handle;

    handle = initGEOS_r( geos_Notice, geos_Error );

    status = dataValueToGeos(adf_scb, dv1, handle, &geometry, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_srid: Failed to convert data value to GEOSGeometry."));
    }

    srid = GEOSGetSRID_r(handle, geometry);

    if(srid == 0)
    {
        status = E_AD5601_GEOSPATIAL_INTERNAL;
    }
    else
    {
        MEcopy(&srid, sizeof(i4), rdv->db_data);
    }

    GEOSGeom_destroy_r(handle, geometry);
    finishGEOS_r(handle);

    return status;
#endif
}

/*
 * OGC 1.1 standard SQL function on type Geometry
 * Return 1 if the given geometry is empty, 0 if it's not,
 * -1 otherwise.
 */
DB_STATUS
adu_isempty(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    GEOSGeometry *geometry = NULL;
    GEOSContextHandle_t handle;
    i4 isEmpty = 0;

    if(ADI_ISNULL_MACRO(dv1))
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    handle = initGEOS_r( geos_Notice, geos_Error );

    status = dataValueToGeos(adf_scb, dv1, handle, &geometry, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_isEmpty: Failed to convert data value to GEOSGeometry."));
    }

    isEmpty = GEOSisEmpty_r(handle, geometry);

    MEcopy(&isEmpty, sizeof(i4), rdv->db_data);
    /*
     * Clean up
     */
    GEOSGeom_destroy_r(handle, geometry);
    finishGEOS_r(handle);

    return status;
#endif
}

/*
 * OGC 1.1 standard SQL function on type Geometry
 * Return 1 if the given geometry is simple, 0 if it's not,
 * -1 otherwise.
 */
DB_STATUS
adu_issimple(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    GEOSGeometry *geometry = NULL;
    GEOSContextHandle_t handle;
    i4 isSimple = 0;

    if(ADI_ISNULL_MACRO(dv1))
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    handle = initGEOS_r( geos_Notice, geos_Error );

    status = dataValueToGeos(adf_scb, dv1, handle, &geometry, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_isSimple: Failed to convert data value to GEOSGeometry."));
    }

    /*
     * If it's called on a geometry collection, return an error
     * NOTE: This is how PostGIS handles it
     */
    if(GEOSGeomTypeId_r(handle, geometry) == GEOS_GEOMETRYCOLLECTION)
    {
        GEOSGeom_destroy_r(handle, geometry);
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
            "IsSimple does not support GeometryCollections."));
    }

    isSimple = GEOSisSimple_r(handle, geometry);

    MEcopy(&isSimple, sizeof(i4), rdv->db_data);
    /*
     * Clean up
     */
    GEOSGeom_destroy_r(handle, geometry);
    finishGEOS_r(handle);

    return status;
#endif
}

/*
 * OGC 1.1 standard SQL function on type Geometry
 * Return the number of points in this linestring.
 */
DB_STATUS
adu_numpoints(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    GEOSGeometry *geometry = NULL;
    const GEOSGeometry *line;
    GEOSContextHandle_t handle;
    i4 np = 0, geoms;

    /* Initialize the handle for GEOS */
    if(ADI_ISNULL_MACRO(dv1))
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    /*
     * Make sure it's a linestring or mlinestring
     * If not, return NULL
     * NOTE: This is how PostGIS handles it
     */
    if(abs(dv1->db_datatype) != DB_LINE_TYPE &&
       abs(dv1->db_datatype) != DB_MLINE_TYPE)
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    handle = initGEOS_r(geos_Notice, geos_Error);

    /* 
     * We need to handle nullable data types, adu expects them to be 
     * non-nullable
     */
    dv1->db_datatype = abs(dv1->db_datatype);

    /* 
     * Convert the geometry out from adf session control block and pt data
     * value
     */
    status = dataValueToGeos(adf_scb, dv1, handle, &geometry, FALSE);
    if(status != E_DB_OK)
    {
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
                "adu_numpoints: Failed to convert data value to GEOSGeometry."));
    }
    
    /*
     * If it's a multi linestring, only count from the first linestring
     * NOTE: This is how PostGIS handles it
     */
    if(GEOSGeomTypeId_r(handle, geometry) == GEOS_MULTILINESTRING)
    {
        geoms = GEOSGetNumGeometries_r(handle, geometry);
        if(geoms >= 1)
        {
            line = GEOSGetGeometryN_r(handle, geometry, 0);
            np = GEOSGeomGetNumPoints_r(handle, line);
        }
        else
        {
            np = 0;
        }
    }
    else
    {
        /* Get the number of points from the line */
        np = GEOSGeomGetNumPoints_r(handle, geometry);
    }

    if(np == -1)
    {
        GEOSGeom_destroy_r(handle, geometry);
        finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
            "adu_numpoints: Failed to get number of points from the LineString."));
    }

    /* Copy the resulting x value into return data value */
    MEcopy(&np, sizeof(i4), rdv->db_data);

    /* Clean stuffs up */
    GEOSGeom_destroy_r(handle, geometry);
    finishGEOS_r(handle);
	
    return E_DB_OK;
#endif
}

DB_STATUS
adu_geom_name(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    i4 i = 0;
    const char *invalid_type = "INVALID_TYPE";
    i4 dt;

    switch(dv1->db_length)
    {
        case 1:
            dt = *(i1 *) dv1->db_data;
            break;
        case 2:
            dt = *(i2 *) dv1->db_data;
            break;
        case 4:
            dt = *(i4 *) dv1->db_data;
            break;
        case 8:
            dt = *(i8 *) dv1->db_data;
            break;
        default:
            return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
                "adu_geom_name: invalid length passed with data value 1"));
    }


    /*
     * This search algorithm is ineffcient and will be changed
     */
    while(geometry_types[i].type_code != -1)
    {
        if(geometry_types[i].type_code == dt)
        {
            STmove(geometry_types[i].type_name, ' ', rdv->db_length, 
                   (char *) rdv->db_data);
            break;
        }

        i++;
    }

    if(geometry_types[i].type_code == -1)
    {
        STmove(invalid_type, ' ', rdv->db_length, (char *) rdv->db_data);
    }

    return status;
#endif
}

DB_STATUS
adu_geom_dimensions(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    i4 i = 0;
    i4 dt;

    switch(dv1->db_length)
    {
        case 1:
            dt = *(i1 *) dv1->db_data;
            break;
        case 2:
            dt = *(i2 *) dv1->db_data;
            break;
        case 4:
            dt = *(i4 *) dv1->db_data;
            break;
        case 8:
            dt = *(i8 *) dv1->db_data;
            break;
        default:
            return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_geom_dimensions: invalid length passed with data value 1"));
    }

    /*
     * This search algorithm is ineffcient and will be changed.
     */
    while(geometry_types[i].type_code != -1)
    {
        if(geometry_types[i].type_code == dt)
        {
            break;
        }

        i++;
    }

    MEcopy(&geometry_types[i].dimensions, rdv->db_length, rdv->db_data);

    return status;
#endif
}

DB_STATUS
adu_point_x(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *pt_dv,
DB_DATA_VALUE   *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
	DB_STATUS status = E_DB_OK;
	GEOSGeometry *geometry = NULL;
	GEOSContextHandle_t handle;
	f8 x;
	
	/* Initialize the handle for GEOS */
	if(ADI_ISNULL_MACRO(pt_dv))
	{
		ADF_SETNULL_MACRO(rdv);
		return E_DB_OK;
	}
	
	/* Make sure it's a point */
	if(abs(pt_dv->db_datatype) != DB_POINT_TYPE)
	{
    	return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
		"adu_point_x: Only Point is allowed."));
	}
	
	handle = initGEOS_r(geos_Notice, geos_Error);
	
	/* 
 	 * We need to handle nullable data types, adu expects them to be 
 	 * non-nullable
 	 */
	pt_dv->db_datatype = abs(pt_dv->db_datatype);

	/*
	 * Convert the geometry out from adf session control block and pt data
	 * value
	 */
	status = dataValueToGeos(adf_scb, pt_dv, handle, &geometry, FALSE);
	if(status != E_DB_OK)
	{
		finishGEOS_r(handle);
		return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
                                "adu_point_x: Failed to convert data value to GEOSGeometry."));
	}
	
	/*
	 * If it's an empty point, return NULL
	 * NOTE: This is how PostGIS handles it
	 */
	if(GEOSisEmpty_r(handle, geometry))
	{
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
	}

	/* Get the x value from the point */
	status = GEOSGeomGetX_r(handle, geometry, &x);
	if(!status){
		GEOSGeom_destroy_r(handle, geometry);
		finishGEOS_r(handle);
		return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
				"adu_point_x: Failed to get x value from GEOSGeometry."));
	}
	
	/* Copy the resulting x value into return data value */
	MEcopy(&x, sizeof(f8), rdv->db_data);
	
	/* Clean stuffs up */
	GEOSGeom_destroy_r(handle, geometry);
	finishGEOS_r(handle);

 	return E_DB_OK;
#endif
}

DB_STATUS
adu_point_y(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *pt_dv,
DB_DATA_VALUE   *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
	DB_STATUS status = E_DB_OK;
	GEOSGeometry *geometry = NULL;
	GEOSContextHandle_t handle;
	f8 y;
	
	/* initialize the handle for GEOS */
	if(ADI_ISNULL_MACRO(pt_dv)){
		ADF_SETNULL_MACRO(rdv);
		return E_DB_OK;
	}

	/* Make sure it's a point */
	if(abs(pt_dv->db_datatype)!= DB_POINT_TYPE){
		return(adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
		"adu_point_y: Only Point is allowed."));
	}

	handle = initGEOS_r(geos_Notice, geos_Error);

    /* We need to handle nullable data types, adu expects them to be 
     * non-nullable
     */
    pt_dv->db_datatype = abs(pt_dv->db_datatype);

	/* Convert the geometry out from adf session control block and pt data 
	 * value
	 */
	status = dataValueToGeos(adf_scb, pt_dv, handle, &geometry, FALSE);
	if(status != E_DB_OK){
		finishGEOS_r(handle);
		return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
			"adu_point_y: Failed to convert data value to GEOSGeometry."));
	}

	/*
     * If it's an empty point, return NULL
     * NOTE: This is how PostGIS handles it
     */
    if(GEOSisEmpty_r(handle, geometry))
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }


	/* Get the y value from the point */
	status = GEOSGeomGetY_r(handle, geometry, &y);
	if(!status){
		GEOSGeom_destroy_r(handle, geometry);
		finishGEOS_r(handle);
		return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
				"adu_point_y: Failed to get y value from GEOSGeometry."));
	}

	/* Copy the result y value into return data value */
	MEcopy(&y, sizeof(f8), rdv->db_data);

	/* Clean stuffs up */
	GEOSGeom_destroy_r(handle, geometry);
	finishGEOS_r(handle);
	
	return E_DB_OK;
#endif
}

#ifdef _WITH_GEO
/*
 * Compare a line, linear ring or point, as a GEOS coordSeq.
 * Used for rtree indexing.  Returns 1 is g1 > g2, 0 if g1 = g2, -1 if g1< g2.
 * Each point is compared to get the final result.
 */
static i4
coordSeq_cmp(GEOSContextHandle_t handle, const GEOSGeometry *g1,
                const GEOSGeometry *g2)
{
    const GEOSCoordSequence *gcs1, *gcs2;
    unsigned int g1size, g2size, sizeCnt;
    double g1x, g2x, g1y, g2y;
    i4 result;

    gcs1 = GEOSGeom_getCoordSeq_r(handle, g1);
    gcs2 = GEOSGeom_getCoordSeq_r(handle, g2);

    if(gcs1 == NULL || gcs2 == NULL) return 0;
    
    GEOSCoordSeq_getSize_r(handle, gcs1, &g1size);
    GEOSCoordSeq_getSize_r(handle, gcs2, &g2size);
    if(g1size > g2size)
        result = 1;
    else if(g1size < g2size)
        result = -1;
    else
    {
        result = 0;
        for(sizeCnt = 0; sizeCnt < g1size; sizeCnt++)
        {
            GEOSCoordSeq_getX_r(handle, gcs1, sizeCnt, &g1x);
            GEOSCoordSeq_getX_r(handle, gcs2, sizeCnt, &g2x);    
            GEOSCoordSeq_getY_r(handle, gcs1, sizeCnt, &g1y);
            GEOSCoordSeq_getY_r(handle, gcs2, sizeCnt, &g2y);    
            if(g1x > g2x)
            {
                result = 1;
                break;
            }
            if(g1x < g2x)
            {
                result = -1;
                break;
            }
            if(g1y > g2y)
            {
                result = 1;
                break;
            }
            if(g1y < g2y)
            {
                result = -1;
                break;
            }
        }
    }
    return result;
}

/* 
 * Compares two polygons by breaking them up into their linear ring pieces and
 * called coordSeq_cmp
 */
static i4 
polygon_cmp(GEOSContextHandle_t handle, const GEOSGeometry *g1, 
               const GEOSGeometry *g2)
{
    i4 result = 0;
    const GEOSGeometry *lr1 = NULL, *lr2 = NULL;
    i4 g1rings, g2rings, ringCnt = 0;

    g1rings = GEOSGetNumInteriorRings_r(handle, g1);
    g2rings = GEOSGetNumInteriorRings_r(handle, g2);
    if(g1rings > g2rings)
        result = 1;
    else if(g1rings < g2rings)
        result = -1;
    else
    {
        while(ringCnt < g1rings && result == 0)
        {
            lr1 = GEOSGetInteriorRingN_r(handle, g1, ringCnt);
            lr2 = GEOSGetInteriorRingN_r(handle, g2, ringCnt);
            result = coordSeq_cmp(handle, lr1, lr2);
            ringCnt++;
        }
        lr1 = GEOSGetExteriorRing_r(handle, g1);
        lr2 = GEOSGetExteriorRing_r(handle, g2);
        result = coordSeq_cmp(handle, lr1, lr2);
    }
    return result;
}

/* 
 * Compares 2 geometries by determining the type and breaking the type up
 * accordingly.
 * lines, linear rings and points call coordSeq_cmp.
 * Polygons call polygon_cmp.
 * Multi points and multi lines call coorSeq_cmp for each geometry contained.
 * Multpolygon call polygon_cmp for each polygon contained
 * Geometry collections recursively call geom_cmp for each geometry contained.
 */
static i4 
geom_cmp(GEOSContextHandle_t handle, const GEOSGeometry *g1, 
            const GEOSGeometry *g2, i4 g1Type, i4 g2Type)
{
    i4 g1NumPts, g2NumPts, ptCnt;
    i4 numGeoms1, numGeoms2;
    i4 geomCount;
    i4 result;
    const GEOSGeometry *tmp1, *tmp2;

    numGeoms1 = GEOSGetNumGeometries_r(handle, g1);
    numGeoms2 = GEOSGetNumGeometries_r(handle, g2);
    g1NumPts = GEOSGetNumCoordinates_r(handle, g1);
    g2NumPts = GEOSGetNumCoordinates_r(handle, g2);

    if((numGeoms1 > numGeoms2) || (g1NumPts > g2NumPts))
        result = 1;
    else if((numGeoms1 < numGeoms2) || (g1NumPts < g2NumPts))
        result = -1;
    else
    {
        switch(g1Type)
        {
            case GEOS_POINT:
            case GEOS_LINESTRING:
            case GEOS_LINEARRING:
                result = coordSeq_cmp(handle, (const GEOSGeometry*)g1, 
                                      (const GEOSGeometry*)g2);
                break;
            case GEOS_POLYGON:
                result = polygon_cmp(handle, (const GEOSGeometry*)g1, 
                                     (const GEOSGeometry*)g2);
                break;
            case GEOS_MULTIPOINT:
            case GEOS_MULTILINESTRING:
                result = 0;
                for(geomCount = 0; geomCount < numGeoms1 && result == 0;
                    geomCount++)
                {
                    tmp1 = GEOSGetGeometryN_r(handle, g1, geomCount);
                    tmp2 = GEOSGetGeometryN_r(handle, g2, geomCount);
                    result = coordSeq_cmp(handle, tmp1, tmp2);
                } 
            case GEOS_MULTIPOLYGON:
                result = 0;
                for(geomCount = 0; geomCount < numGeoms1 && result == 0;
                    geomCount++)
                {
                    tmp1 = GEOSGetGeometryN_r(handle, g1, geomCount);
                    tmp2 = GEOSGetGeometryN_r(handle, g2, geomCount);
                    result = polygon_cmp(handle, tmp1, tmp2);
                } 
            case GEOS_GEOMETRYCOLLECTION:
                result = 0;
                for(geomCount = 0; geomCount < numGeoms1 && result == 0;
                    geomCount++)
                {
                    tmp1 = GEOSGetGeometryN_r(handle, g1, geomCount);
                    tmp2 = GEOSGetGeometryN_r(handle, g2, geomCount);
                    result = geom_cmp(handle, tmp1, tmp2,
                                  GEOSGeomTypeId_r(handle, tmp1),
                                  GEOSGeomTypeId_r(handle, tmp2));
                } 
        }
    }
}
#endif

/* 
 * Compare two geometries for rtree indexing.
 * Returns 1 for g1 > g2, 0 for g1 = g2, -1 for g1 < g2.
 * Geos equals is only a boolean return so this function breaks the geometries
 * up to compare types then each geometry point by point.
 */
DB_STATUS
adu_rtree_cmp(
ADF_CB              *adf_scb,
DB_DATA_VALUE       *dv1,
DB_DATA_VALUE       *dv2,
i4                  *cmp_result)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    GEOSContextHandle_t handle;
    GEOSGeometry *g1 = NULL, *g2 = NULL;
    i4 g1Type, g2Type;

    /* Nulls are treated as greater. */
    if(ADI_ISNULL_MACRO(dv1) && ADI_ISNULL_MACRO(dv2))
    {
        *cmp_result = 0;
        return E_DB_OK;
    }
    else if (ADI_ISNULL_MACRO(dv1))
    {
        *cmp_result = 1;
        return E_DB_OK;
    }
    else if (ADI_ISNULL_MACRO(dv2))
    {
        *cmp_result = -1;
        return E_DB_OK;
    }

    /* NBR Types can be compared directly, no geos processing required. */
    if(dv1->db_datatype == DB_NBR_TYPE && dv2->db_datatype == DB_NBR_TYPE)
    {
        geomNBR_t *nbr1, *nbr2, *nbrOut;

        nbr1 = (geomNBR_t *)dv1->db_data;
        nbr2 = (geomNBR_t *)dv2->db_data;

        if(nbr1->x_low > nbr2->x_low)
        {
            *cmp_result = 1;
            return E_DB_OK;
        }
        if(nbr1->x_low < nbr2->x_low)
        {
            *cmp_result = -1;
            return E_DB_OK;
        }
        
        if(nbr1->y_low > nbr2->y_low);
        {
            *cmp_result = 1;
            return E_DB_OK;
        }
        if(nbr1->y_low < nbr2->y_low);
        {
            *cmp_result = -1;
            return E_DB_OK;
        }

        if(nbr1->x_hi > nbr2->x_hi);
        {
            *cmp_result = 1;
            return E_DB_OK;
        }
        if(nbr1->x_hi < nbr2->x_hi);
        {
            *cmp_result = -1;
            return E_DB_OK;
        }

        if(nbr1->y_hi > nbr2->y_hi);
        {
            *cmp_result = 1;
            return E_DB_OK;
        }
        if(nbr1->y_hi < nbr2->y_hi);
        {
            *cmp_result = -1;
            return E_DB_OK;
        }

        *cmp_result = 0;
        return E_DB_OK;
    }
    else
    {
        handle = initGEOS_r( geos_Notice, geos_Error );

        status = dataValueToGeos(adf_scb, dv1, handle, &g1, FALSE);
        if(status != E_DB_OK)
        {
            finishGEOS_r(handle);
            return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_rtree_cmp: Failed to convert data value 1 to GEOSGeometry."));
        }

        status = dataValueToGeos(adf_scb, dv2, handle, &g2, FALSE);
        if(status != E_DB_OK)
        {
            GEOSGeom_destroy_r(handle, g1);
            finishGEOS_r(handle);
            return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
            "adu_rtree_cmp: Failed to convert data value 2 to GEOSGeometry."));
        }

        g1Type = GEOSGeomTypeId_r(handle, g1);
        g2Type = GEOSGeomTypeId_r(handle, g2);

        if(g1Type > g2Type)
            *cmp_result = 1;
        else if(g1Type < g2Type)
            *cmp_result = -1;
        else
            *cmp_result = geom_cmp(handle, (const GEOSGeometry*)g1, 
                                   (const GEOSGeometry*)g2, g1Type, g2Type);
    }
    return status;
#endif
}

/************************************************************************/
/*                           adjust_angles()                            */
/*                                                                      */
/*      Translate angular values between degrees and radians or back.   */
/************************************************************************/

static void 
adjust_angles( double *xyz, int num_points, double mult_factor )

{
    int i;

    for( i = 0; i < num_points; i++ )
    {
        xyz[i*3+0] *= mult_factor;
        xyz[i*3+1] *= mult_factor;
    }
}

/************************************************************************/
/*                         geos_pj_transform()                          */
/************************************************************************/
#ifdef _WITH_GEO
static int 
geos_pj_transform( GEOSGeometry *geom, GEOSContextHandle_t hCtx,
                       projPJ src_pj, projPJ dst_pj )

{
    int  geom_type = GEOSGeomTypeId_r( hCtx, geom );
    DB_STATUS           status = E_DB_OK;

    switch( geom_type )
    {
      case GEOS_POLYGON:
      {
          int   sub_geom_count = GEOSGetNumInteriorRings_r( hCtx, geom );
          int   i;

          for( i = -1; i < sub_geom_count; i++ )
          {
              const GEOSGeometry *sub_geom;

              if( i == -1 )
                  sub_geom = GEOSGetExteriorRing_r( hCtx, geom );
              else
                  sub_geom = GEOSGetInteriorRingN_r( hCtx, geom, i );
                                                     
              if( !geos_pj_transform( (GEOSGeometry *) sub_geom, hCtx, 
                                      src_pj, dst_pj ) )
                  return 0;
          }

          return 1;
      }
      break;

      case GEOS_MULTIPOINT:
      case GEOS_MULTILINESTRING:
      case GEOS_MULTIPOLYGON:
      case GEOS_GEOMETRYCOLLECTION:
      {
          int   sub_geom_count = GEOSGetNumGeometries_r( hCtx, geom );
          int   i;

          for( i = 0; i < sub_geom_count; i++ )
          {
              const GEOSGeometry *sub_geom;

              sub_geom = GEOSGetGeometryN_r( hCtx, geom, i );
              if( !geos_pj_transform( (GEOSGeometry *) sub_geom, hCtx, 
                                      src_pj, dst_pj ) )
                  return 0;
          }

          return 1;
      }
      break;

      case GEOS_POINT:
      case GEOS_LINESTRING:
      case GEOS_LINEARRING:
      {
          const GEOSCoordSequence *coords = GEOSGeom_getCoordSeq_r(hCtx,geom);
          unsigned int num_points, i, dims;
          double *xyz;
          int err;

          GEOSCoordSeq_getSize_r( hCtx, coords, &num_points );
          xyz = (double *) MEcalloc(num_points*sizeof(double)*3,&status);

          GEOSCoordSeq_getDimensions_r( hCtx, coords, &dims );

          for( i = 0; i < num_points; i++ )
          {
              GEOSCoordSeq_getX_r( hCtx, coords, i, xyz + 3*i + 0 );
              GEOSCoordSeq_getY_r( hCtx, coords, i, xyz + 3*i + 1 );
              if( dims > 2 )
              {
                  GEOSCoordSeq_getZ_r( hCtx, coords, i, xyz + 3*i + 2 );

                  /*
                  ** The following code should be removable once the patch
                  ** for geos #345 is in wide use. 
                  */
                  if( !MHdfinite(xyz[3*i+2]) )
                  {
                      xyz[3*i+2] = 0.0;
                      dims = 2;
                  }
              }
          }
          
          if( pj_is_latlong( src_pj ) )
              adjust_angles( xyz, num_points, DEG_TO_RAD );

          err = pj_transform( src_pj, dst_pj, num_points, 3, 
                              xyz + 0, xyz + 1, xyz + 2 );

          if( err != 0 )
          {
              MEfree( xyz );
              return 0;
          }
          
          if( pj_is_latlong( dst_pj ) )
              adjust_angles( xyz, num_points, RAD_TO_DEG );

          /*
          ** The following is based on the assumption that the coordinate
          ** sequence can be forcably updated in place.  Very dirty!
          */
          for( i = 0; i < num_points; i++ )
          {
              GEOSCoordSeq_setX_r( hCtx, (GEOSCoordSequence*) coords, 
                                   i, xyz[3*i + 0] );
              GEOSCoordSeq_setY_r( hCtx, (GEOSCoordSequence*) coords, 
                                   i, xyz[3*i + 1] );
              if( dims > 2 )
                  GEOSCoordSeq_setZ_r( hCtx, (GEOSCoordSequence*) coords, 
                                       i, xyz[3*i + 2] );
          }

          MEfree( xyz );

          return 1;
      }
      break;

      default:
        return 0;
    }

    return 1;
}

/************************************************************************/
/*                           ii_init_proj4()                            */
/************************************************************************/

static void 
ii_init_proj4()

{
    static int initialized = 0;

    if( initialized )
        return;

    {
        LOCATION loc;
        const char *geo_dir;

	NMloc(FILES, FILENAME & PATH, (char *) NULL, &loc);
	LOfaddpath(&loc, "geo", &loc);
        
        /* Obtusely the search path is the last place pj_open_lib() looks
         * for files, so system grid shift files will be found ahead of
         * those in the ingres geo directory 
         */

        geo_dir = loc.path;
        pj_set_searchpath( 1, &geo_dir );

        initialized = 1;
    }
}
                    
/************************************************************************/
/*                           geos_transform()                           */
/************************************************************************/

static int 
geos_transform( GEOSGeometry *geom, GEOSContextHandle_t hCtx,
                           const char *src_srs, 
                           const char *dst_srs )
{
    projPJ src_pj, dst_pj;
    int result;

    ii_init_proj4();

    src_pj = pj_init_plus( src_srs );
    if( src_pj == NULL )
        return 0;

    dst_pj = pj_init_plus( dst_srs );
    if( dst_pj == NULL )
        return 0;

    result = geos_pj_transform( geom, hCtx, src_pj, dst_pj );

    pj_free( src_pj );
    pj_free( dst_pj );

    return result;
}
#endif

/*
 * Perform reprojection of a geometry. 
 */
DB_STATUS
adu_transform(
ADF_CB          *adf_scb,
DB_DATA_VALUE   *dv1,
DB_DATA_VALUE   *dv2,
DB_DATA_VALUE   *rdv)
{
#ifndef _WITH_GEO
    return (adu_error(adf_scb, E_AD5606_SPATIAL_NOT_SUPPORTED, 2, 0));
#else
    DB_STATUS status = E_DB_OK;
    GEOSGeometry *geom = NULL;
    GEOSContextHandle_t handle;
    i4 dt = abs(dv1->db_datatype);
    int dst_srid, src_srid;
    DB_SPATIAL_REF_SYS src_srs_row, dst_srs_row;

    if(ADI_ISNULL_MACRO(dv1) || ADI_ISNULL_MACRO(dv2))
    {
        ADF_SETNULL_MACRO(rdv);
        return E_DB_OK;
    }

    switch(dv2->db_length)
    {
      case 1:
        dst_srid = *(i1 *) dv2->db_data;
        break;
      case 2:
        dst_srid = *(i2 *) dv2->db_data;
        break;
      case 4:
        dst_srid = *(i4 *) dv2->db_data;
        break;
      case 8:
        dst_srid = *(i8 *) dv2->db_data;
        break;
      default:
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
                          "adu_transform: invalid srid passed with data value 2"));
    }

    /* 
     * Transform geometry.
     */
    handle = initGEOS_r( geos_Notice, geos_Error );

    status = dataValueToGeos(adf_scb, dv1, handle, &geom, FALSE);
    if(status != E_DB_OK)
    {
    	finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
                          "adu_transform: Failed to convert data value to GEOSGeometry."));
    }

    src_srid = GEOSGetSRID_r( handle, geom );

    if( src_srid < 1 )
    {
    	finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
                          "adu_transform: source geometry lacks SRID."));
    }

    /*
     * Lookup SRIDs to get proj.4 coordinate system definitions.
     */
    
    status = getSRS( adf_scb, &src_srs_row, src_srid );
    if( status != E_DB_OK )
        return status;

    if( STlen(src_srs_row.srs_proj4text) == 0 )
    {
    	finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
                          "adu_transform: source SRID %d lacks proj4text.",
                          src_srid));
    }

    status = getSRS( adf_scb, &dst_srs_row, dst_srid );
    if( status != E_DB_OK )
        return status;

    if( STlen(dst_srs_row.srs_proj4text) == 0 )
    {
    	finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
                          "adu_transform: destination SRID %d lacks proj4text.",
                          dst_srid));
    }

    /*
     *  Perform the transformation.
     */
    if( !geos_transform( geom, handle, 
                         src_srs_row.srs_proj4text, 
                         dst_srs_row.srs_proj4text ) )
    {
        GEOSGeom_destroy_r(handle, geom);
    	finishGEOS_r(handle);
        return (adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
                          "adu_transform: Failure in geos_transform()."));
    }

    /* 
     * Convert back into WKB. 
     */
    status = geosToDataValue(adf_scb, geom, handle, rdv, NULL, FALSE);

    /*
     * Clean up
     */
    GEOSGeom_destroy_r(handle, geom);
    finishGEOS_r(handle);

    return status;
#endif
}

#ifdef _WITH_GEO
/*
 * Takes an ADF_CB, DB_SPATIAL_REF_SYS pointer and an i4 srid
 * All errors are handled in this function, if it returns
 * anything but E_DB_OK, just abort, do not create a new
 * error message.
 */
DB_STATUS
getSRS(ADF_CB *adf_scb, DB_SPATIAL_REF_SYS *srs, i4 srid)
{
    DB_STATUS status;
    i4 errcode;

    /*
     * Safety check
     */
    if(srs == NULL)
    {
        return (adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
                          "populateSRS: passed a null pointer to srs."));
    }

    /*
     * Assign the SRID to the struct then look it up using the FEXI
     */
    srs->srs_srid = srid;
    status = (*Adf_globs->Adi_fexi[ADI_08GETSRS_FEXI].adi_fcn_fexi) (srs, &errcode);

    /*
     * If we have an error let's translate it if we can.
     */
    if(status != E_DB_OK)
    {
        switch(errcode)
        {
        case E_AD5603_NO_TRANSACTION:
            adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
                      "No transaction found, couldn't look up spatial reference system.");
            break;
        case E_AD5604_SRS_NONEXISTENT:
            adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
                      "spatial_ref_sys table not found.");
            break;
        case E_AD5605_INVALID_SRID:
            adu_error(adf_scb, E_AD5600_GEOSPATIAL_USER, 2, 0,
                      "An invalid SRID was specified.");
            break;
        default:
            adu_error(adf_scb, E_AD5601_GEOSPATIAL_INTERNAL, 2, 0,
                      "populateSRS: failed to retrieve spatial reference system");
        }
        return status;
    }

    return E_DB_OK;
}
#endif
