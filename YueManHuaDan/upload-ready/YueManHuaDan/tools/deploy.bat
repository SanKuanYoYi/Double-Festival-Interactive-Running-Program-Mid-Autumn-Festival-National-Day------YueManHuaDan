@echo off
rem ============================================================================
rem  tools\deploy.bat -- package YueManHuaDan.exe together with the Qt DLLs it
rem  needs, into one folder you can double-click (or copy to another PC).
rem
rem  Usage:
rem      tools\deploy.bat [QTDIR] [EXE] [OUTDIR]
rem
rem      QTDIR    Qt kit dir containing bin\qmake.exe and plugins\,
rem               e.g. C:\Qt\5.15.2\mingw81_64
rem               If omitted: %QTDIR% is used, then a few common install
rem               locations are probed automatically.
rem      EXE      built executable. Default: build\YueManHuaDan.exe
rem      OUTDIR   output folder.    Default: deploy
rem
rem  The script calls windeployqt first. Some Qt distributions ship plugins
rem  with a .gnu_debuglink section, which makes windeployqt classify them as
rem  DEBUG builds and refuse to copy the platform plugin ("Unable to find the
rem  platform plugin"). When that happens the script falls back to copying the
rem  required DLLs directly, so the packaged folder always works.
rem
rem  Result layout (double-click YueManHuaDan.exe):
rem      YueManHuaDan.exe
rem      Qt5Core.dll  Qt5Gui.dll  Qt5Widgets.dll
rem      libgcc_s_seh-1.dll  libstdc++-6.dll  libwinpthread-1.dll
rem      platforms\qwindows.dll
rem      styles\qwindowsvistastyle.dll
rem      imageformats\qjpeg.dll ...   (needed to read the optional .jpg assets)
rem      images\                      (optional drop-in photo assets)
rem ============================================================================

setlocal
set "SCRIPT_DIR=%~dp0"
set "PROJ_DIR=%SCRIPT_DIR%.."

set "QT_DIR=%~1"
set "EXE=%~2"
set "OUTDIR=%~3"

if "%QT_DIR%"=="" set "QT_DIR=%QTDIR%"

if "%QT_DIR%"=="" (
    echo [deploy] QTDIR not given, probing common locations...
    call :probe "%USERPROFILE%\Qt"
    call :probe "C:\Qt"
    call :probe "D:\Qt"
    call :probe "C:\QtEnv"
    call :probe "D:\QtEnv"
)

if "%QT_DIR%"=="" (
    echo [deploy] ERROR: cannot locate Qt. Pass the kit dir as argument 1:
    echo          tools\deploy.bat C:\Qt\5.15.2\mingw81_64
    exit /b 1
)
if not exist "%QT_DIR%\bin\qmake.exe" (
    echo [deploy] ERROR: "%QT_DIR%\bin\qmake.exe" not found - not a Qt kit dir.
    exit /b 1
)

if "%EXE%"=="" set "EXE=%PROJ_DIR%\build\YueManHuaDan.exe"
if "%OUTDIR%"=="" set "OUTDIR=%PROJ_DIR%\deploy"

if not exist "%EXE%" (
    echo [deploy] ERROR: "%EXE%" not found. Build the project first.
    exit /b 1
)

echo [deploy] Qt     : %QT_DIR%
echo [deploy] exe    : %EXE%
echo [deploy] output : %OUTDIR%
echo.

if not exist "%OUTDIR%" mkdir "%OUTDIR%"
copy /y "%EXE%" "%OUTDIR%\" >nul

set "PATH=%QT_DIR%\bin;%PATH%"
if exist "%QT_DIR%\bin\windeployqt.exe" (
    echo [deploy] running windeployqt ...
    "%QT_DIR%\bin\windeployqt.exe" --release --no-translations --no-system-d3d-compiler --no-opengl-sw --compiler-runtime --dir "%OUTDIR%" "%OUTDIR%\YueManHuaDan.exe"
    if errorlevel 1 echo [deploy] windeployqt returned non-zero, continuing with fallback checks...
) else (
    echo [deploy] windeployqt not found, using direct copy only.
)

rem ---- fallback: make sure the runtime pieces really are there --------------
if not exist "%OUTDIR%\platforms\qwindows.dll" (
    echo [deploy] platform plugin missing -- windeployqt misdetected the plugins
    echo [deploy] as debug builds. Falling back to direct copy...
    if not exist "%OUTDIR%\platforms" mkdir "%OUTDIR%\platforms"
    if not exist "%OUTDIR%\styles" mkdir "%OUTDIR%\styles"
    if not exist "%OUTDIR%\imageformats" mkdir "%OUTDIR%\imageformats"

    copy /y "%QT_DIR%\bin\Qt5Core.dll"    "%OUTDIR%\" >nul 2>nul
    copy /y "%QT_DIR%\bin\Qt5Gui.dll"     "%OUTDIR%\" >nul 2>nul
    copy /y "%QT_DIR%\bin\Qt5Widgets.dll" "%OUTDIR%\" >nul 2>nul
    copy /y "%QT_DIR%\plugins\platforms\qwindows.dll"        "%OUTDIR%\platforms\" >nul 2>nul
    copy /y "%QT_DIR%\plugins\styles\qwindowsvistastyle.dll" "%OUTDIR%\styles\"     >nul 2>nul
    for %%P in (qjpeg qico qgif qsvg qwebp qtiff) do (
        copy /y "%QT_DIR%\plugins\imageformats\%%P.dll" "%OUTDIR%\imageformats\" >nul 2>nul
    )
)

rem MinGW runtime - normally handled by windeployqt --compiler-runtime
for %%R in (libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll) do (
    if not exist "%OUTDIR%\%%R" copy /y "%QT_DIR%\bin\%%R" "%OUTDIR%\" >nul 2>nul
)

rem Optional drop-in assets, so photos can be swapped without rebuilding
if exist "%PROJ_DIR%\images" if not exist "%OUTDIR%\images" (
    mkdir "%OUTDIR%\images"
    copy /y "%PROJ_DIR%\images\*.jpg" "%OUTDIR%\images\" >nul 2>nul
    copy /y "%PROJ_DIR%\images\*.png" "%OUTDIR%\images\" >nul 2>nul
)

echo.
if exist "%OUTDIR%\platforms\qwindows.dll" (
    echo [deploy] OK. Run "%OUTDIR%\YueManHuaDan.exe"
    echo [deploy] Copy the whole folder to any Windows PC - Qt is not required.
) else (
    echo [deploy] FAILED: still no platforms\qwindows.dll.
    echo [deploy] Check that "%QT_DIR%\plugins\platforms" exists.
    exit /b 1
)
endlocal
exit /b 0

rem ---------------------------------------------------------------------------
rem  :probe <root>  -- look for a mingw Qt kit under <root>, sets QT_DIR once
rem ---------------------------------------------------------------------------
:probe
if not exist "%~1" exit /b 0
for /d %%V in ("%~1\*") do (
    if not defined QT_DIR if exist "%%~V\mingw81_64\bin\qmake.exe" set "QT_DIR=%%~V\mingw81_64"
    if not defined QT_DIR if exist "%%~V\mingw_64\bin\qmake.exe"   set "QT_DIR=%%~V\mingw_64"
    if not defined QT_DIR if exist "%%~V\win64_mingw81\bin\qmake.exe" set "QT_DIR=%%~V\win64_mingw81"
    if not defined QT_DIR if exist "%%~V\mingw73_64\bin\qmake.exe" set "QT_DIR=%%~V\mingw73_64"
)
exit /b 0
