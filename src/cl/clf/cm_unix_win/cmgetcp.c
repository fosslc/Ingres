/*
** Copyright (c) 2001, 2005 Ingres Corporation
**
*/
# include <compat.h>
# include <gl.h>
# include <clsecret.h>
# include <systypes.h>
# include <cs.h>
# include <cm.h>
# include <er.h>
# include <nm.h>
# include <st.h>
# include <lo.h>
# include <si.h>

# include <locale.h>
# ifdef NT_GENERIC
# include <windows.h>
# include <winnls.h>
# else
# include <langinfo.h>
# endif /* NT_GENERIC */

# ifdef xCL_006_FCNTL_H_EXISTS
# include       <fcntl.h>
# endif /* xCL_006_FCNTL_H_EXISTS */
# include <ernames.h>

#ifdef  VMS
#include <fab.h>
#include <rab.h>
#include <rmsdef.h>
#endif

#include <errno.h>

/**
** Name: CMGETCP.c - Obtains encoding names for common Linux/Solaris 
**		     locales.
**
** Description:
**      This file defines a table Localemap, which contains the locales 
**	and the code pages associated with them.
**	Typical use for this file is to identify the codepage for the 
**	unix locale, using CM function call CM_getcharset.
**
** History:
**
**	06-Jul-2005 (gupsh01) 
**	    Added.
**	13-Jul-2005 (gupsh01)
**	    Fixed this routine. This gives a segv on Suse Linux.
**	13-Jul-2005 (gupsh01)
**	    langinfo.h is not found on Windows. Do not include 
**	    langinfo.h on windows.
**	6-Apr-2007 (kschendel) b122041
**	    Compiler warning fixup, char[] isn't pointer.
*/

typedef struct _CM_LOC_MAP 
{
    char locale_value[CM_MAXLOCALE];
    char locale_encoding[CM_MAXLOCALE]; 
}    CM_LOC_MAP;

