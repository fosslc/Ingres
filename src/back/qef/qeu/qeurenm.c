/* Copyright (c) 1995, 2005 Ingres Corporation
**
**
*/

#include    <compat.h>
#include    <gl.h>
#include    <me.h>
#include    <pc.h>
#include    <st.h>
#include    <iicommon.h>
#include    <usererror.h>
#include    <dbdbms.h>
#include    <dudbms.h>
#include    <ddb.h>
#include    <cui.h>
#include    <bt.h>
#include    <tm.h>
#include    <er.h>
#include    <cs.h>
#include    <lk.h>
#include    <scf.h>
#include    <sxf.h>
#include    <ulb.h>
#include    <ulf.h>
#include    <ulm.h>
#include    <adf.h>
#include    <cm.h>
#include    <qsf.h>
#include    <dmf.h>
#include    <dmacb.h>
#include    <dmucb.h>
#include    <dmtcb.h>
#include    <dmrcb.h>
#include    <qefmain.h>
#include    <qefrcb.h>
#include    <qefcb.h>
#include    <qefscb.h>
#include    <qefqeu.h>
#include    <qeuqcb.h>
#include    <psfindep.h>
#include    <qeugrant.h>
#include    <qeurevoke.h>
#include    <ade.h>
#include    <dmmcb.h>
#include    <ex.h>
#include    <qefnode.h>
#include    <psfparse.h>
#include    <qefact.h>
#include    <qefqp.h>
#include    <qefdsh.h>
#include    <rqf.h>
#include    <tpf.h>
#include    <qefkon.h>
#include    <qefqry.h>
#include    <qefcat.h>
#include    <qefprotos.h>
#include    <rdf.h>


