/*
**  Copyright (C) 2005-2006 Ingres Corporation. All Rights Reserved.		       
*/

/*
**    Source   : detaidml.cpp: implementation for including the file generated
**               from ESQLCC -multi -fdetaidml.inc detaidml.scc
**    Project  : Meta data library 
**    Author   : Sotheavut UK (uk$so01)
**    Purpose  : Access low lewel data (Ingres Database)
**
** History:
**
** 06-Oct-2000 (uk$so01)
**    Created
** 04-Mar-2003 (uk$so01)
**    SIR #109718, Avoiding the duplicated source files by integrating the libraries.
**/


#include "stdafx.h"
#include "usrexcep.h"
#include "qryinfo.h"
#include "constdef.h"
#include "dmldbase.h" // Database
#include "dmluser.h"  // User
#include "dmlgroup.h" // Group
#include "dmlprofi.h" // Profile
#include "dmlrole.h"  // Role
#include "dmllocat.h" // Location
#include "dmlvnode.h" // CaNode, CaNodeXxx
#include "dmlproc.h"  // CaProcedure
#include "dmltable.h" // CaTable
#include "dmlview.h"  // CaView
#include "dmlindex.h" // CaIndex
#include "dmlrule.h"  // CaRule
#include "dmlinteg.h" // CaIntegrity
#include "dmldbeve.h" // CaDBEvent
#include "dmlalarm.h" // CaAlarm
#include "dmlsynon.h" // CaSynonym

#include "sessimgr.h" // CaSession


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif




//
// This file "" is generated from ESQLCC -multi -fdetaidml.inc detaidml.scc
#include "detaidml.inc"



