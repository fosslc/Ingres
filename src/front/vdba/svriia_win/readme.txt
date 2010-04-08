*******************************************************************************
09-DEC-2001 (Separator)
According to francois noirot-nerin (noifr01):

A line will not support a multiple kinds of separators.
    Example, a line=AAAA,BBBB;CCCC DDDD
    will interprete as:
    a) Consider comma and only comma <,> as separator:
       Col1 =AAAA
       Col2 =BBBB;CCCC DDDD.
    b) Consider semicolon and only semicolon <;> as separator:
       Col1 =AAAA,BBBB
       Col2 =CCCC DDDD
    c) Consider space and only space < > as separator:
       Col1 =AAAA,BBBB;CCCC
       Col2 =DDDD
    
    Reject the possibility of 
    Col1 =AAAA
    Col2 =BBBB
    Col3 =CCCC
    Col4 =DDDD
    and any combinaison that is not in a) b) c)
    for example if a space is not considered as separator:
    Col1 =AAAA
    Col2 =BBBB
    Col3 =CCCC DDDD

*******************************************************************************
09-DEC-2001 (String Qualifier)
According to francois noirot-nerin (noifr01):

A string qualifier if exists must qualify the TOTALITY of the column AND must
precede IMMEDIATELY a separator or a begin of line AND must follow IMMEDIATELY 
by a separator or end of line.
For axample a line bellow: 
    ABCD;"XYZ;0123";ABC
    has a string qualifier "XYZ;0123" and the semicolon inside the the string
    qualifier will not interprete as a separator and the algo generates:
    Col1 =ABCD
    Col2 =XYZ;0123
    Col3 =ABC

BUT the following line (only semicolon is a UNIQUE separator)
    ABC;BEGIN X= "if a > b the 10; else 100" END;   "do you see?"
will not have any string qualifier. So it is possible to generate the following:
    Col1 =ABC
    Col2 =BEGIN X= "if a > b the 10
    Col3 = else 100" END
    Col4 =   "do you see?"



copy table onechar3varchar 
(nx1=char(0) comma, nx2=char(0) comma, nx3=char(0) comma, nx4=char(0) nl) 
from 'csv6.csv'

copy table "administrator".onechar3varchar 
(nx1=char(0) ',', nx2=char(0) ',', nx3=char(0) ',', nx4=char(0) nl) 
from 'file6.txt'

************** will copy to col_integer a vulue (null)*************
copy table "administrator".onechar1integer 
(col_char=char(0)',', col_integer='d0nl') 
from 'onechar1integer.txt'

*************** OK ********************** Read as characters
copy table "administrator".onechar1integer 
(col_char=char(0)',', col_integer=char(0) nl) 
from 'onechar1integer.txt'

******************OK************DECIMAL
copy table "administrator".onechar1decimal 
(char_col=char(0)',', decimal_col=char(0) nl) 
from 'onechar1decimal.csv'\g

******************OK************WITH NULL ( ,, ===> specify WITH NULL (''))
copy table "administrator".onechar1decimal 
(char_col=char(0)',' with null(''), decimal_col=char(0) nl) 
from 'onechar1decimal.csv'\g

******************* SKIP COLUMN **************************** OK nx3, nx4 contains (null)
copy table onechar3varchar 
(nx1=char(0) comma, nx2=char(0) comma, xyz='d0,', nx4=char(0) nl) 
from 'csv6.csv'\g

copy table onechar3varchar 
(nx1=char(0) comma, nx2=char(0) comma, nx3=char(0) comma, nx4='d0nl') 
from 'csv6.csv'


**************************
create table tballtypes (
	a1 char(10),
	a2 varchar (10),
	a3 c(10),
	a4 text(10),
	a5 byte(10),
	a6 byte varying(10),
	a7 money,
	a8 integer1,
	a9 integer2,
	b1 integer,
	b2 float,
	b3 float4,
	b4 smallint,
	b5 date)


// col Type         output type length
// a8  integer1,    integer      1
// a9  integer2,    integer      2
// b1  integer,     integer      4
// b2  float,       float        8
// b3  float4,      float        4
// b4  smallint,    integer      2


