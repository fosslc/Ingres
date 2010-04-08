COPYFORM:	6.4	1992_11_11 00:30:19 GMT  
FORM:	coptions		
	100	24	0	0	1	0	16	9	0	0	0	0	0	64	0	5
FIELD:
	0	rfsrttbl	0	10	0	4	12	59	10	11	1	1	0		0	0	0	1073759265	0	0	0					1	0
	0	srt_colname	-21	35	0	25	1	25	0	1	25	0	1	 Column Name	1	-1	0	0	576	0	0		c25			2	1
	1	srt_seq	30	4	0	3	1	9	0	27	3	0	27	 Sequence	27	-1	0	65536	0	0	0	0	-i3	Sorting Sequence must be in the range 0 - 300.	rfsrttbl.srt_seq>=0 and rfsrttbl.srt_seq <=300	2	2
	2	srt_dir	32	10	0	10	1	10	0	37	10	0	37	 Direction	37	-1	0	64	0	0	0		c10	The sort direction must be either "ascending" or "descending".	rfsrttbl.srt_dir in ["a","as","asc","asce","ascen","ascend","ascendi","ascendin","d","de","des","desc","desce","descen","descend","descendi","descending","ascending","descendin"]	2	3
	3	srt_selval	32	5	0	5	1	10	0	48	5	0	48	 Selection	48	-1	0	64	0	0	0		c5	Selection criteria must be either "none", "value" or "range".	rfsrttbl.srt_selval in ["n","no","non","none","v","va","val","valu","value","r","ra","ran","rang","range"]	2	4
TRIM:
	0	0	RBF - Column Options	0	0	0	0
	2	2	Set the column options by filling in the appropriate information for each	0	0	0	0
	2	3	column.  For sort columns the sequence must be in the range 1 - 300.  The	0	0	0	0
	2	4	sort direction is "ascending" or "descending".  The runtime selection	0	0	0	0
	2	5	criteria must be "none", "value" or "range".	0	0	0	0
	11	7	4:59:0	1	0	0	0
	12	9	Column Name	0	0	0	0
	37	7	4:1:1	1	0	0	0
	38	8	Sort	0	0	0	0
	38	9	Sequence	0	0	0	0
	47	7	4:1:2	1	0	0	0
	48	8	Sort	0	0	0	0
	48	9	Direction	0	0	0	0
	58	7	4:1:3	1	0	0	0
	59	8	Selection	0	0	0	0
	59	9	Criteria	0	0	0	0
