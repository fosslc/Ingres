/*
** pslsscantoks.h generated from pslsscantoks.dat
** using pslscanprep.awk
*/


static const u_char Key_string[] = {
/*
** Copyright 2010, Ingres Corporation
**
**	pslstoks.dat - token file for SQL
**
** Input file for pslscanprep.awk to produce pslsscantoks.h for inclusion
** into pslsscan.c
**
** The numbers correspond to the indes into Key_index tables in the scanner.
** The tokens need to be ordered according to then and alphabetically within
** the lengths. Violations of this will be reported,
**
** History:
**	18-Mar-2010 (kiria01) b123438
**	    Created for sanity sake.
**	19-Mar-2010 (gupsh01) SIR 123444
**	    Added rename keyword.
**	14-Apr-2010 (toumi01) SIR 122403
**	    Add encrypt token.
*/
                  /* Keywords of length 0, and other non-existent lengths */
/* 0 */           0,
                  /* Keywords of length 2 */
/* 1 */           1, 'a', 's', 0,
                  2, 'a', 't', 0,
                  3, 'b', 'y', 0,
                 26, 'd', 'o', 0,
                 27, 'i', 'f', 0,
                  4, 'i', 'n', 0,
                  5, 'i', 's', 0,
                167, 'n', 'o', 0,
                  6, 'o', 'f', 0,
                  7, 'o', 'n', 0,
                  8, 'o', 'r', 0,
                  9, 't', 'o', 0,
/* 49 */          0,
                  /* Keywords of length 3 */
/* 50 */        102, 'a', 'd', 'd', 0,
                 10, 'a', 'l', 'l', 0,
                 11, 'a', 'n', 'd', 0,
                 12, 'a', 'n', 'y', 0,
                 63, 'a', 's', 'c', 0,
                 13, 'a', 'v', 'g', 0,
                 57, 'e', 'n', 'd', 0,
                 61, 'f', 'o', 'r', 0,
                 14, 'm', 'a', 'x', 0,
                 15, 'm', 'i', 'n', 0,
                 16, 'n', 'o', 't', 0,
                193, 'o', 'u', 't', 0,
                157, 'r', 'o', 'w', 0,
                 17, 's', 'e', 't', 0,
                 64, 's', 'q', 'l', 0,
                 18, 's', 'u', 'm', 0,
                162, 't', 'o', 'p', 0,
/* 135 */         0,
                  /* Keywords of length 4 */
/* 136 */        51, 'b', 'a', 's', 'e', 0,
                155, 'c', 'a', 's', 'e', 0,
                183, 'c', 'a', 's', 't', 0,
                 20, 'c', 'o', 'p', 'y', 0,
                189, 'c', 'u', 'b', 'e', 0,
                 66, 'd', 'r', 'o', 'p', 0,
                 32, 'e', 'l', 's', 'e', 0,
                202, 'f', 'r', 'e', 'e', 0,
                 21, 'f', 'r', 'o', 'm', 0,
                108, 'f', 'u', 'l', 'l', 0,
                 23, 'i', 'n', 't', 'o', 0,
                109, 'j', 'o', 'i', 'n', 0,
                110, 'l', 'e', 'f', 't', 0,
                 67, 'l', 'i', 'k', 'e', 0,
                168, 'n', 'e', 'x', 't', 0,
                 68, 'n', 'u', 'l', 'l', 0,
                151, 'o', 'n', 'l', 'y', 0,
                 59, 'o', 'p', 'e', 'n', 0,
                143, 'r', 'e', 'a', 'd', 0,
                124, 'r', 'o', 'w', 's', 0,
                 25, 's', 'a', 'v', 'e', 0,
                 69, 's', 'o', 'm', 'e', 0,
                218, 's', 'r', 'i', 'd', 0,
                 33, 't', 'h', 'e', 'n', 0,
                213, 't', 'r', 'u', 'e', 0,
                 24, 'u', 's', 'e', 'r', 0,
                 97, 'v', 'i', 'e', 'w', 0,
                114, 'w', 'h', 'e', 'n', 0,
                 28, 'w', 'i', 't', 'h', 0,
                 70, 'w', 'o', 'r', 'k', 0,
/* 316 */         0,
                  /* Keywords of length 5 */
/* 317 */        29, 'a', 'b', 'o', 'r', 't', 0,
                100, 'a', 'l', 't', 'e', 'r', 0,
                 30, 'b', 'e', 'g', 'i', 'n', 0,
                136, 'b', 'y', 'r', 'e', 'f', 0,
                169, 'c', 'a', 'c', 'h', 'e', 0,
                 71, 'c', 'h', 'e', 'c', 'k', 0,
                 60, 'c', 'l', 'o', 's', 'e', 0,
                 31, 'c', 'o', 'u', 'n', 't', 0,
                166, 'c', 'r', 'o', 's', 's', 0,
                170, 'c', 'y', 'c', 'l', 'e', 0,
                 36, 'e', 'n', 'd', 'i', 'f', 0,
                212, 'f', 'a', 'l', 's', 'e', 0,
                 99, 'f', 'e', 't', 'c', 'h', 0,
                162, 'f', 'i', 'r', 's', 't', 0,
                 72, 'g', 'r', 'a', 'n', 't', 0,
                 73, 'g', 'r', 'o', 'u', 'p', 0,
                 58, 'i', 'n', 'd', 'e', 'x', 0,
                111, 'i', 'n', 'n', 'e', 'r', 0,
                194, 'i', 'n', 'o', 'u', 't', 0,
                159, 'l', 'e', 'a', 'v', 'e', 0,
                144, 'l', 'e', 'v', 'e', 'l', 0,
                122, 'l', 'o', 'c', 'a', 'l', 0,
                 74, 'o', 'r', 'd', 'e', 'r', 0,
                153, 'o', 'u', 't', 'e', 'r', 0,
                105, 'r', 'a', 'i', 's', 'e', 0,
                112, 'r', 'i', 'g', 'h', 't', 0,
                171, 's', 't', 'a', 'r', 't', 0,
                 75, 't', 'a', 'b', 'l', 'e', 0,
                 76, 'u', 'n', 'i', 'o', 'n', 0,
                 34, 'u', 'n', 't', 'i', 'l', 0,
                 87, 'u', 's', 'i', 'n', 'g', 0,
                 35, 'w', 'h', 'e', 'r', 'e', 0,
                 37, 'w', 'h', 'i', 'l', 'e', 0,
                152, 'w', 'r', 'i', 't', 'e', 0,
/* 555 */         0,
                  /* Keywords of length 6 */
/* 556 */       150, 'c', 'o', 'l', 'u', 'm', 'n', 0,
                 77, 'c', 'o', 'm', 'm', 'i', 't', 0,
                 38, 'c', 'r', 'e', 'a', 't', 'e', 0,
                 78, 'c', 'u', 'r', 's', 'o', 'r', 0,
                 39, 'd', 'e', 'f', 'i', 'n', 'e', 0,
                 40, 'd', 'e', 'l', 'e', 't', 'e', 0,
                 39, 'e', 'l', 's', 'e', 'i', 'f', 0,
                116, 'e', 'n', 'a', 'b', 'l', 'e', 0,
                160, 'e', 'n', 'd', 'f', 'o', 'r', 0,
                206, 'e', 'n', 'd', 'i', 'n', 'g', 0,
                 96, 'e', 's', 'c', 'a', 'p', 'e', 0,
                163, 'e', 'x', 'c', 'e', 'p', 't', 0,
                 79, 'e', 'x', 'i', 's', 't', 's', 0,
                121, 'g', 'l', 'o', 'b', 'a', 'l', 0,
                 80, 'h', 'a', 'v', 'i', 'n', 'g', 0,
                 65, 'i', 'm', 'p', 'o', 'r', 't', 0,
                 81, 'i', 'n', 's', 'e', 'r', 't', 0,
                 41, 'm', 'o', 'd', 'i', 'f', 'y', 0,
                120, 'm', 'o', 'd', 'u', 'l', 'e', 0,
                184, 'n', 'u', 'l', 'l', 'i', 'f', 0,
                203, 'o', 'f', 'f', 's', 'e', 't', 0,
                 82, 'o', 'p', 't', 'i', 'o', 'n', 0,
                 19, 'p', 'e', 'r', 'm', 'i', 't', 0,
                 83, 'p', 'u', 'b', 'l', 'i', 'c', 0,
                165, 'r', 'a', 'w', 'p', 'c', 't', 0,
                103, 'r', 'e', 'm', 'o', 'v', 'e', 0,
                216, 'r', 'e', 'n', 'a', 'm', 'e', 0,
                106, 'r', 'e', 'p', 'e', 'a', 't', 0,
                156, 'r', 'e', 's', 'u', 'l', 't', 0,
                 42, 'r', 'e', 't', 'u', 'r', 'n', 0,
                101, 'r', 'e', 'v', 'o', 'k', 'e', 0,
                190, 'r', 'o', 'l', 'l', 'u', 'p', 0,
                140, 's', 'c', 'h', 'e', 'm', 'a', 0,
                201, 's', 'c', 'r', 'o', 'l', 'l', 0,
                 84, 's', 'e', 'l', 'e', 'c', 't', 0,
                 43, 'u', 'n', 'i', 'q', 'u', 'e', 0,
                 62, 'u', 'p', 'd', 'a', 't', 'e', 0,
                 85, 'v', 'a', 'l', 'u', 'e', 's', 0,
/* 860 */         0,
                  /* Keywords of length 7 */
/* 861 */        86, 'b', 'e', 't', 'w', 'e', 'e', 'n', 0,
                126, 'c', 'a', 's', 'c', 'a', 'd', 'e', 0,
                186, 'c', 'o', 'l', 'l', 'a', 't', 'e', 0,
                118, 'c', 'o', 'm', 'm', 'e', 'n', 't', 0,
                 88, 'c', 'u', 'r', 'r', 'e', 'n', 't', 0,
                172, 'c', 'u', 'r', 'r', 'v', 'a', 'l', 0,
                 44, 'd', 'e', 'c', 'l', 'a', 'r', 'e', 0,
                129, 'd', 'e', 'f', 'a', 'u', 'l', 't', 0,
                117, 'd', 'i', 's', 'a', 'b', 'l', 'e', 0,
                217, 'e', 'n', 'c', 'r', 'y', 'p', 't', 0,
                 45, 'e', 'n', 'd', 'l', 'o', 'o', 'p', 0,
                 46, 'e', 'x', 'e', 'c', 'u', 't', 'e', 0,
                130, 'f', 'o', 'r', 'e', 'i', 'g', 'n', 0,
                 47, 'm', 'e', 's', 's', 'a', 'g', 'e', 0,
                113, 'n', 'a', 't', 'u', 'r', 'a', 'l', 0,
                173, 'n', 'e', 'x', 't', 'v', 'a', 'l', 0,
                174, 'n', 'o', 'c', 'a', 'c', 'h', 'e', 0,
                175, 'n', 'o', 'c', 'y', 'c', 'l', 'e', 0,
                176, 'n', 'o', 'o', 'r', 'd', 'e', 'r', 0,
                 89, 'p', 'r', 'e', 'p', 'a', 'r', 'e', 0,
                131, 'p', 'r', 'i', 'm', 'a', 'r', 'y', 0,
                177, 'r', 'e', 's', 't', 'a', 'r', 't', 0,
                125, 's', 'e', 's', 's', 'i', 'o', 'n', 0,
                207, 's', 'i', 'm', 'i', 'l', 'a', 'r', 0,
                214, 'u', 'n', 'k', 'n', 'o', 'w', 'n', 0,
                192, 'w', 'i', 't', 'h', 'o', 'u', 't', 0,
/* 1095 */        0,
                  /* Keywords of length 8 */
/* 1096 */      115, 'c', 'a', 'l', 'l', 'p', 'r', 'o', 'c', 0,
                185, 'c', 'o', 'a', 'l', 'e', 's', 'c', 'e', 0,
                 90, 'c', 'o', 'n', 't', 'i', 'n', 'u', 'e', 0,
                 91, 'd', 'e', 's', 'c', 'r', 'i', 'b', 'e', 0,
                 92, 'd', 'i', 's', 't', 'i', 'n', 'c', 't', 0,
                 48, 'e', 'n', 'd', 'w', 'h', 'i', 'l', 'e', 0,
                191, 'g', 'r', 'o', 'u', 'p', 'i', 'n', 'g', 0,
                210, 'i', 'd', 'e', 'n', 't', 'i', 't', 'y', 0,
                200, 'i', 'n', 't', 'e', 'r', 'v', 'a', 'l', 0,
                178, 'm', 'a', 'x', 'v', 'a', 'l', 'u', 'e', 0,
                179, 'm', 'i', 'n', 'v', 'a', 'l', 'u', 'e', 0,
                123, 'p', 'r', 'e', 's', 'e', 'r', 'v', 'e', 0,
                107, 'r', 'e', 'g', 'i', 's', 't', 'e', 'r', 0,
                 50, 'r', 'e', 'l', 'o', 'c', 'a', 't', 'e', 0,
                106, 'r', 'e', 'p', 'e', 'a', 't', 'e', 'd', 0,
                127, 'r', 'e', 's', 't', 'r', 'i', 'c', 't', 0,
                 93, 'r', 'o', 'l', 'l', 'b', 'a', 'c', 'k', 0,
/* 1266 */        0,
                  /* Keywords of length 9 */
/* 1267 */      204, 'b', 'e', 'g', 'i', 'n', 'n', 'i', 'n', 'g', 0,
                145, 'c', 'o', 'm', 'm', 'i', 't', 't', 'e', 'd', 0,
                141, 'c', 'o', 'p', 'y', '_', 'f', 'r', 'o', 'm', 0,
                142, 'c', 'o', 'p', 'y', '_', 'i', 'n', 't', 'o', 0,
                158, 'e', 'n', 'd', 'r', 'e', 'p', 'e', 'a', 't', 0,
                128, 'e', 'x', 'c', 'l', 'u', 'd', 'i', 'n', 'g', 0,
                208, 'g', 'e', 'n', 'e', 'r', 'a', 't', 'e', 'd', 0,
                 94, 'i', 'm', 'm', 'e', 'd', 'i', 'a', 't', 'e', 0,
                180, 'i', 'n', 'c', 'r', 'e', 'm', 'e', 'n', 't', 0,
                 22, 'i', 'n', 't', 'e', 'g', 'r', 'i', 't', 'y', 0,
                164, 'i', 'n', 't', 'e', 'r', 's', 'e', 'c', 't', 0,
                146, 'i', 's', 'o', 'l', 'a', 't', 'i', 'o', 'n', 0,
                 49, 'p', 'r', 'o', 'c', 'e', 'd', 'u', 'r', 'e', 0,
                 53, 's', 'a', 'v', 'e', 'p', 'o', 'i', 'n', 't', 0,
                215, 's', 'i', 'n', 'g', 'l', 'e', 't', 'o', 'n', 0,
                161, 's', 'u', 'b', 's', 't', 'r', 'i', 'n', 'g', 0,
                187, 's', 'y', 'm', 'm', 'e', 't', 'r', 'i', 'c', 0,
                119, 't', 'e', 'm', 'p', 'o', 'r', 'a', 'r', 'y', 0,
/* 1465 */        0,
                  /* Keywords of length 10 */
/* 1466 */      188, 'a', 's', 'y', 'm', 'm', 'e', 't', 'r', 'i', 'c', 0,
                134, 'c', 'o', 'n', 's', 't', 'r', 'a', 'i', 'n', 't', 0,
                205, 'c', 'o', 'n', 't', 'a', 'i', 'n', 'i', 'n', 'g', 0,
                198, 'l', 'o', 'c', 'a', 'l', '_', 't', 'i', 'm', 'e', 0,
                181, 'n', 'o', 'm', 'a', 'x', 'v', 'a', 'l', 'u', 'e', 0,
                182, 'n', 'o', 'm', 'i', 'n', 'v', 'a', 'l', 'u', 'e', 0,
                211, 'o', 'v', 'e', 'r', 'r', 'i', 'd', 'i', 'n', 'g', 0,
                 95, 'p', 'r', 'i', 'v', 'i', 'l', 'e', 'g', 'e', 's', 0,
                133, 'r', 'e', 'f', 'e', 'r', 'e', 'n', 'c', 'e', 's', 0,
                147, 'r', 'e', 'p', 'e', 'a', 't', 'a', 'b', 'l', 'e', 0,
/* 1586 */        0,
                  /* Keywords of length 11 */
/* 1587 */      104,'r','e','f','e','r','e','n','c','i','n','g',0,
                137,'s','y','s','t','e','m','_','u','s','e','r',0,
                148,'u','n','c','o','m','m','i','t','t','e','d',0,
/* 1626 */        0,
                  /* Keywords of length 12 */
/* 1627 */      195,'c','u','r','r','e','n','t','_','d','a','t','e',0,
                196,'c','u','r','r','e','n','t','_','t','i','m','e',0,
                135,'c','u','r','r','e','n','t','_','u','s','e','r',0,
                138,'i','n','i','t','i','a','l','_','u','s','e','r',0,
                149,'s','e','r','i','a','l','i','z','a','b','l','e',0,
                139,'s','e','s','s','i','o','n','_','u','s','e','r',0,
/* 1711 */        0,
                  /* Keywords of length 13 */
/* 1712 */       98,'a','u','t','h','o','r','i','z','a','t','i','o','n',0,
/* 1727 */        0,
                  /* Leywords of length 14 */
/* 1728 */      209,'a','u','t','o','_','i','n','c','r','e','m','e','n','t',0,
/* 1744 */        0,
                  /* Keywords of length 15 */
/* 1745 */      199,'l','o','c','a','l','_','t','i','m','e','s','t','a','m','p',0,
/* 1762 */        0,
                  /* Keywords of length 17 */
/* 1763 */      197,'c','u','r','r','e','n','t','_','t','i','m','e','s','t','a',
                    'm','p',0,
                154,'s','y','s','t','e','m','_','m','a','i','n','t','a','i','n',
                    'e','d',0,
/* 1801 */        0
                  };

