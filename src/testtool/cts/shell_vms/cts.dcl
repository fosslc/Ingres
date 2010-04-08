$ verify = f$verify(0)
$! CTS.COM
$!	This is the script that starts CTS (concurrency test system) on a single
$!	node or on a clustered installation with more than one node.  This 
$!	script starts the coordinator and the slaves on batch queues on one or
$!	several different nodes. The script ctsnodes.com contains a list
$!	of the nodes and the batch queue for each node. This script should be
$!	updated if you are running on different environments.
$
$!	Before you can run this script you should check for three things:
$!	1. The cts database has been created. Use tst_basis:initctsdb.com to 
$!		create the database. 
$!	2. The batch queue that is going to be used to run this test is up and 
$!		running.
$!	3. The source code has been compiled and linked with the release you 
$!		are testing. To link the code you can use the script 
$!		makects.com which can can be found in this directory.
$
$!	Parameteres:
$!	p1	database name 
$!	p2	test number	(Note that this is the CTS test number, not the
$!				 test suite number. PLease refer to the CTS docu-
$!				 mentation for more information on the tests or 
$!				 look at the table test_index for a list of the
$!				 tests, how many slaves they have and which 
$!				 routines they execute)
$!	p3	Slave number	 This parameter is the number of slaves that
$!				 you want to run in this test. Note that the
$!				 number of slaves for each test is defined in
$!				 the table test_index.
$!	p4	Result file name This parameter should be the name you want to
$!				 give to the result files.
$!	p5	Traceflag	 This traceflag indicates whether you want a
$!				 complete trace of all the actions that the 
$!				 coordinator and slave do. If you pass 0 you
$!				 will get some messages in the result files 
$!				 that indicate at what the coordinator and
$!				 slaves are doing. If you pass 1 you will get
$!				 complete information about the routines that
$!			 	 each slave is executing. When running the tests
$!				 as part of the automated test suite you should
$!				 pass 0 instead of 1.
$!	CTSNODES.COM		A command file that will supply the following
$!				list of cluster node information.
$!
$!	ctsnodes	How many nodes should be used to run this test.
$!	node#		Name of the nodes available, with # being substituted
$!			with at number (e.g. node1, node2).
$!	node#que	The batch queue associated with the defined nodes.
$!			(e.g. node1 had queue node1que assoicated with it).
$!	total_nodes	The currently number of nodes define for this cluster
$!			installations.  Also the maxinum number cts_nodes can
$!			be defined as.
$!	master_node	Defines the master node on the cluster.
$!
$!
$!	Output:
$!		The coordinator and each one of the slaves have a result 
$!		file. The name of the result files should be passed by you as
$!		one of the parameters for CTS. If you use the value hk000a, the
$!		result files will be hk000a.res, hk000a1.res, hk000a2.res, etc.
$
$
$
$!
$!	Run ctsnodes to define the nodes and batch queues in this cluster 
$!	installation
$! Modifications:
$!	Jul-11-97 (rosga02)
$!		Changed location of CTSNODE.COM, SLAVE.COM, COORD.COM
$!
$
$ @TOOLS_DIR:[bin]ctsnodes.com
$
$!
$!	Make sure there are input parameters
$!
$ if (p1 .nes. "") .and. (p2 .nes. "") .and. (p3 .nes. "") -
	.and. (p4 .nes. "") - 
  then $goto ARGSOK
$   write sys$output-
    "Usage: cts <database> <test_number> <no_of_kids> <testid> <traceflag> <num_nodes>"
$   exit
$
$!
$!	It is not possible to run zero or a negative number of slaves
$!	
$ ARGSOK:
$ if (p3 .gt. 0) then $goto NUMSOK
$ 	write sys$output "Can't start "'p3" slaves -- Try again"
$ 	exit
$
$!
$!	There needs to be at least one node on which to run the CTS test.  
$!	Test will allow for a default to one node if nothing is entered, but
$!	if a value is entered, the value number to greater than 0 and less 
$!	than or equal to the total number of nodes defined in ctsnodes.com
$ NUMSOK:
$ if (ctsnodes .eq. 0) then ctsnodes = 1
$ if (ctsnodes .gt. 0) then $goto NODESOK
$  write sys$output "The number of nodes must be between 1 and ''total_nodes'"
$  exit
$
$ NODESOK:
$ if (ctsnodes .le. total_nodes) then $goto MAXNODESOK
$  write sys$output "The number of nodes must be between 1 and ''total_nodes'"
$  exit
$
$!
$!	Start the coordinator
$!
$ MAXNODESOK:
$ if (p5 .nes. "1") then p5 = 0
$
$! dbname = F$TRNLNM("''p1'")
$! sho sym p1
$!
$ nodecount == 0
$ quename=''master_que'
$ nodename=''master_node'
$ mtr_node :== 'quename'
$!
$ submit/restart/que='quename'/noprint/log=ing_tst:[output]'p4'_c.log -
	/noidentify/param=('p1,'p2,'p3,'p4,'p5)/name='p4'_c -
		TOOLS_DIR:[bin]coord.com
$ write sys$output "Started CTS coordinator"
$ write sys$output ""
$!
$!	Start the slaves
$!
$ total_slaves == p3
$ count == 1
$ nodecount == nodecount + 1
$
$ STARTLOOP:
$ quename=node'nodecount'que
$ nodename=node'nodecount'
$!
$ submit/que='quename'/noprint/log=ing_tst:[output]'p4'_'count'.log -
	/noidentify/param=('p1,'p2,'count,'p4,'p5)/name='p4'_'count' -
		TOOLS_DIR:[bin]slave.com
$ write sys$output "Started CTS slave number ''count'"
$ write sys$output ""
$ nodecount == nodecount + 1
$
$! the following show symbols are for stress test development only
$! sho sym count
$! sho sym ctsnodes
$ if count .gt. 50
$ then
$	ctsnodes = 1
$!	sho sym ctsnodes
$ else
$	if count .gt. 50
$	then 
$		ctsnodes = 2
$!		sho sym ctsnodes
$	endif
$ endif
$	
$ if nodecount .gt. ctsnodes then nodecount == 1
$ count == count + 1
$ if (count .le. total_slaves) then $goto STARTLOOP
$!
$ write sys$output "All CTS slaves are running"
$ write sys$output ""
$
$ synchronize/que='mtr_node' 'p4'_c
$!
$ write sys$output "CTS coordinator and slaves are done"
$!
$ if verify .eq. 1 then set verify
$ exit
