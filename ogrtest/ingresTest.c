#include <iiapi.h>
#include <ogr_api.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAXWKTLEN 1047551

static void print_error(IIAPI_GENPARM *genParm);
const char *load_test_wkt(const char *file_name);
void load_shapefile(const char *shape_file);

const char *functions[] =
{
    "Intersects",
    "Equals",
    "Disjoint",
    "Touches",
    "Crosses",
    "Within",
    "Contains",
    "Overlaps",
    "Intersection",
    "Union",
    "Difference",
    "Distance",
    NULL
};


const char *load_test_wkt(const char *file_name)
{
    FILE *fp;
    static char result[MAXWKTLEN];
    fp = fopen(file_name, "r");

    if(fp == NULL)
    {
        printf("Couldn't open %s.\n", file_name);
        return NULL;
    }

    fgets(result, MAXWKTLEN, fp);

    return result;
}

void load_shapefile(const char *shape_file)
{
    OGRDataSourceH data_source = NULL;
    OGRSFDriverH read_driver = NULL;
    OGRSFDriverH write_driver = NULL;
    OGRDataSourceH write_source = NULL;
    char *options[] = { "OVERWRITE=YES" };
    OGRRegisterAll(); //Register all drivers
    read_driver = OGRGetDriverByName("ESRI Shapefile");
    write_driver = OGRGetDriverByName("Ingres");
    data_source = OGR_Dr_Open(read_driver, shape_file, FALSE);
    write_source = OGR_Dr_CopyDataSource(write_driver, data_source, "@driver=ingres,dbname=geo", options);
    OGR_DS_Destroy(write_source);
}

void load_ingres(OGRDataSourceH data_source)
{

}

static void print_error(IIAPI_GENPARM *genParm)
{
    IIAPI_GETEINFOPARM getInfoParm;
    getInfoParm.ge_errorHandle = genParm->gp_errorHandle;
    IIapi_getErrorInfo(&getInfoParm);

    if(getInfoParm.ge_status == IIAPI_ST_SUCCESS)
    {
        switch(getInfoParm.ge_type)
        {
            case IIAPI_GE_ERROR:
                printf("ERROR:\n");
                break;
            case IIAPI_GE_WARNING:
                printf("WARNING:\n");
                break;
            case IIAPI_GE_MESSAGE:
                printf("MESSAGE:\n");
                break;
        }
        printf("Info: '%s' 0x%x: %s\n", getInfoParm.ge_SQLSTATE, getInfoParm.ge_errorCode,
               getInfoParm.ge_message ? getInfoParm.ge_message : "No Message");
    }
    else
    {
        switch(genParm->gp_status)
        {
            case IIAPI_ST_SUCCESS:
                printf("IIAPI_ST_SUCCESS\n");
                break;
            case IIAPI_ST_MESSAGE:
                printf("IIAPI_ST_MESSAGE\n");
                break;
            case IIAPI_ST_WARNING:
                printf("IIAPI_ST_WARNING\n");
                break;
            case IIAPI_ST_NO_DATA:
                printf("IIAPI_ST_NO_DATA\n");
                break;
            case IIAPI_ST_ERROR:
                printf("IIAPI_ST_ERROR\n");
                break;
            case IIAPI_ST_FAILURE:
                printf("IIAPI_ST_FAILURE\n");
                break;
            case IIAPI_ST_NOT_INITIALIZED:
                printf("IIAPI_ST_NOT_INITIALIZED\n");
                break;
            case IIAPI_ST_INVALID_HANDLE:
                printf("IIAPI_ST_INVALID_HANDLE\n");
                break;
            case IIAPI_ST_OUT_OF_MEMORY:
                printf("IIAPI_ST_OUT_OF_MEMORY\n");
                break;
            default:
                printf("Unknown Status: %d\n", genParm->gp_status);
                break;
        }
    }
}
void execute_query(const char *query_text, II_PTR *connHandle, II_PTR *stmtHandle, II_PTR *tranHandle)
{
    IIAPI_WAITPARM waitParm = { -1 };
    IIAPI_QUERYPARM queryParm;
    IIAPI_GETDESCRPARM getDescrParm;
    IIAPI_GETCOLPARM getColParm;
    IIAPI_DATAVALUE *dataValues;
    IIAPI_CLOSEPARM closeParm;
    IIAPI_COMMITPARM commitParm;
    int i;

    memset(&queryParm, 0, sizeof(queryParm));

    queryParm.qy_connHandle = *connHandle;
    queryParm.qy_queryType = IIAPI_QT_QUERY;
    queryParm.qy_queryText = (II_CHAR *) query_text;
    
    IIapi_query(&queryParm);

    while(queryParm.qy_genParm.gp_completed == FALSE)
        IIapi_wait(&waitParm);

    
    if(queryParm.qy_genParm.gp_status != IIAPI_ST_SUCCESS)
    {
        // something went wrong again
        print_error(&queryParm.qy_genParm);
        return;
    }

    *stmtHandle = queryParm.qy_stmtHandle;
    *tranHandle = queryParm.qy_tranHandle;

    memset(&getDescrParm, 0, sizeof(getDescrParm));

    getDescrParm.gd_stmtHandle = *stmtHandle;

    IIapi_getDescriptor(&getDescrParm);

    while(getDescrParm.gd_genParm.gp_completed == FALSE)
        IIapi_wait(&waitParm);

    if(getDescrParm.gd_genParm.gp_status != IIAPI_ST_SUCCESS)
    {
        print_error(&getDescrParm.gd_genParm);
        return;
    }

    memset(&getColParm, 0, sizeof(getColParm));

    dataValues = (IIAPI_DATAVALUE *) malloc(sizeof(IIAPI_DATAVALUE) * getDescrParm.gd_descriptorCount);

    getColParm.gc_stmtHandle = *stmtHandle;
    getColParm.gc_columnData = dataValues;
    getColParm.gc_rowCount = 1; //One at a time
    getColParm.gc_columnCount = getDescrParm.gd_descriptorCount;

    for(i = 0; i < getDescrParm.gd_descriptorCount; i++)
    {
        getColParm.gc_columnData[i].dv_value = malloc(getDescrParm.gd_descriptor[i].ds_length+1);
    }

    do
    {
        IIapi_getColumns(&getColParm);

        while(getColParm.gc_genParm.gp_completed == FALSE)
            IIapi_wait(&waitParm);

        if(getColParm.gc_genParm.gp_status == IIAPI_ST_NO_DATA)
            break;

        if(getColParm.gc_genParm.gp_status != IIAPI_ST_SUCCESS)
        {
            print_error(&getColParm.gc_genParm);
            return;
        }
        for(i = 0; i < getColParm.gc_columnCount; i++)
        {
            char *value = (char *) getColParm.gc_columnData[i].dv_value;
            value[dataValues[i].dv_length] = '\0';
            //printf("%s = %d\n", getDescrParm.gd_descriptor[i].ds_columnName, dataValues[i].dv_null ? "NULL" : *((int *) getColParm.gc_columnData[i].dv_value));
        }
    } while(TRUE);

    memset(&closeParm, 0, sizeof(closeParm));
    closeParm.cl_stmtHandle = *stmtHandle;
    IIapi_close(&closeParm);

    while(closeParm.cl_genParm.gp_completed == FALSE)
        IIapi_wait(&waitParm);

    if(closeParm.cl_genParm.gp_status != IIAPI_ST_SUCCESS)
    {
        print_error(&closeParm.cl_genParm);
        return;
    } 
    memset(&commitParm, 0, sizeof(commitParm));
    commitParm.cm_tranHandle = *tranHandle;

    IIapi_commit(&commitParm);
    while(commitParm.cm_genParm.gp_completed == FALSE)
        IIapi_wait(&waitParm);

    if(commitParm.cm_genParm.gp_status != IIAPI_ST_SUCCESS)
    {
        print_error(&commitParm.cm_genParm);
        return;
    }
}