/*
** This table contains pointers into the above Psl_keystr[] array.  The
** n'th pointer points to the keywords of length n in Psl_keystr (where n
** begins at zero).
*/

static const u_char *Key_index[] = {
		&Key_string[   0],	/* Keywords of length  0 */
		&Key_string[   0],	/* Keywords of length  1 */
		&Key_string[   1],	/* Keywords of length  2 */
		&Key_string[  50],	/* Keywords of length  3 */
		&Key_string[ 136],	/* Keywords of length  4 */
		&Key_string[ 317],	/* Keywords of length  5 */
		&Key_string[ 556],	/* Keywords of length  6 */
		&Key_string[ 861],	/* Keywords of length  7 */
		&Key_string[1096],	/* Keywords of length  8 */
		&Key_string[1267],	/* Keywords of length  9 */
		&Key_string[1466],	/* Keywords of length 10 */
		&Key_string[1587],	/* Keywords of length 11 */
		&Key_string[1627],	/* Keywords of length 12 */
		&Key_string[1712],	/* Keywords of length 13 */
		&Key_string[1728],	/* Keywords of length 14 */
		&Key_string[1745],	/* Keywords of length 15 */
		&Key_string[   0],	/* Keywords of length 16 */
		&Key_string[1763],	/* Keywords of length 17 */
		&Key_string[   0],	/* Keywords of length 18 */
		&Key_string[   0],	/* Keywords of length 19 */
		&Key_string[   0],	/* Keywords of length 20 */
		&Key_string[   0],	/* Keywords of length 21 */
		&Key_string[   0],	/* Keywords of length 22 */
		&Key_string[   0],	/* Keywords of length 23 */
		&Key_string[   0],	/* Keywords of length 24 */
		&Key_string[   0],	/* Keywords of length 25 */
		&Key_string[   0],	/* Keywords of length 26 */
		&Key_string[   0],	/* Keywords of length 27 */
		&Key_string[   0],	/* Keywords of length 28 */
		&Key_string[   0],	/* Keywords of length 29 */
		&Key_string[   0],	/* Keywords of length 30 */
		&Key_string[   0],	/* Keywords of length 31 */
		&Key_string[   0],	/* Keywords of length 32 */
		};