static CM_LOC_MAP Localemap[]  = 
{
    { "C",	"ANSI_X3.4-1968" },
    { "POSIX",	"ANSI_X3.4-1968" },
    { "zh_TW",	"BIG5" },
    { "zh_HK",	"BIG5-HKSCS" },
    { "be_BY",	"CP1251" },
    { "bg_BG",	"CP1251" },
    { "yi_US",	"CP1255" },
    { "ja_JP",	"EUC-JP" },
    { "japanese",	"EUC-JP" },
    { "ko_KR",	"EUC-KR" },
    { "korean",	"EUC-KR" },
    { "zh_CN",	"GB2312" },
    { "ka_GE",	"GEORGIAN-PS" },
    { "af_ZA",	"ISO-8859-1" },
    { "bokmal",	"ISO-8859-1" },
    { "br_FR",	"ISO-8859-1" },
    { "ca_ES",	"ISO-8859-1" },
    { "catalan",	"ISO-8859-1" },
    { "da_DK",	"ISO-8859-1" },
    { "danish",	"ISO-8859-1" },
    { "dansk",	"ISO-8859-1" },
    { "de_AT",	"ISO-8859-1" },
    { "de_BE",	"ISO-8859-1" },
    { "de_CH",	"ISO-8859-1" },
    { "de_DE",	"ISO-8859-1" },
    { "de_LU",	"ISO-8859-1" },
    { "deutsch",	"ISO-8859-1" },
    { "dutch",	"ISO-8859-1" },
    { "eesti",	"ISO-8859-1" },
    { "en_AU",	"ISO-8859-1" },
    { "en_BW",	"ISO-8859-1" },
    { "en_CA",	"ISO-8859-1" },
    { "en_DK",	"ISO-8859-1" },
    { "en_GB",	"ISO-8859-1" },
    { "en_HK",	"ISO-8859-1" },
    { "en_IE",	"ISO-8859-1" },
    { "en_NZ",	"ISO-8859-1" },
    { "en_PH",	"ISO-8859-1" },
    { "en_SG",	"ISO-8859-1" },
    { "en_US",	"ISO-8859-1" },
    { "en_ZA",	"ISO-8859-1" },
    { "en_ZW",	"ISO-8859-1" },
    { "es_AR",	"ISO-8859-1" },
    { "es_BO",	"ISO-8859-1" },
    { "es_CL",	"ISO-8859-1" },
    { "es_CO",	"ISO-8859-1" },
    { "es_CR",	"ISO-8859-1" },
    { "es_DO",	"ISO-8859-1" },
    { "es_EC",	"ISO-8859-1" },
    { "es_ES",	"ISO-8859-1" },
    { "es_GT",	"ISO-8859-1" },
    { "es_HN",	"ISO-8859-1" },
    { "es_MX",	"ISO-8859-1" },
    { "es_NI",	"ISO-8859-1" },
    { "es_PA",	"ISO-8859-1" },
    { "es_PE",	"ISO-8859-1" },
    { "es_PR",	"ISO-8859-1" },
    { "es_PY",	"ISO-8859-1" },
    { "es_SV",	"ISO-8859-1" },
    { "estonian",	"ISO-8859-1" },
    { "es_US",	"ISO-8859-1" },
    { "es_UY",	"ISO-8859-1" },
    { "es_VE",	"ISO-8859-1" },
    { "et_EE",	"ISO-8859-1" },
    { "eu_ES",	"ISO-8859-1" },
    { "fi_FI",	"ISO-8859-1" },
    { "finnish",	"ISO-8859-1" },
    { "fo_FO",	"ISO-8859-1" },
    { "fr_BE",	"ISO-8859-1" },
    { "fr_CA",	"ISO-8859-1" },
    { "fr_CH",	"ISO-8859-1" },
    { "french",	"ISO-8859-1" },
    { "fr_FR",	"ISO-8859-1" },
    { "fr_LU",	"ISO-8859-1" },
    { "ga_IE",	"ISO-8859-1" },
    { "galego",	"ISO-8859-1" },
    { "galician",	"ISO-8859-1" },
    { "german",	"ISO-8859-1" },
    { "gl_ES",	"ISO-8859-1" },
    { "gv_GB",	"ISO-8859-1" },
    { "icelandic",	"ISO-8859-1" },
    { "id_ID",	"ISO-8859-1" },
    { "is_IS",	"ISO-8859-1" },
    { "italian",	"ISO-8859-1" },
    { "it_CH",	"ISO-8859-1" },
    { "it_IT",	"ISO-8859-1" },
    { "kl_GL",	"ISO-8859-1" },
    { "kw_GB",	"ISO-8859-1" },
    { "ms_MY",	"ISO-8859-1" },
    { "nb_NO",	"ISO-8859-1" },
    { "nl_BE",	"ISO-8859-1" },
    { "nl_NL",	"ISO-8859-1" },
    { "nn_NO",	"ISO-8859-1" },
    { "no_NO",	"ISO-8859-1" },
    { "norwegian",	"ISO-8859-1" },
    { "nynorsk",	"ISO-8859-1" },
    { "oc_FR",	"ISO-8859-1" },
    { "portuguese",	"ISO-8859-1" },
    { "pt_BR",	"ISO-8859-1" },
    { "pt_PT",	"ISO-8859-1" },
    { "spanish",	"ISO-8859-1" },
    { "sq_AL",	"ISO-8859-1" },
    { "sv_FI",	"ISO-8859-1" },
    { "sv_SE",	"ISO-8859-1" },
    { "swedish",	"ISO-8859-1" },
    { "tl_PH",	"ISO-8859-1" },
    { "uz_UZ",	"ISO-8859-1" },
    { "wa_BE",	"ISO-8859-1" },
    { "lithuanian",	"ISO-8859-13" },
    { "lt_LT",	"ISO-8859-13" },
    { "lv_LV",	"ISO-8859-13" },
    { "mi_NZ",	"ISO-8859-13" },
    { "cy_GB",	"ISO-8859-14" },
    { "an_ES",	"ISO-8859-15" },
    { "ca_ES@euro",	"ISO-8859-15" },
    { "de_AT@euro",	"ISO-8859-15" },
    { "de_BE@euro",	"ISO-8859-15" },
    { "de_DE@euro",	"ISO-8859-15" },
    { "de_LU@euro",	"ISO-8859-15" },
    { "en_IE@euro",	"ISO-8859-15" },
    { "es_ES@euro",	"ISO-8859-15" },
    { "eu_ES@euro",	"ISO-8859-15" },
    { "fi_FI@euro",	"ISO-8859-15" },
    { "fr_BE@euro",	"ISO-8859-15" },
    { "fr_FR@euro",	"ISO-8859-15" },
    { "fr_LU@euro",	"ISO-8859-15" },
    { "ga_IE@euro",	"ISO-8859-15" },
    { "gl_ES@euro",	"ISO-8859-15" },
    { "it_IT@euro",	"ISO-8859-15" },
    { "nl_BE@euro",	"ISO-8859-15" },
    { "nl_NL@euro",	"ISO-8859-15" },
    { "pt_PT@euro",	"ISO-8859-15" },
    { "sv_FI@euro",	"ISO-8859-15" },
    { "wa_BE@euro",	"ISO-8859-15" },
    { "bs_BA",	"ISO-8859-2" },
    { "croatian",	"ISO-8859-2" },
    { "cs_CZ",	"ISO-8859-2" },
    { "czech",	"ISO-8859-2" },
    { "hr_HR",	"ISO-8859-2" },
    { "hrvatski",	"ISO-8859-2" },
    { "hu_HU",	"ISO-8859-2" },
    { "hungarian",	"ISO-8859-2" },
    { "pl_PL",	"ISO-8859-2" },
    { "polish",	"ISO-8859-2" },
    { "romanian",	"ISO-8859-2" },
    { "ro_RO",	"ISO-8859-2" },
    { "sk_SK",	"ISO-8859-2" },
    { "slovak",	"ISO-8859-2" },
    { "slovene",	"ISO-8859-2" },
    { "slovenian",	"ISO-8859-2" },
    { "sl_SI",	"ISO-8859-2" },
    { "sr_YU",	"ISO-8859-2" },
    { "mt_MT",	"ISO-8859-3" },
    { "mk_MK",	"ISO-8859-5" },
    { "ru_RU",	"ISO-8859-5" },
    { "russian",	"ISO-8859-5" },
    { "sr_YU@cyrillic",	"ISO-8859-5" },
    { "ar_AE",	"ISO-8859-6" },
    { "ar_BH",	"ISO-8859-6" },
    { "ar_DZ",	"ISO-8859-6" },
    { "ar_EG",	"ISO-8859-6" },
    { "ar_IQ",	"ISO-8859-6" },
    { "ar_JO",	"ISO-8859-6" },
    { "ar_KW",	"ISO-8859-6" },
    { "ar_LB",	"ISO-8859-6" },
    { "ar_LY",	"ISO-8859-6" },
    { "ar_MA",	"ISO-8859-6" },
    { "ar_OM",	"ISO-8859-6" },
    { "ar_QA",	"ISO-8859-6" },
    { "ar_SA",	"ISO-8859-6" },
    { "ar_SD",	"ISO-8859-6" },
    { "ar_SY",	"ISO-8859-6" },
    { "ar_TN",	"ISO-8859-6" },
    { "ar_YE",	"ISO-8859-6" },
    { "el_GR",	"ISO-8859-7" },
    { "greek",	"ISO-8859-7" },
    { "hebrew",	"ISO-8859-8" },
    { "he_IL",	"ISO-8859-8" },
    { "iw_IL",	"ISO-8859-8" },
    { "tr_TR",	"ISO-8859-9" },
    { "turkish",	"ISO-8859-9" },
    { "tg_TJ",	"KOI8-T" },
    { "ru_UA",	"KOI8-U" },
    { "uk_UA",	"KOI8-U" },
    { "thai",	"TIS-620" },
    { "th_TH",	"TIS-620" },
    { "ar_IN",	"UTF-8" },
    { "en_IN",	"UTF-8" },
    { "fa_IR",	"UTF-8" },
    { "hi_IN",	"UTF-8" },
    { "lo_LA",	"UTF-8" },
    { "mr_IN",	"UTF-8" },
    { "se_NO",	"UTF-8" },
    { "ta_IN",	"UTF-8" },
    { "te_IN",	"UTF-8" },
    { "ur_PK",	"UTF-8" },
    { "vi_VN",	"UTF-8" },
    { "zh_TW.big5",	"BIG5" },
    { "zh_HK.big5hkscs",	"BIG5-HKSCS" },
    { "be_BY.cp1251",	"CP1251" },
    { "bg_BG.cp1251",	"CP1251" },
    { "yi_US.cp1255",	"CP1255" },
    { "ja_JP.eucjp",	"EUC-JP" },
    { "ja_JP.ujis",	"EUC-JP" },
    { "japanese.euc",	"EUC-JP" },
    { "ko_KR.euckr",	"EUC-KR" },
    { "korean.euc",	"EUC-KR" },
    { "zh_TW.euctw",	"EUC-TW" },
    { "zh_CN.gb18030",	"GB18030" },
    { "zh_CN.gb2312",	"GB2312" },
    { "zh_CN.gbk",	"GBK" },
    { "ka_GE.georgianps",	"GEORGIAN-PS" },
    { "af_ZA.iso88591",	"ISO-8859-1" },
    { "br_FR.iso88591",	"ISO-8859-1" },
    { "ca_ES.iso88591",	"ISO-8859-1" },
    { "da_DK.iso88591",	"ISO-8859-1" },
    { "de_AT.iso88591",	"ISO-8859-1" },
    { "de_BE.iso88591",	"ISO-8859-1" },
    { "de_CH.iso88591",	"ISO-8859-1" },
    { "de_DE.iso88591",	"ISO-8859-1" },
    { "de_LU.iso88591",	"ISO-8859-1" },
    { "en_AU.iso88591",	"ISO-8859-1" },
    { "en_BW.iso88591",	"ISO-8859-1" },
    { "en_CA.iso88591",	"ISO-8859-1" },
    { "en_DK.iso88591",	"ISO-8859-1" },
    { "en_GB.iso88591",	"ISO-8859-1" },
    { "en_HK.iso88591",	"ISO-8859-1" },
    { "en_IE.iso88591",	"ISO-8859-1" },
    { "en_NZ.iso88591",	"ISO-8859-1" },
    { "en_PH.iso88591",	"ISO-8859-1" },
    { "en_SG.iso88591",	"ISO-8859-1" },
    { "en_US.iso88591",	"ISO-8859-1" },
    { "en_ZA.iso88591",	"ISO-8859-1" },
    { "en_ZW.iso88591",	"ISO-8859-1" },
    { "es_AR.iso88591",	"ISO-8859-1" },
    { "es_BO.iso88591",	"ISO-8859-1" },
    { "es_CL.iso88591",	"ISO-8859-1" },
    { "es_CO.iso88591",	"ISO-8859-1" },
    { "es_CR.iso88591",	"ISO-8859-1" },
    { "es_DO.iso88591",	"ISO-8859-1" },
    { "es_EC.iso88591",	"ISO-8859-1" },
    { "es_ES.iso88591",	"ISO-8859-1" },
    { "es_GT.iso88591",	"ISO-8859-1" },
    { "es_HN.iso88591",	"ISO-8859-1" },
    { "es_MX.iso88591",	"ISO-8859-1" },
    { "es_NI.iso88591",	"ISO-8859-1" },
    { "es_PA.iso88591",	"ISO-8859-1" },
    { "es_PE.iso88591",	"ISO-8859-1" },
    { "es_PR.iso88591",	"ISO-8859-1" },
    { "es_PY.iso88591",	"ISO-8859-1" },
    { "es_SV.iso88591",	"ISO-8859-1" },
    { "es_US.iso88591",	"ISO-8859-1" },
    { "es_UY.iso88591",	"ISO-8859-1" },
    { "es_VE.iso88591",	"ISO-8859-1" },
    { "et_EE.iso88591",	"ISO-8859-1" },
    { "eu_ES.iso88591",	"ISO-8859-1" },
    { "fi_FI.iso88591",	"ISO-8859-1" },
    { "fo_FO.iso88591",	"ISO-8859-1" },
    { "fr_BE.iso88591",	"ISO-8859-1" },
    { "fr_CA.iso88591",	"ISO-8859-1" },
    { "fr_CH.iso88591",	"ISO-8859-1" },
    { "fr_FR.iso88591",	"ISO-8859-1" },
    { "fr_LU.iso88591",	"ISO-8859-1" },
    { "ga_IE.iso88591",	"ISO-8859-1" },
    { "gl_ES.iso88591",	"ISO-8859-1" },
    { "gv_GB.iso88591",	"ISO-8859-1" },
    { "id_ID.iso88591",	"ISO-8859-1" },
    { "is_IS.iso88591",	"ISO-8859-1" },
    { "it_CH.iso88591",	"ISO-8859-1" },
    { "it_IT.iso88591",	"ISO-8859-1" },
    { "kl_GL.iso88591",	"ISO-8859-1" },
    { "kw_GB.iso88591",	"ISO-8859-1" },
    { "ms_MY.iso88591",	"ISO-8859-1" },
    { "nb_NO.iso88591",	"ISO-8859-1" },
    { "nl_BE.iso88591",	"ISO-8859-1" },
    { "nl_NL.iso88591",	"ISO-8859-1" },
    { "nn_NO.iso88591",	"ISO-8859-1" },
    { "no_NO.iso88591",	"ISO-8859-1" },
    { "oc_FR.iso88591",	"ISO-8859-1" },
    { "pt_BR.iso88591",	"ISO-8859-1" },
    { "pt_PT.iso88591",	"ISO-8859-1" },
    { "sq_AL.iso88591",	"ISO-8859-1" },
    { "sv_FI.iso88591",	"ISO-8859-1" },
    { "sv_SE.iso88591",	"ISO-8859-1" },
    { "tl_PH.iso88591",	"ISO-8859-1" },
    { "uz_UZ.iso88591",	"ISO-8859-1" },
    { "wa_BE.iso88591",	"ISO-8859-1" },
    { "lt_LT.iso885913",	"ISO-8859-13" },
    { "lv_LV.iso885913",	"ISO-8859-13" },
    { "mi_NZ.iso885913",	"ISO-8859-13" },
    { "cy_GB.iso885914",	"ISO-8859-14" },
    { "an_ES.iso885915",	"ISO-8859-15" },
    { "ca_ES.iso885915@euro",	"ISO-8859-15" },
    { "da_DK.iso885915",	"ISO-8859-15" },
    { "de_AT.iso885915@euro",	"ISO-8859-15" },
    { "de_BE.iso885915@euro",	"ISO-8859-15" },
    { "de_DE.iso885915@euro",	"ISO-8859-15" },
    { "de_LU.iso885915@euro",	"ISO-8859-15" },
    { "en_GB.iso885915",	"ISO-8859-15" },
    { "en_IE.iso885915@euro",	"ISO-8859-15" },
    { "en_US.iso885915",	"ISO-8859-15" },
    { "es_ES.iso885915@euro",	"ISO-8859-15" },
    { "eu_ES.iso885915@euro",	"ISO-8859-15" },
    { "fi_FI.iso885915@euro",	"ISO-8859-15" },
    { "fr_BE.iso885915@euro",	"ISO-8859-15" },
    { "fr_FR.iso885915@euro",	"ISO-8859-15" },
    { "fr_LU.iso885915@euro",	"ISO-8859-15" },
    { "ga_IE.iso885915@euro",	"ISO-8859-15" },
    { "gl_ES.iso885915@euro",	"ISO-8859-15" },
    { "it_IT.iso885915@euro",	"ISO-8859-15" },
    { "nl_BE.iso885915@euro",	"ISO-8859-15" },
    { "nl_NL.iso885915@euro",	"ISO-8859-15" },
    { "pt_PT.iso885915@euro",	"ISO-8859-15" },
    { "sv_FI.iso885915@euro",	"ISO-8859-15" },
    { "sv_SE.iso885915",	"ISO-8859-15" },
    { "wa_BE.iso885915@euro",	"ISO-8859-15" },
    { "bs_BA.iso88592",	"ISO-8859-2" },
    { "cs_CZ.iso88592",	"ISO-8859-2" },
    { "hr_HR.iso88592",	"ISO-8859-2" },
    { "hu_HU.iso88592",	"ISO-8859-2" },
    { "pl_PL.iso88592",	"ISO-8859-2" },
    { "ro_RO.iso88592",	"ISO-8859-2" },
    { "sk_SK.iso88592",	"ISO-8859-2" },
    { "sl_SI.iso88592",	"ISO-8859-2" },
    { "sr_YU.iso88592",	"ISO-8859-2" },
    { "mt_MT.iso88593",	"ISO-8859-3" },
    { "mk_MK.iso88595",	"ISO-8859-5" },
    { "ru_RU.iso88595",	"ISO-8859-5" },
    { "sr_YU.iso88595@cyrillic",	"ISO-8859-5" },
    { "ar_AE.iso88596",	"ISO-8859-6" },
    { "ar_BH.iso88596",	"ISO-8859-6" },
    { "ar_DZ.iso88596",	"ISO-8859-6" },
    { "ar_EG.iso88596",	"ISO-8859-6" },
    { "ar_IQ.iso88596",	"ISO-8859-6" },
    { "ar_JO.iso88596",	"ISO-8859-6" },
    { "ar_KW.iso88596",	"ISO-8859-6" },
    { "ar_LB.iso88596",	"ISO-8859-6" },
    { "ar_LY.iso88596",	"ISO-8859-6" },
    { "ar_MA.iso88596",	"ISO-8859-6" },
    { "ar_OM.iso88596",	"ISO-8859-6" },
    { "ar_QA.iso88596",	"ISO-8859-6" },
    { "ar_SA.iso88596",	"ISO-8859-6" },
    { "ar_SD.iso88596",	"ISO-8859-6" },
    { "ar_SY.iso88596",	"ISO-8859-6" },
    { "ar_TN.iso88596",	"ISO-8859-6" },
    { "ar_YE.iso88596",	"ISO-8859-6" },
    { "el_GR.iso88597",	"ISO-8859-7" },
    { "he_IL.iso88598",	"ISO-8859-8" },
    { "iw_IL.iso88598",	"ISO-8859-8" },
    { "tr_TR.iso88599",	"ISO-8859-9" },
    { "ru_RU.koi8r",	"KOI8-R" },
    { "tg_TJ.koi8t",	"KOI8-T" },
    { "ru_UA.koi8u",	"KOI8-U" },
    { "uk_UA.koi8u",	"KOI8-U" },
    { "vi_VN.tcvn",	"TCVN5712-1" },
    { "th_TH.tis620",	"TIS-620" },
    { "ar_AE.utf8",	"UTF-8" },
    { "ar_BH.utf8",	"UTF-8" },
    { "ar_DZ.utf8",	"UTF-8" },
    { "ar_EG.utf8",	"UTF-8" },
    { "ar_IN.utf8",	"UTF-8" },
    { "ar_IQ.utf8",	"UTF-8" },
    { "ar_JO.utf8",	"UTF-8" },
    { "ar_KW.utf8",	"UTF-8" },
    { "ar_LB.utf8",	"UTF-8" },
    { "ar_LY.utf8",	"UTF-8" },
    { "ar_MA.utf8",	"UTF-8" },
    { "ar_OM.utf8",	"UTF-8" },
    { "ar_QA.utf8",	"UTF-8" },
    { "ar_SA.utf8",	"UTF-8" },
    { "ar_SD.utf8",	"UTF-8" },
    { "ar_SY.utf8",	"UTF-8" },
    { "ar_TN.utf8",	"UTF-8" },
    { "ar_YE.utf8",	"UTF-8" },
    { "be_BY.utf8",	"UTF-8" },
    { "bg_BG.utf8",	"UTF-8" },
    { "ca_ES.utf8",	"UTF-8" },
    { "ca_ES.utf8@euro",	"UTF-8" },
    { "cs_CZ.utf8",	"UTF-8" },
    { "cy_GB.utf8",	"UTF-8" },
    { "da_DK.utf8",	"UTF-8" },
    { "de_AT.utf8",	"UTF-8" },
    { "de_AT.utf8@euro",	"UTF-8" },
    { "de_BE.utf8",	"UTF-8" },
    { "de_BE.utf8@euro",	"UTF-8" },
    { "de_CH.utf8",	"UTF-8" },
    { "de_DE.utf8",	"UTF-8" },
    { "de_DE.utf8@euro",	"UTF-8" },
    { "de_LU.utf8",	"UTF-8" },
    { "de_LU.utf8@euro",	"UTF-8" },
    { "el_GR.utf8",	"UTF-8" },
    { "en_AU.utf8",	"UTF-8" },
    { "en_BW.utf8",	"UTF-8" },
    { "en_CA.utf8",	"UTF-8" },
    { "en_DK.utf8",	"UTF-8" },
    { "en_GB.utf8",	"UTF-8" },
    { "en_HK.utf8",	"UTF-8" },
    { "en_IE.utf8",	"UTF-8" },
    { "en_IE.utf8@euro",	"UTF-8" },
    { "en_IN.utf8",	"UTF-8" },
    { "en_NZ.utf8",	"UTF-8" },
    { "en_PH.utf8",	"UTF-8" },
    { "en_SG.utf8",	"UTF-8" },
    { "en_US.utf8",	"UTF-8" },
    { "en_ZA.utf8",	"UTF-8" },
    { "en_ZW.utf8",	"UTF-8" },
    { "es_AR.utf8",	"UTF-8" },
    { "es_BO.utf8",	"UTF-8" },
    { "es_CL.utf8",	"UTF-8" },
    { "es_CO.utf8",	"UTF-8" },
    { "es_CR.utf8",	"UTF-8" },
    { "es_DO.utf8",	"UTF-8" },
    { "es_EC.utf8",	"UTF-8" },
    { "es_ES.utf8",	"UTF-8" },
    { "es_ES.utf8@euro",	"UTF-8" },
    { "es_GT.utf8",	"UTF-8" },
    { "es_HN.utf8",	"UTF-8" },
    { "es_MX.utf8",	"UTF-8" },
    { "es_NI.utf8",	"UTF-8" },
    { "es_PA.utf8",	"UTF-8" },
    { "es_PE.utf8",	"UTF-8" },
    { "es_PR.utf8",	"UTF-8" },
    { "es_PY.utf8",	"UTF-8" },
    { "es_SV.utf8",	"UTF-8" },
    { "es_US.utf8",	"UTF-8" },
    { "es_UY.utf8",	"UTF-8" },
    { "es_VE.utf8",	"UTF-8" },
    { "et_EE.utf8",	"UTF-8" },
    { "eu_ES.utf8",	"UTF-8" },
    { "eu_ES.utf8@euro",	"UTF-8" },
    { "fa_IR.utf8",	"UTF-8" },
    { "fi_FI.utf8",	"UTF-8" },
    { "fi_FI.utf8@euro",	"UTF-8" },
    { "fo_FO.utf8",	"UTF-8" },
    { "fr_BE.utf8",	"UTF-8" },
    { "fr_BE.utf8@euro",	"UTF-8" },
    { "fr_CA.utf8",	"UTF-8" },
    { "fr_CH.utf8",	"UTF-8" },
    { "fr_FR.utf8",	"UTF-8" },
    { "fr_FR.utf8@euro",	"UTF-8" },
    { "fr_LU.utf8",	"UTF-8" },
    { "fr_LU.utf8@euro",	"UTF-8" },
    { "ga_IE.utf8",	"UTF-8" },
    { "ga_IE.utf8@euro",	"UTF-8" },
    { "gl_ES.utf8",	"UTF-8" },
    { "gl_ES.utf8@euro",	"UTF-8" },
    { "gv_GB.utf8",	"UTF-8" },
    { "he_IL.utf8",	"UTF-8" },
    { "hi_IN.utf8",	"UTF-8" },
    { "hr_HR.utf8",	"UTF-8" },
    { "hu_HU.utf8",	"UTF-8" },
    { "id_ID.utf8",	"UTF-8" },
    { "is_IS.utf8",	"UTF-8" },
    { "it_CH.utf8",	"UTF-8" },
    { "it_IT.utf8",	"UTF-8" },
    { "it_IT.utf8@euro",	"UTF-8" },
    { "iw_IL.utf8",	"UTF-8" },
    { "ja_JP.utf8",	"UTF-8" },
    { "kl_GL.utf8",	"UTF-8" },
    { "ko_KR.utf8",	"UTF-8" },
    { "kw_GB.utf8",	"UTF-8" },
    { "lo_LA.utf8",	"UTF-8" },
    { "lt_LT.utf8",	"UTF-8" },
    { "lv_LV.utf8",	"UTF-8" },
    { "mk_MK.utf8",	"UTF-8" },
    { "mr_IN.utf8",	"UTF-8" },
    { "ms_MY.utf8",	"UTF-8" },
    { "mt_MT.utf8",	"UTF-8" },
    { "nl_BE.utf8",	"UTF-8" },
    { "nl_BE.utf8@euro",	"UTF-8" },
    { "nl_NL.utf8",	"UTF-8" },
    { "nl_NL.utf8@euro",	"UTF-8" },
    { "nn_NO.utf8",	"UTF-8" },
    { "no_NO.utf8",	"UTF-8" },
    { "pl_PL.utf8",	"UTF-8" },
    { "pt_BR.utf8",	"UTF-8" },
    { "pt_PT.utf8",	"UTF-8" },
    { "pt_PT.utf8@euro",	"UTF-8" },
    { "ro_RO.utf8",	"UTF-8" },
    { "ru_RU.utf8",	"UTF-8" },
    { "ru_UA.utf8",	"UTF-8" },
    { "se_NO.utf8",	"UTF-8" },
    { "sk_SK.utf8",	"UTF-8" },
    { "sl_SI.utf8",	"UTF-8" },
    { "sq_AL.utf8",	"UTF-8" },
    { "sr_YU.utf8",	"UTF-8" },
    { "sr_YU.utf8@cyrillic",	"UTF-8" },
    { "sv_FI.utf8",	"UTF-8" },
    { "sv_FI.utf8@euro",	"UTF-8" },
    { "sv_SE.utf8",	"UTF-8" },
    { "ta_IN.utf8",	"UTF-8" },
    { "te_IN.utf8",	"UTF-8" },
    { "th_TH.utf8",	"UTF-8" },
    { "tr_TR.utf8",	"UTF-8" },
    { "uk_UA.utf8",	"UTF-8" },
    { "ur_PK.utf8",	"UTF-8" },
    { "vi_VN.utf8",	"UTF-8" },
    { "zh_CN.utf8",	"UTF-8" },
    { "zh_HK.utf8",	"UTF-8" },
    { "zh_TW.utf8",	"UTF-8" },
    { {'\0'},  {'\0'} },
};

