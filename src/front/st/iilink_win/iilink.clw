; CLW file contains information for the MFC ClassWizard

[General Info]
Version=1
LastClass=MainPage
LastTemplate=CPropertyPage
NewFileInclude1=#include "stdafx.h"
NewFileInclude2=#include "iilink.h"

ClassCount=7
Class1=CIilinkApp
Class2=MainPage

ResourceCount=9
Resource2=IDD_FAIL_DIALOG
Resource1=IDR_MAINFRAME
Class3=PropSheet
Resource3=IDD_WAIT_DIALOG
Class4=FinalPage
Resource4=IDD_SPLASH
Class5=UserUdts
Resource5=IDD_FINAL_PAGE
Resource6=IDD_CHOOSEDIRS
Resource7=IDD_USERUDT_PAGE
Class6=FailDlg
Resource8=IDD_MAIN_PAGE
Class7=COptionsPage
Resource9=IDD_OPTIONS_PAGE

[CLS:CIilinkApp]
Type=0
HeaderFile=iilink.h
ImplementationFile=iilink.cpp
Filter=N
LastObject=CIilinkApp

[DLG:IDD_MAIN_PAGE]
Type=1
Class=MainPage
ControlCount=7
Control1=IDC_MAIN_DESC,static,1342308352
Control2=IDC_CHECK_BACKUP,button,1342242819
Control3=IDC_EDIT_EXTENSION,edit,1350631552
Control4=IDC_IMAGE,static,1342308352
Control5=IDC_BACKUP_DESC,static,1342308352
Control6=IDC_CHECK_RESTORE,button,1342242819
Control7=IDC_RESTORE_TXT,static,1342308352

[DLG:IDD_SPLASH]
Type=1
Class=?
ControlCount=1
Control1=IDC_IMAGE,static,1342308352

[CLS:PropSheet]
Type=0
HeaderFile=PropSheet.h
ImplementationFile=PropSheet.cpp
BaseClass=CPropertySheet
Filter=W
LastObject=IDC_IMAGE

[CLS:MainPage]
Type=0
HeaderFile=MainPage.h
ImplementationFile=MainPage.cpp
BaseClass=CPropertyPage
Filter=D
VirtualFilter=idWC
LastObject=MainPage

[DLG:IDD_FINAL_PAGE]
Type=1
Class=FinalPage
ControlCount=6
Control1=IDC_COMPILE_MSG,static,1342308352
Control2=IDC_EDIT_COMPILE,edit,1352728580
Control3=IDC_LINK_MSG,static,1342308352
Control4=IDC_EDIT_LINK,edit,1352728580
Control5=IDC_IMAGE,static,1342308352
Control6=IDC_FINAL_HEADING,static,1342308352

[CLS:FinalPage]
Type=0
HeaderFile=FinalPage.h
ImplementationFile=FinalPage.cpp
BaseClass=CPropertyPage
Filter=D
LastObject=IDC_EDIT_COMPILE
VirtualFilter=idWC

[DLG:IDD_USERUDT_PAGE]
Type=1
Class=UserUdts
ControlCount=6
Control1=IDC_USERUDT_TXT,static,1342308352
Control2=IDC_LIST_FILES,SysListView32,1350648837
Control3=IDC_BUTTON_SELECT,button,1342242816
Control4=IDC_BUTTON_DESELECT,button,1342242816
Control5=IDC_IMAGE,static,1342308352
Control6=IDC_BUTTON_VIEWSRC,button,1342242816

[CLS:UserUdts]
Type=0
HeaderFile=UserUdts.h
ImplementationFile=UserUdts.cpp
BaseClass=CPropertyPage
Filter=D
LastObject=IDC_BUTTON_DESELECT
VirtualFilter=idWC

[DLG:IDD_CHOOSEDIRS]
Type=1
Class=?
ControlCount=5
Control1=IDC_TREE1,SysTreeView32,1342242871
Control2=IDC_CHOOSEDIR_TXT,button,1342177287
Control3=IDC_EDIT1,edit,1350631552
Control4=IDOK,button,1342242817
Control5=IDCANCEL,button,1342242816

[DLG:IDD_WAIT_DIALOG]
Type=1
Class=?
ControlCount=2
Control1=IDC_AVI,SysAnimate32,1342242820
Control2=IDC_WAIT_HEADING,static,1342308352

[DLG:IDD_FAIL_DIALOG]
Type=1
Class=FailDlg
ControlCount=4
Control1=IDOK,button,1342242816
Control2=IDC_BUTTON_SEE_ERRS,button,1342242816
Control3=IDC_FAIL_HEADING,static,1342308352
Control4=IDC_STATIC,static,1342177283

[CLS:FailDlg]
Type=0
HeaderFile=FailDlg.h
ImplementationFile=FailDlg.cpp
BaseClass=CDialog
Filter=D
VirtualFilter=dWC
LastObject=IDC_BUTTON_SEE_ERRS

[DLG:IDD_OPTIONS_PAGE]
Type=1
Class=COptionsPage
ControlCount=10
Control1=IDC_SPATIAL_DESC,static,1342308352
Control2=IDC_CHECK_SPATIAL,button,1342242819
Control3=IDC_DEMO_UDT_DESC,static,1342308352
Control4=IDC_CHECK_DEMOS,button,1342242819
Control5=IDC_CHECK_USER_UDT,button,1342242819
Control6=IDC_PERSONAL_LOC,static,1342308352
Control7=IDC_EDIT_LOCATION,edit,1350631552
Control8=IDC_BUTTON_LOC,button,1342242816
Control9=IDC_IMAGE,static,1342308352
Control10=IDC_USER_UDT_DESC,static,1342308352

[CLS:COptionsPage]
Type=0
HeaderFile=OptionsPage.h
ImplementationFile=OptionsPage.cpp
BaseClass=CPropertyPage
Filter=D
LastObject=COptionsPage
VirtualFilter=idWC

