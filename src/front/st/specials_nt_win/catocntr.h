/********************************************************************
//
//  Copyright (C) 1995 Ingres Corporation
//
//    Project  : CA/OpenIngres Visual DBA
//
//    Source   : catocntr.h
//
//     
//
********************************************************************/
#ifdef _WIN32
#define CONTAINER_CLASS      "CA_Container32"
#else
#define CONTAINER_CLASS      "CA_Container"
#endif
#define CTS_READONLY         0x00000100   // container is read-only
#define CTS_SINGLESEL        0x00000200   // container has single record selection
#define CTS_MULTIPLESEL      0x00000400   // container has multiple record selection
#define CTS_EXTENDEDSEL      0x00000800   // container has extended record selection
#define CTS_BLOCKSEL         0x00001000   // container has block field selection
#define CTS_SPLITBAR         0x00002000   // container can have a split bar
#define CTS_VERTSCROLL       0x00004000   // container has vert scroll bar
#define CTS_HORZSCROLL       0x00008000   // container has horz scroll bar
#define CTS_INTEGRALWIDTH    0x00000001   // no unused bk area to the right of last field
#define CTS_INTEGRALHEIGHT   0x00000002   // no partial records at bottom of display area
#define CTS_EXPANDLASTFLD    0x00000004   // expand last field if container increases in width
#define CTS_ASYNCNOTIFY      0x00000080   // use asynchronous notification method