/* Name: CM_getcharset- Get Platforms default character set
**
** Input: 
**   pcs - Pointer to hold the value of platform characterset. The
**	   calling funtion allocates the memory for this variable. 
** Output:  
**   pcs - The platform character set. 
** Results:
**   returns OK if successful, FAIL if error.
** History:
**
**	23-Jan-2004 (gupsh01)
**	    Created.
**	22-Oct-2004 (gupsh01)
**	    Fixed to correctly generate windows code page.
**	6-Jul-2005 (gupsh01)
**	    Modify the code to return the correct character set name
**	    on Unix platforms. Moved here from cmafile.
**	13-Jul-2005 (gupsh01)
**	    Fixed this routine it was giving a segv on Suse.
*/
STATUS
CM_getcharset(char *pcs)
{
   char *charset = NULL;
   bool	found = FALSE;
   char	*charsetstart;
   char	*charsetend;
   i4	charsetlen = 0;
   i4	i = 0;
   char	*codeset;

# if defined(NT_GENERIC)
    u_i2	codepage;
# endif

  if (!pcs)
    return FAIL;

# if defined(NT_GENERIC)
    /* Obtain ANSI code page specifier for the system. */
    codepage = GetACP(); 
    if (codepage != 0)
    { 
      STprintf (pcs , "windows-%d", codepage);
      return OK;
    } 
    else 
      return FAIL;
# else          
    if (charset = setlocale (LC_CTYPE,""))
    { 
        int tlen = 0;
	char workbuffer[CM_MAXLOCALE];
	char *sptr;
	char *bufptr;
	i4   slen;

	slen = STlength(charset);
	sptr = charset;
	bufptr = workbuffer;

	/* normalize input ignore all '-' */
	for (;slen;slen--)
	{
	  if (*sptr != '-')
	    *bufptr++ = *sptr++;
	  else
	    sptr++;
	}
	*bufptr = '\0';
	tlen = sizeof(Localemap)/sizeof(CM_LOC_MAP) - 1;
      for ( i = 0; ((i < (tlen - 1)) && (Localemap[i].locale_value != NULL)); i++)
      {
	if (STbcompare ( workbuffer, 0, Localemap[i].locale_value, 0, 1) == 0)
	{
	  found = TRUE;
	  break;
	}
      }
	
      if (found)
      {
        STlcopy (Localemap[i].locale_encoding, pcs, CM_MAXLOCALE);
      }
      else
      {
      charsetlen = STlength(charset);
	/* Try obtaining charset from the locale value if present */
	if ((charsetstart = STindex(charset, ".", 0)) != NULL)
	{
	    charsetstart++;
	  if ( (charsetend = STindex(charset, "@", 0)) == NULL )
      	    charsetend = charset + charsetlen; 

          if (charsetend != charsetstart)
	  { 
	    STlcopy (charsetstart, pcs, (charsetend - charsetstart)); 
            return OK;
	  } 
	}

	/* Still not found try to obtain the results from the NL_LANGSET */ 
	if ((codeset = nl_langinfo(CODESET)) != NULL)
	{
      	  STcopy(codeset, pcs);
          return OK;
	}

	/* 
	** Exhausted all possibilities, failed to identify characterset.
	** Typically calling module will set characterset to default.
	*/
	return FAIL;	
      }
      return OK;
    }
    else 
      return FAIL;
# endif   /* NT_GENERIC */
}
