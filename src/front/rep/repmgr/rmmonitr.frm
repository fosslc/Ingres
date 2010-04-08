COPYFORM:	6.4	1996_08_09 16:58:23 GMT  
FORM:	rmreplic_monitor		
	81	23	0	0	15	0	8	9	0	0	0	0	0	64	0	19
FIELD:
	0	db	21	38	0	36	1	36	0	44	36	0	0		0	0	0	0	512	0	0		+c36			0	0
	1	servers	0	5	0	2	7	80	3	0	1	1	0		1	1	0	16417	0	0	0					1	1
	0	server_no	30	4	0	6	1	6	0	1	6	1	1		1	-1	0	0	512	0	0		+n6			2	2
	1	ebuff	21	102	0	71	1	71	0	8	71	1	8		8	-1	0	0	576	0	0		c71			2	3
	2	dd_insert	30	4	0	7	1	21	11	2	7	0	14	Inserts:	0	0	0	0	512	0	0		+"zzz,zzz"			0	4
	3	input	30	4	0	7	1	21	11	54	7	0	14	Input:	0	0	0	0	512	0	0		+"zzz,zzn"			0	5
	4	distribution	30	4	0	7	1	21	12	54	7	0	14	Distribution:	0	0	0	0	512	0	0		+"zzz,zzn"			0	6
	5	dd_update	30	4	0	7	1	21	12	2	7	0	14	Updates:	0	0	0	0	512	0	0		+"zzz,zzz"			0	7
	6	tblfield	0	5	0	2	7	80	16	0	1	1	0		1	1	0	16417	0	0	0					1	8
	0	ebuff	21	68	0	66	1	66	0	1	66	0	1		1	-1	0	0	512	0	0		c66			2	9
	1	num	30	4	0	11	1	11	0	68	11	0	68		68	-1	0	0	512	0	0		+"zzz,zzz,zzz"			2	10
	7	dd_delete	30	4	0	7	1	21	13	2	7	0	14	Deletes:	0	0	0	0	512	0	0		+"zzz,zzz"			0	11
	8	dd_transaction	30	4	0	7	1	21	15	2	7	0	14	Transactions:	0	0	0	0	512	0	0		+"zzz,zzz"			0	12
	9	dd_insert2	30	4	0	7	1	21	11	28	7	0	14	Inserts:	0	0	0	0	512	0	0		+"zzz,zzz"			0	13
	10	dd_update2	30	4	0	7	1	21	12	28	7	0	14	Updates:	0	0	0	0	512	0	0		+"zzz,zzz"			0	14
	11	dd_delete2	30	4	0	7	1	21	13	28	7	0	14	Deletes:	0	0	0	0	512	0	0		+"zzz,zzz"			0	15
	12	dd_transaction2	30	4	0	7	1	21	15	28	7	0	14	Transactions:	0	0	0	0	512	0	0		+"zzz,zzz"			0	16
	13	total	30	4	0	7	1	21	14	2	7	0	14	Total  :	0	0	0	0	2560	0	0		+"zzz,zzz"		dd_insert + dd_delete + dd_update	0	17
	14	total_2	30	4	0	7	1	21	14	28	7	0	14	Total  :	0	0	0	0	2560	0	0		+"zzz,zzz"		dd_insert2 + dd_update2 + dd_delete2	0	18
TRIM:
	0	0	RepMgr - Replication Monitor	0	0	0	0
	0	9	8:27:0	1	0	0	0
	1	2	Server	0	0	0	0
	2	10	Outgoing Records	0	0	0	0
	14	2	Status	0	0	0	0
	28	10	Incoming Records	0	0	0	0
	52	9	8:28:1	1	0	0	0
	54	10	Queues	0	0	0	0
