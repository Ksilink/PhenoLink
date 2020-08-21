#
# NSIS script for VLC 4609 Plugin Installation
#

!include MUI2.nsh

Name "VLC STANAG 4609 Plugin"

OutFile "../stanag4609_vlc_plugin_v0.1.b.exe"

Var StartMenuFolder

SetCompressor /SOLID lzma

InstallDir $PROGRAMFILES\VideoLAN\VLC\plugins\

  !insertmacro MUI_PAGE_COMPONENTS
  !insertmacro MUI_PAGE_DIRECTORY

  !insertmacro MUI_PAGE_STARTMENU VideoLAN $StartMenuFolder

  !insertmacro MUI_PAGE_INSTFILES

  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES



Section "4609 Demuxer"
    SetOutPath "$INSTDIR\demux\"
    File "C:\WorkDir\Simulations\OpenCV\build-mingw\libdemux4609ts_plugin.dll"
SectionEnd

Section "4609 KLV codec"
    SetOutPath "$INSTDIR\codec\"
    File "C:\WorkDir\Simulations\OpenCV\build-mingw\libcodec4609_klv_plugin.dll"
SectionEnd


Section "4609 Dictionnary"
    SetOutPath "$DOCUMENTS\4609\"
    File "C:\Users\admin\Documents\4609\dictionnary.txt"
SectionEnd


Section /o "Source Code"
    SetOutPath "$DOCUMENTS\VLC Plugin Source Code\"
    File "C:\WorkDir\Simulations\OpenCV\OpenCV_Rountines\OpenCV_Rountines\VLC Plugins\CMakeLists.txt"
    File "C:\WorkDir\Simulations\OpenCV\OpenCV_Rountines\OpenCV_Rountines\VLC Plugins\Install 4609 VLC Plugin.nsi"
    File "C:\WorkDir\Simulations\OpenCV\OpenCV_Rountines\OpenCV_Rountines\VLC Plugins\codec4609_klv.c"
    File "C:\WorkDir\Simulations\OpenCV\OpenCV_Rountines\OpenCV_Rountines\VLC Plugins\csa.c"
    File "C:\WorkDir\Simulations\OpenCV\OpenCV_Rountines\OpenCV_Rountines\VLC Plugins\csa.h"
    File "C:\WorkDir\Simulations\OpenCV\OpenCV_Rountines\OpenCV_Rountines\VLC Plugins\demux4609ts.c"
#    File "C:\WorkDir\Simulations\OpenCV\OpenCV_Rountines\OpenCV_Rountines\VLC Plugins\demux7023.c"
    File "C:\WorkDir\Simulations\OpenCV\OpenCV_Rountines\OpenCV_Rountines\VLC Plugins\dvb-text.h"
    File "C:\WorkDir\Simulations\OpenCV\OpenCV_Rountines\OpenCV_Rountines\VLC Plugins\dvbpsi_compat.h"
    File "C:\WorkDir\Simulations\OpenCV\OpenCV_Rountines\OpenCV_Rountines\VLC Plugins\subsdec.c"
    File "C:\WorkDir\Simulations\OpenCV\OpenCV_Rountines\OpenCV_Rountines\VLC Plugins\substext.h"
SectionEnd

Section "Un-installer Package"
    WriteUninstaller $INSTDIR\uninstaller_4609_plugins.exe

  !insertmacro MUI_STARTMENU_WRITE_BEGIN VideoLan

    ;Create shortcuts
    CreateDirectory "$SMPROGRAMS\$StartMenuFolder"
    CreateShortCut "$SMPROGRAMS\$StartMenuFolder\Uninstall_4609_plugin.lnk" "$INSTDIR\uninstaller_4609_plugins.exe"

  !insertmacro MUI_STARTMENU_WRITE_END



SectionEnd

;--------------------------------
;Uninstaller Section

Section "Uninstall"

  ;ADD YOUR OWN FILES HERE...

  Delete "$INSTDIR\demux\libdemux4609ts_plugin.dll"
  Delete "$INSTDIR\codec\libcodec4609_klv_plugin.dll"
  Delete "$DOCUMENTS\4609\dictionnary.txt"

  RMDir "$DOCUMENTS\4609\"

SectionEnd
