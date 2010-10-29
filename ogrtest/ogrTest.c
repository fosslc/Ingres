#include <stdio.h>
#include <stdlib.h>
#include <ogr_api.h>

#define MAXWKTLEN 1047551

int cycleLayer(OGRLayerH layer)
{
    OGRFeatureH currentFeature = NULL;
//    char *featWKT = NULL;
    OGRGeometryH testGeo, currGeo, centGeo;
    static char wkt[MAXWKTLEN];
    FILE *input;
    char inputfile[15] = "polygon.wkt";
    size_t size;
    char *wktPtr;

    input = fopen(inputfile, "r");
    if ( ! input ) { perror("fopen"); exit(1); }

    size = fread(wkt, 1, MAXWKTLEN-1, input);
    fclose(input);
    if ( ! size ) { perror("fread"); exit(1); }
    if ( size == MAXWKTLEN-1 ) { perror("WKT input too big!"); exit(1); }
    wkt[size] = '\0'; /* ensure it is null terminated */
    wktPtr = (char *)&wkt;

    OGR_G_CreateFromWkt(&wktPtr, OGR_L_GetSpatialRef(layer), &testGeo);
/*
    OGR_G_ExportToWkt(testGeo, &featWKT);
    printf("Test wkt %s\n", featWKT);
    if(featWKT != NULL)
    {
        free(featWKT);
        featWKT = NULL;
    }
*/
    OGR_L_ResetReading(layer);
    while((currentFeature = OGR_L_GetNextFeature(layer)) != NULL)
    {
        currGeo = OGR_F_GetGeometryRef(currentFeature);
/*
        printf("Feature wkt %s\n", featWKT);
        if(featWKT != NULL)
        {
            free(featWKT);
            featWKT = NULL;
        }
*/
        OGR_G_Intersects(currGeo, testGeo);
        OGR_G_Equals(currGeo, testGeo);
        OGR_G_Disjoint(currGeo, testGeo);
        OGR_G_Touches(currGeo, testGeo);
        OGR_G_Crosses(currGeo, testGeo);
        OGR_G_Within(currGeo, testGeo);
        OGR_G_Contains(currGeo, testGeo);
        OGR_G_Overlaps(currGeo, testGeo);

        OGR_G_Intersection(currGeo, testGeo);
        OGR_G_Union(currGeo, testGeo);
        OGR_G_Difference(currGeo, testGeo);

        OGR_G_Distance(currGeo, testGeo);

        OGR_G_GetBoundary(currGeo);
        OGR_G_GetArea(currGeo);

        centGeo = OGR_G_CreateGeometry(wkbPoint);
        OGR_G_Centroid(currGeo, centGeo);
        OGR_G_DestroyGeometry(centGeo);
        
        OGR_F_Destroy(currentFeature);
        currentFeature = NULL;
    }

    OGR_G_DestroyGeometry(testGeo);
    return 0;
}

int main()
{
    OGRDataSourceH data_source = NULL;
    OGRSFDriverH read_driver = NULL;
    OGRLayerH layer = NULL;
    OGRSFDriverH write_driver = NULL;
    OGRDataSourceH write_source = NULL;
    OGRRegisterAll(); //Register all drivers
    read_driver = OGRGetDriverByName("ESRI Shapefile");
    write_driver = OGRGetDriverByName("Memory");
    data_source = OGR_Dr_Open(read_driver, "province.shp", FALSE);
    layer = OGR_DS_GetLayer(data_source, 0);
    write_source = OGR_Dr_CopyDataSource(write_driver, data_source, "InMemorium", NULL);
/*
    printf("Layer count source: %d\n", OGR_DS_GetLayerCount(data_source));
    printf("Feature count source: %d\n", OGR_L_GetFeatureCount(layer, TRUE));
    printf("Layer count destination: %d\n", OGR_DS_GetLayerCount(write_source));
    printf("Feature count destination: %d\n", OGR_L_GetFeatureCount(OGR_DS_GetLayer(write_source, 0), TRUE));
*/
    cycleLayer(layer);

    OGR_DS_Destroy(write_source);

    return 0;
}