/* Name: qeu_renameValidate - Validate the dependent procedures, views, 
**			      rules on the table /columns  being renamed and 
**			      make sure we don't rename a table which has 
**			      external procedures rules etc on it. 
**
** History:
**
** 31-March-2010 (gupsh01)
**	Created.
** 08-Oct-2010 (gupsh01) bug 124572
**	Added support for scanning the iipriv catalog to rule out any
**	dependent views and procedures on the table/column being
**	renamed.
**	2-Dec-2010 (kschendel) SIR 124685
**	    Warning, prototype fixes.
*/
DB_STATUS
qeu_renameValidate(
QEF_CB		*qef_cb,
QEUQ_CB		*qeuq_cb,
ULM_RCB         *ulm,
DB_ERROR        *error_block,
DMT_TBL_ENTRY	*table_entry,
bool		isColumnRename,
DMF_ATTR_ENTRY  **dmf_attr)
{
   QEU_CB          tranqeu;
   bool		   transtarted;
   DB_STATUS	   status = E_DB_ERROR;
   DB_STATUS	   local_status;
   i4		   error;
   bool		   exit_loop, exit_loop2;
   DB_TAB_ID	   *dbtable;

   /* Variables used for iidbdepends */
   QEU_CB          vqeu;
   bool            view_opened = FALSE;
   DMR_ATTR_ENTRY  vkey_array[4];
   DMR_ATTR_ENTRY  *vkey_ptr_array[4];
   QEF_DATA        vqef_data;
   i4		   obj_type = DB_TABLE; /* To set the key value for iidbdepends request */
   i4		   tree_mode = DB_VIEW;    /* To set the key value for iitree request */ 
   DB_IIDBDEPENDS  *dtuple;
   bool		   found_view = FALSE;

   /* Variables used for iipriv */
   QEU_CB          prqeu;
   bool            priv_opened = FALSE;
   DMR_ATTR_ENTRY  prkey_array[4];
   DMR_ATTR_ENTRY  *prkey_ptr_array[4];
   QEF_DATA        prqef_data;
   DB_IIPRIV       *prtuple;

   /* Variables used for iixpriv */
   QEU_CB          xprqeu;
   bool            xpriv_opened = FALSE;
   DMR_ATTR_ENTRY  xprkey_array[4];
   DMR_ATTR_ENTRY  *xprkey_ptr_array[4];
   QEF_DATA        xprqef_data;
   DB_IIXPRIV      *xprtuple;

   /* Variables used for Procedures */
   QEU_CB          xdbpqeu;                /* for iixprocedure tuples */
   QEU_CB          dbpqeu;                 /* for iiprocedure tuples */
   bool            dbp_opened = FALSE;
   bool            xdbp_opened = FALSE;
   QEF_DATA        dbpqef_data;
   QEF_DATA        xdbpqef_data;
   DMR_ATTR_ENTRY  xdbpkey_array[2];
   DMR_ATTR_ENTRY *xdbpkey_ptr_array[2];
   DB_PROCEDURE   *dbptuple;
   DB_IIXPROCEDURE *xdbptuple;
   bool		   found_procedure = FALSE;

   /* Variables used for protections */
   QEU_CB	    pqeu;                 /* for iiprotect tuples */
   QEF_DATA         pqef_data;
   DB_PROTECTION   *ptuple;
   bool		    found_non_grant_prot = FALSE;
   bool             prot_opened = FALSE;
   DMR_ATTR_ENTRY   pkey_array[2];
   DMR_ATTR_ENTRY  *pkey_ptr_array[2];

   /* Variables for integrity */
   QEU_CB	    iqeu;                 /* for integrity tuples */
   QEF_DATA         iqef_data;
   DB_INTEGRITY    *ituple;
   bool		    found_cons_or_integ = FALSE;
   bool             integ_opened  = FALSE;
   DMR_ATTR_ENTRY   ikey_array[2];
   DMR_ATTR_ENTRY  *ikey_ptr_array[2];

   /* Variables for rules */
   QEU_CB	    rulqeu;                 /* for rules tuples */
   QEF_DATA         rulqef_data;
   DB_IIRULE	    *rultuple;
   bool		    found_rule = FALSE;
   bool             rul_opened = FALSE;
   DMR_ATTR_ENTRY   rulkey_array[2];
   DMR_ATTR_ENTRY  *rulkey_ptr_array[2];

   /* Variables for seculrity alarm*/
   QEU_CB	    aqeu;                 /* for security alarm tuples */
   QEF_DATA         aqef_data;
   DB_SECALARM	    *atuple;
   bool		    found_alarm = FALSE;
   bool             alarm_opened  = FALSE;
   DMR_ATTR_ENTRY   akey_array[3];
   DMR_ATTR_ENTRY  *akey_ptr_array[3];

   /* For column rename need to get the tree tuples in certain cases */
   QEU_CB	    tqeu;                 /* for tree tuples */
   DB_SECALARM	    *ttuple;
   bool             tree_opened = FALSE;
   DMR_ATTR_ENTRY   tkey_array[3];
   DMR_ATTR_ENTRY  *tkey_ptr_array[3];

   if (table_entry)
      dbtable = &table_entry->tbl_id;
   else
      return (E_DB_ERROR);

   for (exit_loop = FALSE; !exit_loop; )            /* to break out of */
   {
      /* Basic sanity checking to ensure a good input state */
      if( table_entry->tbl_status_mask & (DMT_CATALOG | DMT_EXTENDED_CAT ))
      {
	error = E_QE0173_RENAME_TAB_IS_CATALOG; /* Table is a core or extended catalog */
	break;
      }
      else if ( table_entry->tbl_status_mask & DMT_SYS_MAINTAINED || 
		table_entry->tbl_2_status_mask & DMT_SYSTEM_GENERATED )
      {
	/* table is either system generated or system maintained */
	error = E_QE0174_RENAME_TAB_IS_SYSGEN; 
	break;
      }	 
      else if ( table_entry->tbl_2_status_mask & DMT_TEXTENSION)
	{
	error = E_QE0175_RENAME_TAB_IS_ETAB; /* table is a blob etab table */
	break;
      }	
      else if ( table_entry->tbl_status_mask & DMT_GATEWAY)  /* table is a gateway table */	
      {
	error = E_QE0176_RENAME_TAB_IS_GATEWAY;
	break;
      }
      else if ( table_entry->tbl_2_status_mask & DMT_PARTITION )/* This is a partition */
      {
	error = E_QE0177_RENAME_TAB_IS_PARTITION;
	break;
      }
      else if ( table_entry->tbl_2_status_mask & DMT_INCONSIST )/* table is inconsistent */
      {
	error = E_QE0178_RENAME_TAB_IS_INCONSIST;
	break;
      }
      else if ( table_entry->tbl_2_status_mask & DMT_READONLY)/* table is a readonly table */
      {
	error = E_QE0179_RENAME_TAB_IS_READONLY;
	break;
      }

      for (exit_loop2 = FALSE; !exit_loop2; )            /* to break out of */
      {
        ulm->ulm_psize = sizeof(DB_IIDBDEPENDS);
        if ((status = qec_malloc(ulm)) != E_DB_OK)
        {
          ulm_closestream(ulm);
          error = E_QE001E_NO_MEM;
          break;
        }
        dtuple = (DB_IIDBDEPENDS*) ulm->ulm_pptr;

        ulm->ulm_psize = sizeof(DB_IIXPRIV);
        if ((status = qec_malloc(ulm)) != E_DB_OK)
        {
          ulm_closestream(ulm);
          error = E_QE001E_NO_MEM;
          break;
        }
        xprtuple = (DB_IIXPRIV *) ulm->ulm_pptr;

        ulm->ulm_psize = sizeof(DB_IIPRIV);
        if ((status = qec_malloc(ulm)) != E_DB_OK)
        {
          ulm_closestream(ulm);
          error = E_QE001E_NO_MEM;
          break;
        }
        prtuple = (DB_IIPRIV *) ulm->ulm_pptr;

        ulm->ulm_psize = sizeof(DB_IIXPROCEDURE);
        if ((status = qec_malloc(ulm)) != E_DB_OK)
        {
            ulm_closestream(ulm);
            break;
        }
        xdbptuple = (DB_IIXPROCEDURE *) ulm->ulm_pptr;

        ulm->ulm_psize = sizeof(DB_PROCEDURE);
        if ((status = qec_malloc(ulm)) != E_DB_OK)
        {
                error = E_QE001E_NO_MEM;
                ulm_closestream(ulm);
                break;
        }
        dbptuple = (DB_PROCEDURE *) ulm->ulm_pptr;

        ulm->ulm_psize = sizeof(DB_PROTECTION);
        if ((status = qec_malloc(ulm)) != E_DB_OK)
        {
            ulm_closestream(ulm);
            break;
        }
        ptuple = (DB_PROTECTION *) ulm->ulm_pptr;

 	ulm->ulm_psize = sizeof(DB_INTEGRITY);
        if ((status = qec_mopen(ulm)) != E_DB_OK)
        {
            error = E_QE001E_NO_MEM;
            break;
        }
        ituple = (DB_INTEGRITY*) ulm->ulm_pptr;

        ulm->ulm_psize = sizeof(DB_IIRULE);
        if ((status = qec_malloc(ulm)) != E_DB_OK)
        {
            ulm_closestream(ulm);
            error = E_QE001E_NO_MEM;
            break;
        }
        rultuple = (DB_IIRULE *) ulm->ulm_pptr;

        ulm->ulm_psize = sizeof(DB_SECALARM);
        if ((status = qec_malloc(ulm)) != E_DB_OK)
        {
            ulm_closestream(ulm);
            error = E_QE001E_NO_MEM;
            break;
        }
        atuple = (DB_SECALARM *) ulm->ulm_pptr;

        ulm->ulm_psize = sizeof(DB_IITREE);
        if ((status = qec_malloc(ulm)) != E_DB_OK)
        {
            ulm_closestream(ulm);
            error = E_QE001E_NO_MEM;
            break;
        }
        ttuple = (DB_SECALARM *) ulm->ulm_pptr;

        exit_loop2  = TRUE;
     }
     if (status != E_DB_OK)
     {
	error = E_QE001E_NO_MEM;
	break;
     }

     /*
     ** Initialize session control block to point
     ** to active control block.
     */

     if (qef_cb->qef_stat == QEF_NOTRAN)
     {
       tranqeu.qeu_type = QEUCB_CB;
       tranqeu.qeu_length = sizeof(QEUCB_CB);
       tranqeu.qeu_db_id = qeuq_cb->qeuq_db_id;
       tranqeu.qeu_d_id = qeuq_cb->qeuq_d_id;
       tranqeu.qeu_flag = 0;
       tranqeu.qeu_mask = 0;
       status = qeu_btran(qef_cb, &tranqeu);
       if (status != E_DB_OK)
       {
           error = tranqeu.error.err_code;
           break;
       }
       transtarted = TRUE;
     }
     /* escalate the transaction to MST */
     if (qef_cb->qef_auto == QEF_OFF)
       qef_cb->qef_stat = QEF_MSTRAN;

      /* Set up a the QEUCB_CB control block for 
      ** retrieving iidbdepends tuples 
      */
      vqeu.qeu_type = QEUCB_CB;
      vqeu.qeu_length = sizeof(QEUCB_CB);
      vqeu.qeu_db_id = qeuq_cb->qeuq_db_id;
      vqeu.qeu_flag = DMT_U_DIRECT;
      vqeu.qeu_mask = 0;
      vqeu.qeu_qual = 0;
      vqeu.qeu_qarg = 0;
      vqeu.qeu_f_qual = 0;
      vqeu.qeu_f_qarg = 0;
      vqeu.qeu_lk_mode = DMT_IS;
      vqeu.qeu_access_mode = DMT_A_READ;

      /* Open iidbdepends table */
      vqeu.qeu_tab_id.db_tab_base = DM_B_DEPENDS_TAB_ID;
      vqeu.qeu_tab_id.db_tab_index = DM_I_DEPENDS_TAB_ID;
      status = qeu_open(qef_cb, &vqeu);
      if (status != E_DB_OK)
      {
         error = vqeu.error.err_code;
         return (E_DB_ERROR);
      }
      view_opened = TRUE;

     /* Prepare to get tuples from iidbdepends */
     vqeu.qeu_key = vkey_ptr_array;

     /* Initialize key array for dbdepends access */
     vkey_ptr_array[0] = &vkey_array[0];
     vkey_ptr_array[1] = &vkey_array[1];
     vkey_ptr_array[2] = &vkey_array[2];

     vkey_ptr_array[0]->attr_number = DM_1_DEPENDS_KEY;
     vkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
     vkey_ptr_array[0]->attr_value = (char *) &dbtable->db_tab_base;

     vkey_ptr_array[1]->attr_number = DM_2_DEPENDS_KEY;
     vkey_ptr_array[1]->attr_operator = DMR_OP_EQ;
     vkey_ptr_array[1]->attr_value = (char *) &dbtable->db_tab_index;

     vkey_ptr_array[2]->attr_number = DM_3_DEPENDS_KEY;
     vkey_ptr_array[2]->attr_operator = DMR_OP_EQ;
     vkey_ptr_array[2]->attr_value = (char *) &obj_type;

     vqeu.qeu_count = 1;
     vqeu.qeu_tup_length = sizeof(DB_IIDBDEPENDS);
     vqeu.qeu_output = &vqef_data;
     vqef_data.dt_next = 0;
     vqef_data.dt_size = sizeof(DB_IIDBDEPENDS);
     vqef_data.dt_data = (PTR) dtuple;

     vqeu.qeu_getnext = QEU_REPO;
     vqeu.qeu_klen = 3;

     if (isColumnRename)
     {
        STRUCT_ASSIGN_MACRO(vqeu, tqeu);

	 tqeu.qeu_tab_id.db_tab_base  = DM_B_TREE_TAB_ID;
        tqeu.qeu_tab_id.db_tab_index  = DM_I_TREE_TAB_ID;
        status = qeu_open(qef_cb, &tqeu);
        if (status != E_DB_OK)
        {
            error = tqeu.error.err_code;
            break;
        }
        tree_opened = TRUE;

        tqeu.qeu_count = 0;
        tqeu.qeu_tup_length = sizeof(DB_IITREE);
        tqeu.qeu_output = (QEF_DATA *) NULL;
        tqeu.qeu_getnext = QEU_REPO;
        tqeu.qeu_klen = 3;
        tqeu.qeu_key = tkey_ptr_array;

        tkey_ptr_array[0] = &tkey_array[0];
        tkey_ptr_array[0]->attr_number = DM_1_TREE_KEY;
        tkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
        tkey_ptr_array[0]->attr_value = (char *) &dbtable->db_tab_base;

        tkey_ptr_array[1] = &tkey_array[1];
        tkey_ptr_array[1]->attr_number = DM_2_TREE_KEY;
        tkey_ptr_array[1]->attr_operator = DMR_OP_EQ;
        tkey_ptr_array[1]->attr_value = (char *) &dbtable->db_tab_index;

        tkey_ptr_array[2] = &tkey_array[2];
        tkey_ptr_array[2]->attr_number = DM_3_TREE_KEY;
        tkey_ptr_array[2]->attr_operator = DMR_OP_EQ;
        tkey_ptr_array[2]->attr_value = (char *) &tree_mode;
     }

     /* prepare to open iiprocedure and iixprocedure */
     {
	  STRUCT_ASSIGN_MACRO(vqeu, xdbpqeu);

	  /* Open iixdbprocedure table */
	  xdbpqeu.qeu_tab_id.db_tab_base  = DM_B_XDBP_TAB_ID;
	  xdbpqeu.qeu_tab_id.db_tab_index  = DM_I_XDBP_TAB_ID;
	  status = qeu_open(qef_cb, &xdbpqeu);
	  if (status != E_DB_OK)
	  {
            error = xdbpqeu.error.err_code;
	    break;
	  }
	  xdbp_opened = TRUE;

           /* open IIPROCEDURE */
            STRUCT_ASSIGN_MACRO(vqeu, dbpqeu);
            dbpqeu.qeu_tab_id.db_tab_base  = DM_B_DBP_TAB_ID;
            dbpqeu.qeu_tab_id.db_tab_index  = DM_I_DBP_TAB_ID;
            status = qeu_open(qef_cb, &dbpqeu);
            if (status != E_DB_OK)
            {
              error = dbpqeu.error.err_code;
              break;
            }
            dbp_opened = TRUE;

            dbpqeu.qeu_count = 1;
            dbpqeu.qeu_tup_length = sizeof(DB_PROCEDURE);
            dbpqeu.qeu_output = dbpqeu.qeu_input = &dbpqef_data;
            dbpqef_data.dt_next = 0;
            dbpqef_data.dt_size = sizeof(DB_PROCEDURE);
            dbpqef_data.dt_data = (PTR) dbptuple;
     }

     /* Get all tuples from iidbdepends for this table */
     for (exit_loop2 = FALSE; !exit_loop2; )            /* to break out of */
     {
        status = qeu_get(qef_cb, &vqeu);
        if (status != E_DB_OK)
        {
	    if (vqeu.error.err_code == E_QE0015_NO_MORE_ROWS) 
	    {
              status = E_DB_OK;
              break;
	    }
            error = vqeu.error.err_code;
            exit_loop2 = TRUE; 
            break;
        }

        /* Don't reposition, continue scan. */
        vqeu.qeu_getnext = QEU_NOREPO;

        /* Make sure the independent tuple is matching the table being
	** renamed.
	*/
	if ((dtuple->dbv_indep.db_tab_base != dbtable->db_tab_base) || 
	    (dtuple->dbv_indep.db_tab_index != dbtable->db_tab_index))
	    continue;

        /* Check the dependency based on type */
        if (dtuple->dbv_dtype == DB_VIEW)
	{
            DMT_SHW_CB                  dmt_show;
	    DMT_TBL_ENTRY               dmt_tbl_entry;

	    /*
            ** initialize the control block that will be used to look up information
            ** on views
            */
            dmt_show.type                           = DMT_SH_CB;
            dmt_show.length                         = sizeof(DMT_SHW_CB);
            dmt_show.dmt_char_array.data_in_size    = 0;
            dmt_show.dmt_char_array.data_out_size   = 0;
            dmt_show.dmt_flags_mask                 = DMT_M_TABLE;
            dmt_show.dmt_db_id                      = qeuq_cb->qeuq_db_id;
            dmt_show.dmt_session_id                 = qeuq_cb->qeuq_d_id;
            dmt_show.dmt_table.data_address         = (PTR) &dmt_tbl_entry;
            dmt_show.dmt_table.data_in_size         = sizeof(DMT_TBL_ENTRY);
            dmt_show.dmt_char_array.data_address    = (PTR) NULL;
            dmt_show.dmt_char_array.data_in_size    = 0;
            dmt_show.dmt_char_array.data_out_size   = 0;

	    STRUCT_ASSIGN_MACRO(dtuple->dbv_dep, dmt_show.dmt_tab_id);

            local_status = dmf_call(DMT_SHOW, &dmt_show);
            if (   local_status != E_DB_OK
                    && dmt_show.error.err_code != E_DM0114_FILE_NOT_FOUND)
            {
                    error = dmt_show.error.err_code;
                    (VOID) qef_error(error, 0L, local_status, &error,
                        &qeuq_cb->error, 0);
                    if (local_status > status)
                        status = local_status;

		    break;
            }

	    if (isColumnRename)
	    {
		/* We must check the view tree to find out if the view tree 
		** has the columns that is being renamed. In other words if 
		** this view is dependent on the column being renamed.
		*/
	        bool		isdependent;

        	tree_mode = DB_VIEW;
        	tkey_ptr_array[2]->attr_value = (char *) &tree_mode;
		
		status = qeu_v_finddependency(ulm, qef_cb, &tqeu,
                                                   &dtuple->dbv_dep,
                                                   dbtable,
                                                   (DB_TREE_ID *) NULL,
                                                   (i2) 0,
                                                   (DB_NAME *) NULL,
                                                   (DB_OWN_NAME *) NULL,
                                                   (DB_QMODE)dtuple->dbv_dtype,
						   qeuq_cb->qeuq_ano,
                                                   FALSE,
                                                   &isdependent,
                                                   error_block);
                if (status != E_DB_OK)
                {
		    /* If the column is found error may be reported */
		    if (error_block->err_code != E_QE016B_COLUMN_HAS_OBJECTS)
		    {
                       error = error_block->err_code;
                       break;
		    }
                }

		if (isdependent)
		{
	    	    found_view = TRUE;
	            qef_error( E_QE016E_RENAME_TAB_HAS_VIEW,
                           0L, status, &error, error_block, 4,
                           qec_trimwhite(DB_TAB_MAXNAME,
                                (char *)&table_entry->tbl_name.db_tab_name),
                           (char *)&table_entry->tbl_name.db_tab_name,
                           qec_trimwhite(DB_OWN_MAXNAME, 
				(char *)&table_entry->tbl_owner.db_own_name),
                           (char *)&table_entry->tbl_owner.db_own_name,
            		   qec_trimwhite(sizeof(dmt_tbl_entry.tbl_name),
                		(char *) &dmt_tbl_entry.tbl_name),
                	   (char *) &dmt_tbl_entry.tbl_name,
            		   qec_trimwhite(sizeof(dmt_tbl_entry.tbl_owner),
                		(char *) &dmt_tbl_entry.tbl_owner),
                	   (char *) &dmt_tbl_entry.tbl_owner
			);
		}
	    }
	    else
	    {
	        found_view = TRUE;
	        qef_error( E_QE016E_RENAME_TAB_HAS_VIEW,
                           0L, status, &error, error_block, 4,
                           qec_trimwhite(DB_TAB_MAXNAME,
                                (char *)&table_entry->tbl_name.db_tab_name),
                           (char *)&table_entry->tbl_name.db_tab_name,
                           qec_trimwhite(DB_OWN_MAXNAME, 
				(char *)&table_entry->tbl_owner.db_own_name),
                           (char *)&table_entry->tbl_owner.db_own_name,
            		   qec_trimwhite(sizeof(dmt_tbl_entry.tbl_name),
                		(char *) &dmt_tbl_entry.tbl_name),
                	   (char *) &dmt_tbl_entry.tbl_name,
            		   qec_trimwhite(sizeof(dmt_tbl_entry.tbl_owner),
                		(char *) &dmt_tbl_entry.tbl_owner),
                	   (char *) &dmt_tbl_entry.tbl_owner
			);
	    }
	}
        else if (dtuple->dbv_dtype == DB_DBP)
        {
          xdbpqeu.qeu_getnext = QEU_REPO;
          xdbpqeu.qeu_flag = 0;
          xdbpqeu.qeu_klen = 2;
 
          xdbpqeu.qeu_count = 1;
          xdbpqeu.qeu_tup_length = sizeof(DB_IIXPROCEDURE);
          xdbpqeu.qeu_output = &xdbpqef_data;
          xdbpqef_data.dt_next = 0;
          xdbpqef_data.dt_size = sizeof(DB_IIXPROCEDURE);
          xdbpqef_data.dt_data = (PTR) xdbptuple;

          xdbpqeu.qeu_key = xdbpkey_ptr_array;
  
          xdbpkey_ptr_array[0] = &xdbpkey_array[0];
          xdbpkey_ptr_array[0]->attr_number = DM_1_XDBP_KEY;
          xdbpkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
          xdbpkey_ptr_array[0]->attr_value =  (char *)&dtuple->dbv_dep.db_tab_base;

          xdbpkey_ptr_array[1] = &xdbpkey_array[1];
          xdbpkey_ptr_array[1]->attr_number = DM_2_XDBP_KEY;
          xdbpkey_ptr_array[1]->attr_operator = DMR_OP_EQ;
          xdbpkey_ptr_array[1]->attr_value = (char *)&dtuple->dbv_dep.db_tab_index;
	  /*
	  ** look up description of the dbproc (by doing a TID
	  ** join from IIXPROCEDURE); 
	  */
	  status = qeu_get(qef_cb, &xdbpqeu);
	  if (status != E_DB_OK)
	  {
	        error = xdbpqeu.error.err_code;
	        break;
	  }
	  /*
	  ** now do a TID-join into IIPROCEDURE
	  */

	  dbpqeu.qeu_flag = QEU_BY_TID;
	  dbpqeu.qeu_klen = 0;
	  dbpqeu.qeu_getnext = QEU_NOREPO;
	  dbpqeu.qeu_tid = xdbptuple->dbx_tidp;

	  status = qeu_get(qef_cb, &dbpqeu);
	  if (status != E_DB_OK)
	  {
	      error = dbpqeu.error.err_code;
	      if (dbpqeu.error.err_code == E_QE0015_NO_MORE_ROWS) 
	      {
                status = E_DB_OK;
                break;
	      }
              break;
	  }

   	    if (dbptuple->db_mask[0] & 
		(DBP_SYSTEM_GENERATED | DBP_CONS | DBP_NOT_DROPPABLE))
   	    {
	     /* The procedure is system generated or for a constraint
	     ** We report the constraints for which these are reported.
	     ** So skip them. 
	     */
	     continue;
   	    }  
   	    else
   	    {
	      found_procedure = TRUE;
	      qef_error( E_QE016D_RENAME_TAB_HAS_PROC,
                           0L, status, &error, error_block, 4,
                           qec_trimwhite(DB_TAB_MAXNAME,
                               (char *)&table_entry->tbl_name.db_tab_name),
                           &table_entry->tbl_name.db_tab_name,
                           qec_trimwhite(DB_OWN_MAXNAME,
                                   (char *)&table_entry->tbl_owner.db_own_name),
                           &table_entry->tbl_owner.db_own_name,
                           qec_trimwhite(DB_DBP_MAXNAME, (char *)&dbptuple->db_dbpname), 
			   &dbptuple->db_dbpname,
                           qec_trimwhite(DB_OWN_MAXNAME, (char *)&dbptuple->db_owner),
			   &dbptuple->db_owner);
   	    }
        } /* End of procedure handling */
     } // Loop through iidbdepends

     /* Views and procedures created by users with privileges
     ** Will be found in iipriv catalogs. Scan iipriv to report
     ** These 
     */
     
     /* prepare to open iixpriv index */
     xprqeu.qeu_key = xprkey_ptr_array;

     xprkey_ptr_array[0] = &xprkey_array[0];
     xprkey_ptr_array[0]->attr_number = DM_1_XPRIV_KEY;
     xprkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
     xprkey_ptr_array[0]->attr_value = (char *) &dbtable->db_tab_base;

     xprkey_ptr_array[1] = &xprkey_array[1];
     xprkey_ptr_array[1]->attr_number = DM_2_XPRIV_KEY;
     xprkey_ptr_array[1]->attr_operator = DMR_OP_EQ;
     xprkey_ptr_array[1]->attr_value = (char *) &dbtable->db_tab_index;

     /* prepare to open iipriv catalog */
     prqeu.qeu_key = prkey_ptr_array;

     prkey_ptr_array[0] = &prkey_array[0];
     prkey_ptr_array[0]->attr_number = DM_1_PRIV_KEY;
     prkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
     prkey_ptr_array[0]->attr_value = (char *) &dbtable->db_tab_base;

     prkey_ptr_array[1] = &prkey_array[1];
     prkey_ptr_array[1]->attr_number = DM_2_PRIV_KEY;
     prkey_ptr_array[1]->attr_operator = DMR_OP_EQ;
     prkey_ptr_array[1]->attr_value = (char *) &dbtable->db_tab_index;

     /* Open iipriv */
     STRUCT_ASSIGN_MACRO(vqeu, prqeu);
     prqeu.qeu_tab_id.db_tab_base  = DM_B_PRIV_TAB_ID;
     prqeu.qeu_tab_id.db_tab_index  = DM_I_PRIV_TAB_ID;
     status = qeu_open(qef_cb, &prqeu);
     if (status != E_DB_OK)
     {
	error = prqeu.error.err_code;
	break;
     }
     priv_opened = TRUE;

     /* Open iixpriv */
     STRUCT_ASSIGN_MACRO(vqeu, xprqeu);
     xprqeu.qeu_tab_id.db_tab_base  = DM_B_XPRIV_TAB_ID;
     xprqeu.qeu_tab_id.db_tab_index  = DM_I_XPRIV_TAB_ID;
     status = qeu_open(qef_cb, &xprqeu);
     if (status != E_DB_OK)
     {
	error = xprqeu.error.err_code;
	break;
     }
     xpriv_opened = TRUE;

     /* set up QEU_CB for reading iixpriv tuples */
     xprqeu.qeu_count = 1;
     xprqeu.qeu_tup_length = sizeof(DB_IIXPRIV);
     xprqeu.qeu_output = &xprqef_data;
     xprqef_data.dt_next = 0;
     xprqef_data.dt_size = sizeof(DB_IIXPRIV);
     xprqef_data.dt_data = (PTR) xprtuple;

     prqeu.qeu_count = 1;
     prqeu.qeu_tup_length = sizeof(DB_IIPRIV);
     prqeu.qeu_output = &prqef_data;
     prqef_data.dt_next = 0;
     prqef_data.dt_size = sizeof(DB_IIPRIV);
     prqef_data.dt_data = (PTR) prtuple;

     xprqeu.qeu_getnext = QEU_REPO;
     xprqeu.qeu_klen = 2;

     /* get iipriv tuple by tid */
     prqeu.qeu_flag = QEU_BY_TID;
     prqeu.qeu_getnext = QEU_NOREPO;
     prqeu.qeu_klen = 0;

     for (;;)
     {
	    status = qeu_get(qef_cb, &xprqeu);
	    if (status != E_DB_OK)
	    {
		error = xprqeu.error.err_code;
		if (xprqeu.error.err_code == E_QE0015_NO_MORE_ROWS) 
 		{
 		    status = E_DB_OK;
 		}
		break;
	    }
	   
	    /* Don't reposition, continue scan. */
            xprqeu.qeu_getnext = QEU_NOREPO;
            xprqeu.qeu_klen = 0;

	    /* Now do a tid join into IIPRIV to get the actual tuple */
	    prqeu.qeu_tid = xprtuple->dbx_tidp;

            status = qeu_get(qef_cb, &prqeu);
            if (status != E_DB_OK)
            {
                error = prqeu.error.err_code;
                break;
            }
		
	    /* Only looking for views/procedures but not grant tuples */
	    if (prtuple->db_descr_no != 0)
                    continue;

	    if (prtuple->db_dep_obj_type == DB_VIEW)
            {
		/* Found a dependent view */
                DMT_SHW_CB                  dmt_show;
	        DMT_TBL_ENTRY               dmt_tbl_entry;

	        /*
                ** initialize the control block that will be 
		** used to look up information on views.
                */
                dmt_show.type                         = DMT_SH_CB;
                dmt_show.length                       = sizeof(DMT_SHW_CB);
                dmt_show.dmt_char_array.data_in_size  = 0;
                dmt_show.dmt_char_array.data_out_size = 0;
                dmt_show.dmt_flags_mask               = DMT_M_TABLE;
                dmt_show.dmt_db_id                    = qeuq_cb->qeuq_db_id;
                dmt_show.dmt_session_id               = qeuq_cb->qeuq_d_id;
                dmt_show.dmt_table.data_address       = (PTR) &dmt_tbl_entry;
                dmt_show.dmt_table.data_in_size       = sizeof(DMT_TBL_ENTRY);
                dmt_show.dmt_char_array.data_address  = (PTR) NULL;
                dmt_show.dmt_char_array.data_in_size  = 0;
                dmt_show.dmt_char_array.data_out_size = 0;

	        STRUCT_ASSIGN_MACRO(prtuple->db_dep_obj_id, dmt_show.dmt_tab_id);

                local_status = dmf_call(DMT_SHOW, &dmt_show);
                if (   local_status != E_DB_OK
                    && dmt_show.error.err_code != E_DM0114_FILE_NOT_FOUND)
                {
                    error = dmt_show.error.err_code;
                    (VOID) qef_error(error, 0L, local_status, &error,
                        &qeuq_cb->error, 0);
                    if (local_status > status)
                        status = local_status;

		    break;
                }

	        /* If Column rename check if the column is in the map */
	        if (!(isColumnRename) ||
                     ( (BTtest (qeuq_cb->qeuq_ano,
                                (char *) prtuple->db_priv_attmap)))
		   )
	        {
	            found_view = TRUE;
	            qef_error( E_QE016E_RENAME_TAB_HAS_VIEW,
                           0L, status, &error, error_block, 4,
                           qec_trimwhite(DB_TAB_MAXNAME,
                                (char *)&table_entry->tbl_name.db_tab_name),
                           (char *)&table_entry->tbl_name.db_tab_name,
                           qec_trimwhite(DB_OWN_MAXNAME, 
				(char *)&table_entry->tbl_owner.db_own_name),
                           (char *)&table_entry->tbl_owner.db_own_name,
            		   qec_trimwhite(sizeof(dmt_tbl_entry.tbl_name),
                		(char *) &dmt_tbl_entry.tbl_name),
                	   (char *) &dmt_tbl_entry.tbl_name,
            		   qec_trimwhite(sizeof(dmt_tbl_entry.tbl_owner),
                		(char *) &dmt_tbl_entry.tbl_owner),
                	   (char *) &dmt_tbl_entry.tbl_owner
			);
		}
            } /* done view tuple */
	    else if ( prtuple->db_dep_obj_type == DB_DBP )
    	    {
              xdbpqeu.qeu_getnext = QEU_REPO;
              xdbpqeu.qeu_flag = 0;
              xdbpqeu.qeu_klen = 2;
     
              xdbpqeu.qeu_count = 1;
              xdbpqeu.qeu_tup_length = sizeof(DB_IIXPROCEDURE);
              xdbpqeu.qeu_output = &xdbpqef_data;
              xdbpqef_data.dt_next = 0;
              xdbpqef_data.dt_size = sizeof(DB_IIXPROCEDURE);
              xdbpqef_data.dt_data = (PTR) xdbptuple;
    
              xdbpqeu.qeu_key = xdbpkey_ptr_array;
      
              xdbpkey_ptr_array[0] = &xdbpkey_array[0];
              xdbpkey_ptr_array[0]->attr_number = DM_1_XDBP_KEY;
              xdbpkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
              xdbpkey_ptr_array[0]->attr_value =  (char *)&prtuple->db_dep_obj_id.db_tab_base;
    
              xdbpkey_ptr_array[1] = &xdbpkey_array[1];
              xdbpkey_ptr_array[1]->attr_number = DM_2_XDBP_KEY;
              xdbpkey_ptr_array[1]->attr_operator = DMR_OP_EQ;
              xdbpkey_ptr_array[1]->attr_value = (char *)&prtuple->db_dep_obj_id.db_tab_index;
    	      /*
    	      ** look up description of the dbproc (by doing a TID
    	      ** join from IIXPROCEDURE); 
    	      */
    	      status = qeu_get(qef_cb, &xdbpqeu);
    	      if (status != E_DB_OK)
    	      {
    	        error = xdbpqeu.error.err_code;
    	        break;
    	      }
    	      /*
    	      ** now do a TID-join into IIPROCEDURE
    	      */
    
    	      dbpqeu.qeu_flag = QEU_BY_TID;
    	      dbpqeu.qeu_klen = 0;
    	      dbpqeu.qeu_getnext = QEU_NOREPO;
    	      dbpqeu.qeu_tid = xdbptuple->dbx_tidp;
    
    	      status = qeu_get(qef_cb, &dbpqeu);
    	      if (status != E_DB_OK)
    	      {
    	          error = dbpqeu.error.err_code;
    	          if (dbpqeu.error.err_code == E_QE0015_NO_MORE_ROWS) 
                    status = E_DB_OK;
		  else
                    status = E_DB_ERROR;
                  break;
    	      }
    
    	      found_procedure = TRUE;
    	      qef_error( E_QE016D_RENAME_TAB_HAS_PROC,
                               0L, status, &error, error_block, 4,
                               qec_trimwhite(DB_TAB_MAXNAME,
                                   (char *)&table_entry->tbl_name.db_tab_name),
                               &table_entry->tbl_name.db_tab_name,
                               qec_trimwhite(DB_OWN_MAXNAME,
                                   (char *)&table_entry->tbl_owner.db_own_name),
                               &table_entry->tbl_owner.db_own_name,
                               qec_trimwhite(DB_DBP_MAXNAME, (char *)&dbptuple->db_dbpname), 
    			   &dbptuple->db_dbpname,
                               qec_trimwhite(DB_OWN_MAXNAME, (char *)&dbptuple->db_owner),
    			   &dbptuple->db_owner);
            } /* done procedure tuple  */
     } /* done reading iipriv */
    
     if( table_entry->tbl_status_mask & DMT_RULE)
     {
        /* Report any user generated rules on the table */
    	STRUCT_ASSIGN_MACRO(vqeu, rulqeu);
    	rulqeu.qeu_tab_id.db_tab_base  = DM_B_RULE_TAB_ID;
    	rulqeu.qeu_tab_id.db_tab_index = DM_I_RULE_TAB_ID;
    
    	status = qeu_open(qef_cb, &rulqeu);
    	if (status != E_DB_OK)
    	{
    	    error = rulqeu.error.err_code;
    	    break;
    	}
    	rul_opened = TRUE;
    
    	/* set up a QEU_CB fro reading IIRULE tuples */
    	rulqeu.qeu_count = 1;
    	rulqeu.qeu_tup_length = sizeof(DB_IIRULE);
    	rulqeu.qeu_output = &rulqef_data;
    	rulqef_data.dt_next = 0;
	rulqef_data.dt_size = sizeof(DB_IIRULE);
	rulqef_data.dt_data = (PTR) rultuple;

	rulqeu.qeu_key = rulkey_ptr_array;

	rulkey_ptr_array[0] = &rulkey_array[0];
	rulkey_ptr_array[0]->attr_number = DM_1_RULE_KEY;
	rulkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
        rulkey_ptr_array[0]->attr_value = (char*) &dbtable->db_tab_base;

	rulkey_ptr_array[1] = &rulkey_array[1];
	rulkey_ptr_array[1]->attr_number = DM_2_RULE_KEY;
	rulkey_ptr_array[1]->attr_operator = DMR_OP_EQ;
        rulkey_ptr_array[1]->attr_value = (char*) &dbtable->db_tab_index;

	rulqeu.qeu_flag = 0;
	rulqeu.qeu_getnext = QEU_REPO;
	rulqeu.qeu_klen = 2;

	for (;;)
	{
            status = qeu_get(qef_cb, &rulqeu);
            if (DB_FAILURE_MACRO(status))
            {
                 if (rulqeu.error.err_code == E_QE0015_NO_MORE_ROWS)
                 {
                    status = E_DB_OK;
                 }
		 else 
		 {
                    status = E_DB_ERROR;
                    error = rulqeu.error.err_code;
		 }
                 break;
            }
            rulqeu.qeu_getnext = QEU_NOREPO;

	    /* It may not be of interest to users to know the names of system 
	    ** generated rules so filter them out. Views with check option and 
	    ** constraints for which these rules exist may already be reported.
	    */
	    found_rule = TRUE;
	    if ((rultuple->dbr_flags & DBR_F_SYSTEM_GENERATED) == 0)
	    {
		     qef_error (E_QE016F_RENAME_TAB_HAS_RULE,
                           0L, status, &error, error_block, 3,
                           qec_trimwhite(DB_TAB_MAXNAME,
                               (char *)&table_entry->tbl_name.db_tab_name),
                           &table_entry->tbl_name.db_tab_name,
                           qec_trimwhite(DB_OWN_MAXNAME, (char *)&table_entry->tbl_owner.db_own_name),
                           &table_entry->tbl_owner.db_own_name,
			   qec_trimwhite(sizeof(rultuple->dbr_name),
                                          (char *)&rultuple->dbr_name),
                                          (char *)&rultuple->dbr_name);
	    }
            status = E_DB_ERROR;
	}
     } /* RULE */

     if( table_entry->tbl_status_mask & DMT_PROTECTION)
     {
	   /* Check and disallow protections with tree associated with them eg
	   ** with non grant style grants. Grants are transferred to the new
	   ** object. 
	   */
           pqeu.qeu_type = QEUCB_CB;
           pqeu.qeu_length = sizeof(QEUCB_CB);
           pqeu.qeu_db_id = qeuq_cb->qeuq_db_id;
           pqeu.qeu_lk_mode = DMT_IS;
           pqeu.qeu_flag = DMT_U_DIRECT;
           pqeu.qeu_access_mode = DMT_A_READ;
           pqeu.qeu_mask = 0;
           pqeu.qeu_f_qual = 0;
           pqeu.qeu_f_qarg = 0;

           pqeu.qeu_tab_id.db_tab_base  = DM_B_PROTECT_TAB_ID;
           pqeu.qeu_tab_id.db_tab_index  = DM_I_PROTECT_TAB_ID;
           status = qeu_open(qef_cb, &pqeu);
           if (status != E_DB_OK)
           {
               error = pqeu.error.err_code;
               break;
           }
           prot_opened = TRUE;

           pqeu.qeu_count = 1;
           pqeu.qeu_tup_length = sizeof(DB_PROTECTION);
           pqeu.qeu_output = &pqef_data;
           pqeu.qeu_input = &pqef_data;
           pqef_data.dt_next = 0;
           pqef_data.dt_size = sizeof(DB_PROTECTION);
           pqef_data.dt_data = (PTR) ptuple;

	   /*
           ** Position Protect table and get all
           ** protections matching the specified table id.
           */

           pqeu.qeu_getnext = QEU_REPO;
           pqeu.qeu_klen = 2;
           pqeu.qeu_key = pkey_ptr_array;

           pkey_ptr_array[0] = &pkey_array[0];
           pkey_ptr_array[0]->attr_number = DM_1_PROTECT_KEY;
           pkey_ptr_array[0]->attr_operator = DMR_OP_EQ;
           pkey_ptr_array[0]->attr_value = (char*) &dbtable->db_tab_base;
           pkey_ptr_array[1] = &pkey_array[1];
           pkey_ptr_array[1]->attr_number = DM_2_PROTECT_KEY;
           pkey_ptr_array[1]->attr_operator = DMR_OP_EQ;
           pkey_ptr_array[1]->attr_value = (char*) &dbtable->db_tab_index;

           pqeu.qeu_qual = 0;
           pqeu.qeu_qarg = 0;

           for(;;)     /* loop for all protection tuples for this table */
           {
              status = qeu_get(qef_cb, &pqeu);
              if (status != E_DB_OK)
              {
                  error = pqeu.error.err_code;
                  if (error == E_QE0015_NO_MORE_ROWS)
                  {
                        /* We are done with grants */
                        error = E_QE0000_OK;
                        status = E_DB_OK;
                  }
                  break;
               }
               pqeu.qeu_getnext = QEU_NOREPO;
               pqeu.qeu_klen = 0;

	       /*
	       ** Only protections created with grant statement are supported
	       ** for rename.
               */
	       if (!(ptuple->dbp_flags & DBP_GRANT_PERM))
               {
	   	  found_non_grant_prot = TRUE;

	           qef_error (E_QE0170_RENAME_TAB_HAS_NGPERM,
                           0L, status, &error, error_block, 3,
                           qec_trimwhite(DB_TAB_MAXNAME,
                               (char *)&table_entry->tbl_name.db_tab_name),
                           &table_entry->tbl_name.db_tab_name,
                           qec_trimwhite(DB_OWN_MAXNAME, (char *)&table_entry->tbl_owner.db_own_name),
                           &table_entry->tbl_owner.db_own_name,
			   sizeof(ptuple->dbp_permno),
			   &ptuple->dbp_permno);
	       }
           } /* Loop through protections */

	   if (status != E_DB_OK)
               break;
    }
    
    /* Report constraints or integrities if present */
    if(( table_entry->tbl_status_mask & DMT_INTEGRITIES) ||
       ( table_entry->tbl_2_status_mask & DMT_SUPPORTS_CONSTRAINT)) 
    {
        iqeu.qeu_type = QEUCB_CB;
        iqeu.qeu_length = sizeof(QEUCB_CB);
        iqeu.qeu_db_id = qeuq_cb->qeuq_db_id;
        iqeu.qeu_lk_mode = DMT_IS;
        iqeu.qeu_flag = DMT_U_DIRECT;
        iqeu.qeu_access_mode = DMT_A_READ;
        iqeu.qeu_mask = 0;
        iqeu.qeu_qual = NULL;
        iqeu.qeu_qarg = NULL;
        iqeu.qeu_f_qual = NULL;
        iqeu.qeu_f_qarg = NULL;
        iqeu.qeu_tab_id.db_tab_base  = DM_B_INTEGRITY_TAB_ID;
        iqeu.qeu_tab_id.db_tab_index  = DM_I_INTEGRITY_TAB_ID;
        status = qeu_open(qef_cb, &iqeu);
        if (status != E_DB_OK)
        {
            error = iqeu.error.err_code;
            break;
        }
        integ_opened = TRUE;

        iqeu.qeu_count = 1;
        iqeu.qeu_tup_length = sizeof(DB_INTEGRITY);
        iqeu.qeu_output = &iqef_data;
        iqef_data.dt_next = 0;
        iqef_data.dt_size = sizeof(DB_INTEGRITY);
        iqef_data.dt_data = (PTR) ituple;
        iqeu.qeu_getnext = QEU_REPO;
        iqeu.qeu_key = ikey_ptr_array;

        ikey_ptr_array[0] = &ikey_array[0];
        ikey_ptr_array[0]->attr_number = DM_1_INTEGRITY_KEY;
        ikey_ptr_array[0]->attr_operator = DMR_OP_EQ;
        ikey_ptr_array[0]->attr_value = (char*)
                                &qeuq_cb->qeuq_rtbl->db_tab_base;

        ikey_ptr_array[1] = &ikey_array[1];
        ikey_ptr_array[1]->attr_number = DM_2_INTEGRITY_KEY;
        ikey_ptr_array[1]->attr_operator = DMR_OP_EQ;
        ikey_ptr_array[1]->attr_value = (char*)
                                 &qeuq_cb->qeuq_rtbl->db_tab_index;
        iqeu.qeu_qual = 0;
        iqeu.qeu_klen = 2;

	for(;;)
        {
	    bool report_error = FALSE;

            status = qeu_get(qef_cb, &iqeu);
            if (status != E_DB_OK)
            {
                error = iqeu.error.err_code;
                if (error == E_QE0015_NO_MORE_ROWS) 
                {
                      error = E_QE0000_OK;
                      status = E_DB_OK;
                }
                break;
            }

            iqeu.qeu_getnext = QEU_NOREPO;

	    if (isColumnRename)
	    {
	       /* Make sure that the constraint or integrity refer to this 
	       ** column. If not then ignore it.
	       */
	       if ((BTtest (qeuq_cb->qeuq_ano,
                        (char *) ituple->dbi_columns.db_domset)) )
               {
		    report_error = TRUE;
	       }
	    }
	    else 
	    {
		/* Table rename ignore check constraints and unique constraints 
		*/
	        if ((ituple->dbi_consflags & (CONS_UNIQUE | CONS_PRIMARY)) == 0)
		    report_error = TRUE;
	    }

	   if (report_error == TRUE)
	   {
	       if (ituple->dbi_consflags == CONS_NONE)
	       {
	   	/* Old style ingres integrity */
                    qef_error (E_QE0180_RENAME_TAB_HAS_INTEGRITY,
                           0L, status, &error, error_block, 3,
                           qec_trimwhite(DB_TAB_MAXNAME,
                               (char *)&table_entry->tbl_name.db_tab_name),
                           &table_entry->tbl_name.db_tab_name,
                           qec_trimwhite(DB_OWN_MAXNAME, (char *)&table_entry->tbl_owner.db_own_name),
                           &table_entry->tbl_owner.db_own_name,
                           sizeof(ituple->dbi_cons_id.db_tab_base),
                           ituple->dbi_cons_id.db_tab_base);
	       }
	       else
	       {
                    qef_error (E_QE0171_RENAME_TAB_HAS_CONS,
                           0L, status, &error, error_block, 3,
                           qec_trimwhite(DB_TAB_MAXNAME,
                               (char *)&table_entry->tbl_name.db_tab_name),
                           &table_entry->tbl_name.db_tab_name,
                           qec_trimwhite(DB_OWN_MAXNAME, (char *)&table_entry->tbl_owner.db_own_name),
                           &table_entry->tbl_owner.db_own_name,
                           qec_trimwhite(sizeof(ituple->dbi_consname.db_constraint_name),
                                          (char *)&ituple->dbi_consname.db_constraint_name),
                                          (char *)&ituple->dbi_consname.db_constraint_name);
	       }
	       found_cons_or_integ = TRUE;
	       report_error = FALSE;
	   }
	} /* loop through all integrities */
    }

    if( table_entry->tbl_2_status_mask & DMT_ALARM)
    {
	/* If there are any security alarms dependent 
	** on the table being renamed, report them.
	*/
	i4	object_type = DBOB_TABLE;

	STRUCT_ASSIGN_MACRO(vqeu, aqeu);
	aqeu.qeu_tab_id.db_tab_base = DM_B_IISECALARM_TAB_ID;
        aqeu.qeu_tab_id.db_tab_index = DM_I_IISECALARM_TAB_ID;
        status = qeu_open(qef_cb, &aqeu);
        if (status != E_DB_OK)
        {
            error = aqeu.error.err_code;
            break;
        }
        alarm_opened = TRUE;

        aqeu.qeu_getnext = QEU_REPO;
        aqeu.qeu_klen = 3;
        aqeu.qeu_key = akey_ptr_array;
        akey_ptr_array[0] = &akey_array[0];
        akey_ptr_array[1] = &akey_array[1];
        akey_ptr_array[2] = &akey_array[2];

        akey_ptr_array[0]->attr_number = DM_1_IISECALARM_KEY;
        akey_ptr_array[0]->attr_operator = DMR_OP_EQ;
        akey_ptr_array[0]->attr_value = (char*) &object_type;

        akey_ptr_array[1]->attr_number = DM_2_IISECALARM_KEY;
        akey_ptr_array[1]->attr_operator = DMR_OP_EQ;
        akey_ptr_array[1]->attr_value = (char *)&qeuq_cb->qeuq_rtbl->db_tab_base;

        akey_ptr_array[2]->attr_number = DM_3_IISECALARM_KEY;
        akey_ptr_array[2]->attr_operator = DMR_OP_EQ;
        akey_ptr_array[2]->attr_value = (char *)&qeuq_cb->qeuq_rtbl->db_tab_index;

        aqeu.qeu_qual = 0;
        aqeu.qeu_qarg = 0;

        aqeu.qeu_count = 1;
        aqeu.qeu_tup_length = sizeof(DB_SECALARM);
        aqeu.qeu_input = &aqef_data;
        aqeu.qeu_output = &aqef_data;
        aqef_data.dt_next = 0;
        aqef_data.dt_size = sizeof(DB_SECALARM);
        aqef_data.dt_data = (PTR) atuple;

        for (;;)                /* For all alarm tuples found */
        {
            status = qeu_get(qef_cb, &aqeu);
            if (status != E_DB_OK)
            {
                if (aqeu.error.err_code == E_QE0015_NO_MORE_ROWS)
                {
                    error = E_QE0000_OK;
                    status = E_DB_OK;
                }
                else  
                {
                    error = aqeu.error.err_code;
                }
                break;
            } 
            aqeu.qeu_getnext = QEU_NOREPO;
	
	    found_alarm = TRUE;
	    qef_error (E_QE0172_RENAME_TAB_HAS_SECALARM,
                           0L, status, &error, error_block, 3,
                           qec_trimwhite(DB_TAB_MAXNAME,
                               (char *)&table_entry->tbl_name.db_tab_name),
                           &table_entry->tbl_name.db_tab_name,
                           qec_trimwhite(DB_OWN_MAXNAME, (char *)&table_entry->tbl_owner.db_own_name),
                           &table_entry->tbl_owner.db_own_name,
                           qec_trimwhite(sizeof(atuple->dba_alarmname), (char *)&atuple->dba_alarmname),
                           &atuple->dba_alarmname);
	}
     }
     exit_loop = TRUE;
   } /* End of main loop */

   if (status != E_DB_OK)
   {
	/* report the error to the user */
	error_block->err_code = error;
   }
   else if ( (status == E_DB_OK) && 
		( (found_procedure) || 
		  (found_view) || 
		  (found_rule) || 
		  (found_alarm) || 
		  (found_cons_or_integ) || 
		  (found_non_grant_prot)
		)
	    )
    {
        /* If procedure, views, rules, alarm, non grant protections or
	** check constraint or integrity are found then return error.
	** errors have been logged in the error log change the status
	** so the caller (qeu_dbu) will report the error to the user.
	*/
	if (isColumnRename)
	   error_block->err_code = E_QE0181_RENAME_COL_HAS_OBJS;
	else
	   error_block->err_code = E_QE016C_RENAME_TAB_HAS_OBJS;
        status = E_DB_ERROR;
    }

    /* Close all open tables */
    if (view_opened)
    {
        local_status = qeu_close(qef_cb, &vqeu);
        if (local_status != E_DB_OK)
        {
            (VOID) qef_error(vqeu.error.err_code, 0L, local_status,
                        &error, &qeuq_cb->error, 0);
            if (local_status > status)
                status = local_status;
	}
    }

    if (xpriv_opened)
    {
        local_status = qeu_close(qef_cb, &xprqeu);
        if (local_status != E_DB_OK)
        {
            (VOID) qef_error(xprqeu.error.err_code, 0L, local_status,
                        &error, &qeuq_cb->error, 0);
            if (local_status > status)
                status = local_status;
	}
    }

    if (priv_opened)
    {
        local_status = qeu_close(qef_cb, &prqeu);
        if (local_status != E_DB_OK)
        {
            (VOID) qef_error(prqeu.error.err_code, 0L, local_status,
                        &error, &qeuq_cb->error, 0);
            if (local_status > status)
                status = local_status;
	}
    }

    if (dbp_opened)
    {
        local_status = qeu_close(qef_cb, &dbpqeu);
        if (local_status != E_DB_OK)
        {
            (VOID) qef_error(dbpqeu.error.err_code, 0L, local_status,
                        &error, &qeuq_cb->error, 0);
            if (local_status > status)
                status = local_status;
        }
    }

    if (rul_opened)
    {
        local_status = qeu_close(qef_cb, &rulqeu);
        if (local_status != E_DB_OK)
        {
            (VOID) qef_error(rulqeu.error.err_code, 0L, local_status,
                        &error, &qeuq_cb->error, 0);
            if (local_status > status)
                status = local_status;
        }
    }

    if (xdbp_opened)
    {
        local_status = qeu_close(qef_cb, &xdbpqeu);
        if (local_status != E_DB_OK)
        {
            (VOID) qef_error(xdbpqeu.error.err_code, 0L, local_status,
                        &error, &qeuq_cb->error, 0);
            if (local_status > status)
                status = local_status;
        }
    }

    if (prot_opened)
    {
        local_status = qeu_close(qef_cb, &pqeu);
        if (local_status != E_DB_OK)
        {
            (VOID) qef_error(dbpqeu.error.err_code, 0L, local_status,
                        &error, &qeuq_cb->error, 0);
            if (local_status > status)
                status = local_status;
        }
    }

    if (integ_opened)
    {
        local_status = qeu_close(qef_cb, &iqeu);
        if (local_status != E_DB_OK)
        {
            (VOID) qef_error(dbpqeu.error.err_code, 0L, local_status,
                        &error, &qeuq_cb->error, 0);
            if (local_status > status)
                status = local_status;
        }
    }

    if (alarm_opened)
    {
        local_status = qeu_close(qef_cb, &aqeu);
        if (local_status != E_DB_OK)
        {
            (VOID) qef_error(dbpqeu.error.err_code, 0L, local_status,
                        &error, &qeuq_cb->error, 0);
            if (local_status > status)
                status = local_status;
        }
    }

    if (tree_opened)
    {
        local_status = qeu_close(qef_cb, &tqeu);
        if (local_status != E_DB_OK)
        {
            (VOID) qef_error(tqeu.error.err_code, 0L, local_status,
                        &error, &qeuq_cb->error, 0);
            if (local_status > status)
                status = local_status;
        }
    }

    if (transtarted)
    {
        if (status == E_DB_OK)
        {
            status = qeu_etran(qef_cb, &tranqeu);
            if (status != E_DB_OK)
            {
                (VOID) qef_error(tranqeu.error.err_code, 0L, status, &error,
                    &qeuq_cb->error, 0);
            }
        }

        if (status != E_DB_OK)
        {
            local_status = qeu_atran(qef_cb, &tranqeu);
            if (local_status != E_DB_OK)
            {
                (VOID) qef_error(tranqeu.error.err_code, 0L, local_status,
                            &error, &qeuq_cb->error, 0);
                if (local_status > status)
                    status = local_status;
            }
        }
    }
    return (status);
}
