/*
**  fkerr.h
**
**  Header file containing error message numbers for termdr and
**  function keys.
**
**  Copyright (c) 2004 Ingres Corporation
**
**
**  History:
**
**	Created - 07/11/85 (dkh)
 * Revision 61.1  88/03/25  23:35:05  sylviap
 * *** empty log message ***
 * 
 * Revision 61.1  88/03/25  21:17:39  sylviap
 * *** empty log message ***
 * 
 * Revision 60.4  88/03/24  10:42:39  sylviap
 * Integrated with DG path.
 * 
 * Revision 60.3  87/04/08  00:38:43  joe
 * Added compat, dbms and fe.h
 * 
 * Revision 60.2  86/11/19  13:34:14  barbara
 * Initial60 edit of files
 * 
 * Revision 60.1  86/11/19  13:33:59  barbara
 * *** empty log message ***
 * 
 * Revision 40.1  85/08/22  15:51:34  dave
 * Initial revision
 * 
 * Revision 1.3  85/08/09  11:18:23  dave
 * Added two new error messages, FKNOESC and FKNOPFK. (dkh)
 * 
 * Revision 1.2  85/07/24  11:42:13  dave
 * Added new error message 8648. (dkh)
 * 
 * Revision 1.1  85/07/19  11:05:10  dave
 * Initial revision
 * 
*/

/*
**  Constants for error messages.
*/

# define	FKCANTOPEN		8600
# define	FKERRSFND		8601
# define	FKERROPEN		8602
# define	FKBADHAT		8603
# define	FKBADBSQ		8604
# define	FKAFBSQ			8605
# define	FKNONUM			8606
# define	FKBADNUM		8607
# define	FKNUMCONV		8608
# define	FKNOKEY			8609
# define	FKNOALW			8610
# define	FKKEYSELF		8611
# define	FKBADCHAR		8612
# define	FKFIRSTCH		8613
# define	FKDIGITS		8614
# define	FKEQUAL			8615
# define	FKKYRES			8616
# define	FKINSOVFL		8617
# define	FKOVFL			8618
# define	FKREFDEF		8619
# define	FKNODEF			8620
# define	FKNULLEXP		8621
# define	FKBSTART		8622
# define	FKFRSCTRL		8623
# define	FKFRSKEY		8624
# define	FKUNKNOWN		8625
# define	FKNOEQUAL		8626
# define	FKPFKEY			8627
# define	FKBPFKEY		8628
# define	FKBADCMD		8629
# define	FKOFFIGN		8630
# define	FKBADLABEL		8631
# define	FKLBLIGN		8632
# define	FKAFHAT			8633
# define	FKCTRLPF		8634
# define	FKNODEFMAP		8640
# define	FKNOMFILE		8641
# define	FKINUSE			8642
# define	FKBADLCMD		8643
# define	FKOVRRIDE		8644
# define	FKBADRCMD		8645
# define	FKKEYZERO		8646
# define	FKMAXMENU		8647
# define	FKBNUMEX		8648
# define	FKNOESC			8649
# define	FKNOPFK			8650

# define	TDNOTERM		8635
# define	TDUNKNOWN		8636
# define	TDNODEV			8637
# define	TDCURINIT		8638
# define	TDMSGINIT		8639

