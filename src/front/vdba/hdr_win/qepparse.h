/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.
*/

/*
**    Source   : qeparse.h : header file
**    Project  : Web DBA 
**    Author   : Robins Lazer (lazro01)
**    Purpose  : Defining the structures required for WEB DBA project.
**
** History:
**
** 24-02-2004 (lazro01) (SIR 112040)
**    Created.
** 29-Jun-2004 (lazro01) (SIR 112040)
**	  Replaced BOOL with bool.
*/


#ifdef __cplusplus 
extern "C"
{
#endif


/* Structure for storing a list of strings. */
typedef struct CaQepStringList
{
	char                   *m_qepstring;
	struct CaQepStringList *m_nextptr;
}CaQepStringList;

/* Structure for storing the QEP node information. */
typedef struct NewCaQepNodeInformation{
	unsigned int m_nIcon;                        /* ID of icon for Qep box */
	char         m_strNodeType[200];             /* Type of Qep's node.  Ex: "Proj-rest". */
	char         m_strNodeHeader[200];           /* Description of node: Ex: "Sort on (col1)" */
	int          m_nPage;                        /* Total number of pages */
	int          m_nTuple;                       /* Total number of Tuples. */
	int          m_nDiskCost;
	int          m_nCPUCost;
	int          m_nNetCost;
	int          m_iPosX;                        /* "logical" horizontal position ( 2 is the minimum between 2 nodes on the same "line" */
	int          m_qepNode;                      
	char         m_strDatabase[200];             /* Database for Star. Ex: "1->2 FRNASU02::XXXX" */
	char         m_strNode[200];                 /* Node. Ex: "FRNASU02" */
}NewCaQepNodeInformation;

/*
** Structure containing information for each node in a QEP Binary tree.  
** It contains links to its children. 
*/
typedef struct NewCaSqlQEPBinaryTree
{
	int                           m_nID;            /* Unique ID to identify the node. */
	int                           m_nSon;           /* 0: Root, 1: Left, 2: Right */
	int                           m_nParentID;      /* If m_nSon = 0, it is not used. */
	NewCaQepNodeInformation       *m_pQepNodeInfo;  /* More information about node. */	                               
	struct NewCaSqlQEPBinaryTree  *m_pQepNodeLeft;  
	struct NewCaSqlQEPBinaryTree  *m_pQepNodeRight;
}NewCaSqlQEPBinaryTree;

/* 
** The following structure contains information about node and site string related to 
** star database.
*/
typedef struct QepNodeList
{
	int                m_nNodeNo;
	char               *m_qepstring;
	struct QepNodeList *NextPtr;
}QepNodeList;

/* 
** The following structure contains information for a QEP binary tree. 
*/
typedef struct NewCaSqlQEPData
{		
	int                    m_qepType;                 /* Indicate the type of Qep (Normal, Star)*/
	char                   m_strHeader[200];          /* Header of Qep. Ex: Query Plan ... Main Query. */
	char                   m_strGenerateTable[200];   /* Header. Name of table. */
	bool                   m_bTimeOut;                /* Header. */
	bool                   m_bLargeTemporaries;       /* Header. */
	bool                   m_bFloatIntegerException;  /* Header. */
	CaQepStringList        *m_strlistAggregate;       /* Header. */
	CaQepStringList        *m_strlistExpression;      /* Header. */
	NewCaSqlQEPBinaryTree  *m_pQepBinaryTree;         /* Qep's Tree. */
	QepNodeList            *NodeList;                 /* Node information, required for star database */
	struct NewCaSqlQEPData *NextPtr;                  /* Pointer to the next QEP tree. Some sqls may generate more than one Binary tree. */
}NewCaSqlQEPData;

  
extern bool LIBBK_QEPParse(char *, bool, NewCaSqlQEPData **, bool (*ExecSQL)(char *, int ),
		void (*PrepareQEP)(char * statement));

extern bool LIBBK_FreeQEPParse (NewCaSqlQEPData *pQEPData);

#ifdef __cplusplus 
}
#endif  



