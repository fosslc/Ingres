COPYFORM:	6.4	1993_07_20 01:31:16 GMT  
FORM:	roptions		
	80	28	0	0	8	0	14	9	0	0	0	0	0	64	0	13
FIELD:
	0	rpl	32	2	0	2	1	23	2	12	2	0	21	Page Length (lines):	0	0	0	65536	0	0	0		c2	Page length must be in range 0-99, or blank.	rpl = "[0-9][0-9]" or rpl = " " or rpl = "[0-9]"	0	0
	1	rulchar	32	1	0	1	1	24	4	10	1	0	23	Underlining Character:	0	0	0	65536	0	0	0		c1			0	1
	2	rphdr_first	32	3	0	3	1	27	7	9	3	0	24	on first page (yes/no):	0	0	0	65600	0	0	0	y	c3	Answer must be "yes" or "no".	rphdr_first in ["y", "ye","yes","n","no"]	0	2
	3	rnullstring	32	30	0	30	1	54	9	9	30	0	24	Display Null Values as:	0	0	0	65536	0	0	0		c30			0	3
	4	rffs	32	3	0	3	1	30	11	6	3	0	27	Insert Formfeeds (yes/no):	0	0	0	65600	0	0	0	n	c3	Answer must be "yes" or "no".	rffs in ["y","ye","yes","n","no", ""]	0	4
	5	rfirstff	21	5	0	3	1	34	12	2	3	0	31	Print First Formfeed (yes/no):	0	0	0	64	0	0	0		c3	Answer must be "yes" or "no".	rfirstff in ["y","ye","yes", "n","no",""]	0	5
	6	ropttbl	0	4	0	2	8	39	14	1	1	3	0		1	1	0	1073741857	0	0	0					1	6
	0	ropt_section	21	52	0	25	1	25	0	1	25	3	1	Report Section	1	1	0	0	576	0	0		c25			2	7
	1	ropt_under	21	6	0	4	1	11	0	27	4	3	27	Underlining	27	1	0	64	0	0	0		c4	Enter what type of underlining you want for the report section.  Select "none", "all" or "last".	ropttbl.ropt_under in ["a","al","all","l","la","las","last","n","no","non","none"]	2	8
	7	rcomp	0	3	0	3	5	37	17	42	1	1	0		1	1	0	1073758241	0	0	0					1	9
	0	p_comp	-21	12	0	9	1	9	0	1	9	0	1		1	-1	0	0	512	0	0		c9			2	10
	1	include	21	11	0	9	1	9	0	11	9	0	11		11	-1	0	64	0	0	0		c9	Answer must be "yes" or "no".	rcomp.include in ["y","ye","yes","n","no",""]	2	11
	2	format	21	98	0	15	1	15	0	21	15	0	21		21	-1	0	0	64	0	0		c15			2	12
TRIM:
	0	0	RBF - Report Options	0	0	0	0
	6	6	Print Page Header	0	0	0	0
	40	2	(Default is 20 when report is written	0	0	0	0
	40	4	(Default is '-' when report is written	0	0	0	0
	41	3	to a terminal and 61 to a file.)	0	0	0	0
	41	5	to a terminal and '_' to a file.)	0	0	0	0
	42	14	4:37:0	1	0	0	0
	43	16	Component	0	0	0	0
	45	15	Page	0	0	0	0
	52	14	4:1:1	1	0	0	0
	53	16	in Report	0	0	0	0
	54	15	Include	0	0	0	0
	62	14	4:1:2	1	0	0	0
	67	16	Format	0	0	0	0
