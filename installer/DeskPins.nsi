;; DeskPins.nsi

!include "WinVer.nsh"

!define PRETTY_VER "1.32"
!define FULL_VER "1.32.0.0"
!define UNINST_PATH Software\Microsoft\Windows\CurrentVersion\Uninstall\DeskPins
!define RUN_PATH Software\Microsoft\Windows\CurrentVersion\Run

;; The name of the installer
Name "DeskPins ${PRETTY_VER}"

;; The file to write
OutFile "DeskPins-${PRETTY_VER}-setup.exe"

;; component bitmaps (don't use default llama :)
CheckBitmap "${NSISDIR}\Contrib\Graphics\Checks\classic.bmp"

;; The default installation directory
InstallDir $PROGRAMFILES\DeskPins

;; Registry key to check for directory (so if you install again, it will 
;; overwrite the old one automatically)
InstallDirRegKey HKLM "SOFTWARE\Elias Fotinis\DeskPins" "Install_Dir"

;; The text to prompt the user to enter a directory
ComponentText "This will install DeskPins on your computer." \
              "" "Select the components you want to install:"

;; The text to prompt the user to enter a directory
DirText "Choose a directory to install in to:"

;; add manifest in installer/uninstaller
XPStyle on


;; installer version
;; ----------------
VIProductVersion ${FULL_VER}
VIAddVersionKey ProductName     "DeskPins"
VIAddVersionKey Comments        "Freeware"
VIAddVersionKey CompanyName     "Elias Fotinis"
VIAddVersionKey LegalCopyright  "Copyright © 2002-2015 Elias Fotinis"
VIAddVersionKey FileDescription "DeskPins installer"
VIAddVersionKey FileVersion     "{PRETTY_VER}"
VIAddVersionKey ProductVersion  "{PRETTY_VER}"

;; uninstall stuff
UninstallText "This will uninstall DeskPins. Hit next to continue."


;; ----------------
;; PAGES
;; ----------------


;;Page license
Page components
Page directory
Page instfiles


UninstPage uninstConfirm
UninstPage instfiles


Function .onInit
    ${IfNot} ${IsNT}
        Abort "This program cannot be installed on Windows 9x."
    ${EndIf}
FunctionEnd


;; ----------------
;; SECTIONS
;; ----------------


Section "Program files & help"

    SectionIn RO

    ;; Set output path to the installation directory.
    SetOutPath $INSTDIR

    ;; Put files there
    ${If} ${IsNT}
        File "..\Release\DeskPins.exe"
        ;;File "..\Release\dphook.dll" -- not used anymore
    ${Else}
        ;File "..\Release9x\DeskPins.exe"
        ;File "..\Release9x\dphook.dll"
        Abort "This program cannot be installed on Windows 9x."
    ${EndIf}
    File "..\Help\DeskPins.chm"

    ;; Write the installation path into the registry
    WriteRegStr HKLM "SOFTWARE\Elias Fotinis\DeskPins" "Install_Dir" "$INSTDIR"

    ;; Write the uninstall keys for Windows
    WriteRegStr HKLM "${UNINST_PATH}" "DisplayName" "DeskPins"
    WriteRegStr HKLM "${UNINST_PATH}" "UninstallString" '"$INSTDIR\uninst.exe"'
    WriteRegStr HKLM "${UNINST_PATH}" "DisplayIcon" '"$INSTDIR\DeskPins.exe"'
    WriteRegDWORD HKLM "${UNINST_PATH}" "NoModify" 1
    WriteRegDWORD HKLM "${UNINST_PATH}" "NoRepair" 1
    WriteRegStr HKLM "${UNINST_PATH}" "InstallLocation" "$INSTDIR"
    WriteRegStr HKLM "${UNINST_PATH}" "Publisher" "Elias Fotinis"
    WriteRegStr HKLM "${UNINST_PATH}" "DisplayVersion" "${PRETTY_VER}"

    WriteUninstaller "$INSTDIR\uninst.exe"

SectionEnd


Section "Create Start menu icons"

    CreateDirectory "$SMPROGRAMS\DeskPins"

    CreateShortCut  "$SMPROGRAMS\DeskPins\DeskPins.lnk"  "$INSTDIR\DeskPins.exe"
    CreateShortCut  "$SMPROGRAMS\DeskPins\Help.lnk"      "$INSTDIR\DeskPins.chm"
    CreateShortCut  "$SMPROGRAMS\DeskPins\Uninstall.lnk" "$INSTDIR\uninst.exe"

SectionEnd


Section "Start DeskPins with Windows"

    WriteRegStr HKCU "${RUN_PATH}" "DeskPins" "$INSTDIR\DeskPins.exe"

SectionEnd


;; uninstall
Section "Uninstall"

    ;; remove registry keys/values
    DeleteRegKey HKLM "${UNINST_PATH}"
    DeleteRegValue HKCU "${RUN_PATH}" "DeskPins"

    ;; delete app keys
    DeleteRegKey HKLM "SOFTWARE\Elias Fotinis\DeskPins"
    DeleteRegKey HKCU "SOFTWARE\Elias Fotinis\DeskPins"

    ;; delete company keys if empty
    DeleteRegKey /ifempty HKLM "SOFTWARE\Elias Fotinis"
    DeleteRegKey /ifempty HKCU "SOFTWARE\Elias Fotinis"

    ;; remove files (incl. uninstaller)
    Delete /REBOOTOK "$INSTDIR\DeskPins.exe"
    Delete /REBOOTOK "$INSTDIR\dphook.dll"  ;; legacy file
    Delete /REBOOTOK "$INSTDIR\DeskPins*.chm"
    Delete /REBOOTOK "$INSTDIR\uninst.exe"
    ;; remove legacy files
    Delete /REBOOTOK "$INSTDIR\lang*.dll"
    Delete /REBOOTOK "$INSTDIR\readme*.txt"

    ;; remove shortcuts, if any.
    Delete /REBOOTOK "$SMPROGRAMS\DeskPins\*.*"
    Delete /REBOOTOK "$SMSTARTUP\DeskPins.lnk"

    ;; remove directories used.
    RMDir "$SMPROGRAMS\DeskPins"
    RMDir "$INSTDIR"

SectionEnd
