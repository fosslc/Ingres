/*
** Copyright (c) 1987, 2004 Ingres Corporation All Rights Reserved.
*/

/*
** Name: gcaclass.h
**
** Description:
**	Provides a registry for used and potential server
**	class identifiers.  These identifiers are neither
**	required nor restricted by GCA.  This file merely
**	provides a common point of definition for ease of
**	use and to avoid conflicts.
**
**	A small number of server classes are known to the
**	Name Server (see gcn.h).
**
** History:
**	 5-Oct-01 (gordy)
**	    Extracted from gca.h
**	15-jun-2004 (somsa01)
**	    Cleaned up code for Open Source.
*/


# define	GCA_ALB_CLASS		"ALB"	    /* HP Allbase server */
# define	GCA_COMSVR_CLASS	"COMSVR"    /* Communications server */
# define	GCA_BRIDGESVR_CLASS	"BRIDGE"    /* Bridge server */
# define	GCA_DBASE4_CLASS    	"DBASE4"    /* DBASE4 server */
# define	GCA_DB2_CLASS		"DB2"	    /* DB2 server */
# define	GCA_DG_CLASS		"DG"	    /* DG server */
# define	GCA_ICE_CLASS		"ICESVR"    /* Ingres ICE */
# define	GCA_IDMR_CLASS		"IDMSR"	    /* IDMSR server */
# define	GCA_IDMX_CLASS		"IDMSX"	    /* IDMSX server */
# define	GCA_IMS_CLASS		"IMS"	    /* IMS server */
# define	GCA_INFORMIX_CLASS	"INFORMIX"  /* INFORMIX server */
# define	GCA_INGRES_CLASS	"INGRES"    /* INGRES DBMS server */
# define	GCA_IUSVR_CLASS		"IUSVR"	    /* INGRES utility server */
# define	GCA_JDBC_CLASS		"JDBC"      /* JDBC server */
# define	GCA_IINMSVR_CLASS	"IINMSVR"   /* Local Name server */
# define	GCA_NMSVR_CLASS		"NMSVR"	    /* Name servers */
# define	GCA_ODB2_CLASS		"ODB2"      /* ODB2 server */
# define	GCA_ORACLE_CLASS	"ORACLE"    /* ORACLE server */
# define	GCA_OPINGDT_CLASS	"OPINGDT"   /* OpenIngres Desktop */
# define	GCA_RDB_CLASS		"RDB"	    /* RDB server */
# define	GCA_RMS_CLASS		"RMS"	    /* RMS server */
# define	GCA_SQLDS_CLASS		"SQLDS"	    /* SQLDS server */
# define	GCA_STAR_CLASS		"STAR"	    /* STAR server */
# define	GCA_SYBASE_CLASS	"SYBASE"    /* SYBASE server */
# define	GCA_TERADATA_CLASS	"TERADATA"  /* TERADATA server */
# define	GCA_VSAM_CLASS		"VSAM"	    /* VSAM server */

