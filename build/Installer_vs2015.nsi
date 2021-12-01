!include MUI2.nsh
# name the installer
OutFile "Checkout_InstallerV2.0.R3-edf0b5b-1f20f28.exe"

SetCompressor /FINAL /SOLID lzma
SetCompressorDictSize 4
!define MUI_COMPONENTSPAGE_SMALLDESC ;No value

!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

InstallDir "$PROGRAMFILES64\Checkout"

; These are the programs that are needed by ACME Suite.
Section -Prerequisites
  SetOutPath $INSTDIR\Prerequisites

  MessageBox MB_YESNO "Install the Microsoft Visual C++ 2015-2019 Redistribual (x64) - 14 ?" /SD IDYES IDNO endNetCF
    File "C:\Work\GitHub\redist\VC_redist.x64.exe"
    ExecWait "$INSTDIR\VC_redist.x64.exe"
  endNetCF:

SectionEnd

# default section start; every NSIS script has at least one section.
Section "Main Installer Section"

# First step remove all !! to avoid mixing version and clear view on install error
RMDir /r $INSTDIR

# define output path
SetOutPath $INSTDIR\bin

# specify file to go in output path
File "D:/Checkout_2020/CheckoutApp/build\bin\Release\Checkout.exe"
File "D:/Checkout_2020/CheckoutApp/build\bin\Release\CheckoutHttpServer.exe"
File "D:/Checkout_2020/CheckoutApp/build\bin\Release\*.dll"
File "D:/Checkout_2020/CheckoutApp/build\bin\ffmpeg.exe"

File "D:/Checkout_2020/CheckoutApp/build\bin\QtWebEngineProcess.exe"
File "D:/Checkout_2020/CheckoutApp/build\bin\*.dll"

SetOutPath $INSTDIR\bin\resources
File "D:/Checkout_2020/CheckoutApp/build\bin\resources\*.pak"
File "D:/Checkout_2020/CheckoutApp/build\bin\resources\*.dat"

SetOutPath $INSTDIR\bin\styles
File "D:/Checkout_2020/CheckoutApp/build\bin\styles\*.dll"

SetOutPath $INSTDIR\bin\bearer
File "D:/Checkout_2020/CheckoutApp/build\bin\bearer\*.dll"

SetOutPath $INSTDIR\bin\iconengines
File "D:/Checkout_2020/CheckoutApp/build\bin\iconengines\*.dll"

SetOutPath $INSTDIR\bin\translations
File "D:/Checkout_2020/CheckoutApp/build\bin\translations\*.qm"

SetOutPath $INSTDIR\bin\qtwebengine_locales
File "D:/Checkout_2020/CheckoutApp/build\bin\translations\qtwebengine_locales\*.pak"


CreateDirectory $INSTDIR\bin\Cache
;File "D:/Checkout_2020/CheckoutApp/build\bin\*.manifest"

# define uninstaller name
WriteUninstaller $INSTDIR\uninstaller.exe

# default section end
SectionEnd

Section "Plugin Installer Section"

# define output path
SetOutPath $INSTDIR\bin\plugins
File "D:/Checkout_2020/CheckoutApp/build\bin\plugins\Release\*.dll"

;SetOutPath $INSTDIR\bin\Release\accessible
;File "D:/Checkout_2020/CheckoutApp/build\bin\accessible\*.dll"

SetOutPath $INSTDIR\bin\platforms
File "D:/Checkout_2020/CheckoutApp/build\bin\platforms\*.dll"

SetOutPath $INSTDIR\bin\iconengines
File "D:/Checkout_2020/CheckoutApp/build\bin\iconengines\*.dll"

SetOutPath $INSTDIR\bin\imageformats
File "D:/Checkout_2020/CheckoutApp/build\bin\imageformats\*.dll"

#SetOutPath $INSTDIR\bin\sqldrivers
#File "D:/Checkout_2020/CheckoutApp/build\bin\sqldrivers\*.dll"



SectionEnd

# create a section to define what the uninstaller does.
# the section will always be named "Uninstall"
Section "Uninstall"

# Always delete uninstaller first
Delete $INSTDIR\uninstaller.exe
Delete "$INSTDIR\bin\Checkout.exe"
Delete "$INSTDIR\bin\CheckoutProcessServer.exe"
Delete "$INSTDIR\bin\CheckoutCore.dll"
Delete "$INSTDIR\bin\CheckoutGui.dll"
Delete "$INSTDIR\bin\CheckoutPluginManager.dll"
Delete "$INSTDIR\bin\CheckoutServer.dll"
Delete "$INSTDIR\bin\icudt52.dll"
Delete "$INSTDIR\bin\icuin52.dll"
Delete "$INSTDIR\bin\icuuc52.dll"
Delete "$INSTDIR\bin\opencv_ffmpeg300_64.dll"
Delete "$INSTDIR\bin\opencv_world300.dll"
Delete "$INSTDIR\bin\Qt5Core.dll"
Delete "$INSTDIR\bin\Qt5Gui.dll"
Delete "$INSTDIR\bin\Qt5Network.dll"
Delete "$INSTDIR\bin\Qt5OpenGL.dll"
Delete "$INSTDIR\bin\Qt5Sql.dll"
Delete "$INSTDIR\bin\Qt5Widgets.dll"
Delete "$INSTDIR\bin\Qt5Xml.dll"
Delete "$INSTDIR\bin\Qt5XmlPatterns.dll"
Delete "$INSTDIR\bin\Qt5Concurrent.dll"

Delete "$INSTDIR\bin\plugins\*.dll"

SectionEnd