int main(int argc, const char *argv[])
{
    IIAPI_INITPARM  initParm;
    II_PTR connHandle, envHandle;
    IIAPI_CONNPARM connParm;
    IIAPI_WAITPARM waitParm = { -1 }; 
    II_PTR stmtHandle;
    II_PTR tranHandle;
    initParm.in_version = IIAPI_VERSION_7;
    initParm.in_timeout = -1;
    IIapi_initialize(&initParm);
    const char *wkt;
    envHandle = initParm.in_envHandle;
    char query[32000];
    int i;
    int repeats;

    if(argc <= 1)
    {
        printf("Usage: %s <repeats>\n", argv[0]);
        return 0;
    }
    else
    {
        repeats = atoi(argv[1]);
    }

    //First load the data
    load_shapefile("province.shp");
    
    //Next run the ingres stuff
    memset(&connParm, 0, sizeof(IIAPI_CONNPARM));
    connParm.co_type = IIAPI_CT_SQL;
    connParm.co_target = "geo";
    connParm.co_connHandle = envHandle;
    connParm.co_timeout = -1;

    IIapi_connect(&connParm);

    while(connParm.co_genParm.gp_completed == FALSE)
        IIapi_wait(&waitParm);

    if(connParm.co_genParm.gp_errorHandle)
    {
        // something went wrong
        // let's retrieve the message
        print_error(&connParm.co_genParm);
        return 1;
    }

    connHandle = connParm.co_connHandle;

    wkt = load_test_wkt("polygon.wkt");

    int j;

    for(j = 0; j < repeats; j++)
    {
        printf("Running test #%d\n", j+1);
        //some functions tests
        for(i = 0; functions[i] != NULL; i++)
        {
            sprintf(query, "SELECT ST_%s(shape, polyfromtext('%s')) FROM province", functions[i], wkt);
            execute_query(query, &connHandle, &stmtHandle, &tranHandle);
        }

        execute_query("SELECT ST_Boundary(shape) FROM province", &connHandle, &stmtHandle, &tranHandle);
        execute_query("SELECT ST_Area(shape) FROM province", &connHandle, &stmtHandle, &tranHandle);
        execute_query("SELECT ST_Centroid(shape) FROM province", &connHandle, &stmtHandle, &tranHandle);
    }
    {
        //Finish up
        IIAPI_DISCONNPARM disconnParm;
        IIAPI_TERMPARM termParm;

        memset(&disconnParm, 0, sizeof(disconnParm));
        disconnParm.dc_connHandle = connHandle;
        IIapi_disconnect(&disconnParm);

        while(disconnParm.dc_genParm.gp_completed == FALSE)
            IIapi_wait(&waitParm);

        if(disconnParm.dc_genParm.gp_status != IIAPI_ST_SUCCESS)
        {
            print_error(&disconnParm.dc_genParm);
            return 1;
        }

        memset(&termParm, 0, sizeof(termParm));

        IIapi_terminate(&termParm);
    }
}