TODO:
****
	OK	+Handle DBF format III
		+Handle all (III+, IV, V, 7)
	OK	+Import Parameter store in file
	OK	+Create Table
	OK	+Create Copy Statement
	OK	+ANSI/OEM
	OK	+STEP 3 / Fixed width rows
	OK	+Fix width (gui)
	OK	+Fix width (gui), Special v-scroll
	OK	+Detect fix width
		+Set IIDATE Format temporary
	OK	+Can used copy without temporary file ?
	OK	+Create temporary file
	OK	+Handle Skip columns (Create temp file)
	OK	+Create insert statement
	OK	+Run the copy statement.
	OK	+Run insert statement
	OK	+TABs > 100 => MSG
	OK	+If Fixed Width is detected and KB2Scan > 0 => MSG
	OK	+Step 2, put length at colname : EX Col1(n [, p]) n=effective length, p=trailing spaces
	OK	+STEP1 OnWizardNext(), If exists TQ,CS and  KB2Scan > 0 => MSG + Enable combo of TQ (where 2 display MSG ?)
	OK	+STEP4 Check if can use COPY directly (coltype=char & colvalue contains trailing space => cannot ...)
	OK	+Check if the table is empty
	OK	+Fix width (handle skip amount of lines) common in the first step.
	OK	+Step 2 Add check box at each tab of proposal solution.
	OK	 OnNext ==> Select the tab where the check box is ON.
	OK	+Step 4 Add check box (replace the existing rows/append).
	OK	+Move the item <new table> to the top
		+Step1 (Display tree in the form [owner].object sort by objects
		+Detect if INGRES DBMS is alive when startup.
		+Detect separator inside "". a.csv

	OK	+STEP3 validate the skip columns + change column orders
	OK	+Step3, icon check mark with the dynamic text depending on (Existing/New Table)
	OK	+File *.* (all file formats)
	OK	+Check file (CR or line too long)
		+XML detection.
		+Help management.
		+Handle the output messages.******** TO BE VERIFIED
		**+Animation while running COPY/INSERT Statement.
		**+Documentation


*************** OK ********************** Read as varchar(0)
copy table tb_4col_char
(col1=varchar(0), col2=varchar(0), col3=varchar(0), col4=varchar(0) nl) 
from 'varchar(0).txt'






****************
00 = 
01 =  
02 =  
03 =  
04 =  
05 =  
06 =  
07 = 
08 =  
09 = 	
10 = 

11 = 

12 = 
 
13 = 
14 = 
 
15 =  
16 =  
17 =  
18 =  
19 =  
20 =  
21 =  
22 =  
23 =  
24 =  
25 =  
26 =  
27 =  
28 =  
29 =  
30 = -
31 = -
32 =  
33 = !
34 = "
35 = #
36 = $
37 = %
38 = &
39 = '
40 = (
41 = )
42 = *
43 = +
44 = ,
45 = -
46 = .
47 = /
48 = 0
49 = 1
50 = 2
51 = 3
52 = 4
53 = 5
54 = 6
55 = 7
56 = 8
57 = 9
58 = :
59 = ;
60 = <
61 = =
62 = >
63 = ?
64 = @
65 = A
66 = B
67 = C
68 = D
69 = E
70 = F
71 = G
72 = H
73 = I
74 = J
75 = K
76 = L
77 = M
78 = N
79 = O
80 = P
81 = Q
82 = R
83 = S
84 = T
85 = U
86 = V
87 = W
88 = X
89 = Y
90 = Z
91 = [
92 = \
93 = ]
94 = ^
95 = _
96 = `
97 = a
98 = b
99 = c
100 = d
101 = e
102 = f
103 = g
104 = h
105 = i
106 = j
107 = k
108 = l
109 = m
110 = n
111 = o
112 = p
113 = q
114 = r
115 = s
116 = t
117 = u
118 = v
119 = w
120 = x
121 = y
122 = z
123 = {
124 = |
125 = }
126 = ~
127 = 
128 = Ä
129 = Å
130 = Ç
131 = É
132 = Ñ
133 = Ö
134 = Ü
135 = á
136 = à
137 = â
138 = ä
139 = ã
140 = å
141 = ç
142 = é
143 = è
144 = ê
145 = '
146 = '
147 = "
148 = "
149 = o
150 = -
151 = -
152 = ò
153 = ô
154 = ö
155 = õ
156 = ú
157 = ù
158 = û
159 = ü
160 =  
161 = °
162 = ¢
163 = £
164 = §
165 = •
166 = ¶
167 = ß
168 = ®
169 = ©
170 = ™
171 = ´
172 = 
173 = ≠
174 = Æ
175 = Ø
176 = ∞
177 = ±
178 = ≤
179 = ≥
180 = ¥
181 = µ
182 = 
183 = ∑
184 = ∏
185 = π
186 = ∫
187 = ª
188 = º
189 = Ω
190 = æ
191 = ø
192 = ¿
193 = ¡
194 = ¬
195 = √
196 = ƒ
197 = ≈
198 = ∆
199 = «
200 = »
201 = …
202 =  
203 = À
204 = Ã
205 = Õ
206 = Œ
207 = œ
208 = –
209 = —
210 = “
211 = ”
212 = ‘
213 = ’
214 = ÷
215 = ◊
216 = ÿ
217 = Ÿ
218 = ⁄
219 = €
220 = ‹
221 = ›
222 = ﬁ
223 = ﬂ
224 = ‡
225 = ·
226 = ‚
227 = „
228 = ‰
229 = Â
230 = Ê
231 = Á
232 = Ë
233 = È
234 = Í
235 = Î
236 = Ï
237 = Ì
238 = Ó
239 = Ô
240 = 
241 = Ò
242 = Ú
243 = Û
244 = Ù
245 = ı
246 = ˆ
247 = ˜
248 = ¯
249 = ˘
250 = ˙
251 = ˚
252 = ¸
253 = ˝
254 = ˛
255 = ˇ

D:\Development\Projects\COM\Xiia\App\dq-a-na.csv

      {"US"              , dd-mmm-yyyy},
      {"MULTINATIONAL"   , dd/mm/yy},
      {"MULTINATIONAL4"  , dd/mm/yyyy
      {"ISO"             , yymmdd},
      {"ISO4"            , yyyymmdd},
      {"SWEDEN or FINLAND", yyyy-mm-dd},
      {"GERMAN"          , dd.mm.yy},
      {"YMD"             , yyyy-mmm-dd},
      {"DMY"             , dd-mmm-yyyy},
      {"MDY"             , mmm-dd-yyyy},

Hi Ray,

Thanks for your information !
I have changed the codes so that the node name and database name are hard coded (there is no connection to the DBMS).
The only one connection is the one established prior to the execution of the insert statement.
It seems that the "put environment" has no effect to the DBMS.


To do this (implementation code lines):

	In CxDlgTrackIIDataFormat::OnInitDialog(), I comment the line ComboTable(&m_cComboTable);
	BOOL CxDlgTrackIIDataFormat::OnInitDialog() 
	{
		CDialog::OnInitDialog();
	
		ComboDataFormat(&m_cComboIIDate);
		ComboNode(&m_cComboNode);
		ComboDatabase(&m_cComboDatabase);
		//	ComboTable(&m_cComboTable);

		return TRUE;  	// return TRUE unless you set the focus to a control
	              		// EXCEPTION: OCX Property Pages should return FALSE
	}
In the function member CxDlgTrackIIDataFormat::ComboDatabase(CComboBox* pCombo), I change to the following:
	try
	{
		CString strItem;
		CaLLQueryInfo info;
		CTypedPtrList< CObList, CaDBObject* > listObj;
		m_cComboDatabase.ResetContent();
		m_cComboDatabase.AddString ("uknode");

		/*
		m_cComboNode.GetWindowText (strItem);
		info.SetNode(strItem);
		info.SetObjectType (OT_DATABASE);
		info.SetIncludeSystemObjects(FALSE);

		INGRESII_llQueryObject (&info, listObj,  NULL);
		POSITION pos = listObj.GetHeadPosition();
		while (pos != NULL)
		{
			CaDBObject* pObj = listObj.GetNext (pos);
			pCombo->AddString (pObj->GetName());
		}
		while (!listObj.IsEmpty())
			delete listObj.RemoveHead();
		*/
	}
	catch (CeSqlException e)
	{
		AfxMessageBox (e.GetReason());
	}

	pCombo->SetCurSel(0);
Last, I comment all the body codes of menber function CxDlgTrackIIDataFormat::ComboTable(CComboBox* pCombo)

Vut.

