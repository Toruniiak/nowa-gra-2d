@echo off
rem ==========================================================================
rem  Nowa Gra 2D - uruchamia gre do testow, ZAWSZE w najnowszej wersji.
rem  Skopiuj ten plik na Pulpit i uruchamiaj dwuklikiem.
rem
rem  Za kazdym razem: pobiera najnowszy kod z GitHuba, buduje i uruchamia
rem  serwer (przez WSL - serwer uzywa gniazd Linuksa), pobiera Godot 4.3
rem  jesli go nie ma, uruchamia gre, a po jej zamknieciu zatrzymuje serwer
rem  (z zapisem pozycji graczy).
rem
rem  Wymagania (jednorazowo): Git for Windows oraz WSL z Ubuntu.
rem ==========================================================================
setlocal EnableExtensions
chcp 65001 >nul
title Nowa Gra 2D - test

set "REPO_URL=https://github.com/Toruniiak/nowa-gra-2d.git"
set "GAME_DIR=%USERPROFILE%\nowa-gra-2d"
set "TOOLS_DIR=%LOCALAPPDATA%\nowa-gra-2d-tools"
set "PORT=7777"
set "GODOT_ZIP=Godot_v4.3-stable_win64.exe.zip"
set "GODOT=%TOOLS_DIR%\Godot_v4.3-stable_win64_console.exe"

where git >nul 2>&1
if errorlevel 1 (
  echo BLAD: brak programu Git. Zainstaluj go poleceniem:
  echo     winget install --id Git.Git -e
  echo albo ze strony https://git-scm.com - potem uruchom ten plik ponownie.
  goto :fail
)
where wsl >nul 2>&1
if errorlevel 1 goto :nowsl
wsl -e true >nul 2>&1
if errorlevel 1 goto :nowsl

rem ---------------------------------------------------------------- 1. kod
if exist "%GAME_DIR%\.git" (
  echo === Pobieram najnowsza wersje gry...
  git -C "%GAME_DIR%" pull --ff-only
  if errorlevel 1 echo Nie udalo sie pobrac aktualizacji - uruchamiam wersje, ktora juz jest.
) else (
  echo === Pierwsze uruchomienie: pobieram gre do %GAME_DIR%
  echo     GitHub moze poprosic o zalogowanie - to normalne przy prywatnym repo.
  git clone "%REPO_URL%" "%GAME_DIR%"
  if errorlevel 1 (
    echo BLAD: nie udalo sie pobrac gry z GitHuba.
    goto :fail
  )
)
for /f "delims=" %%v in ('git -C "%GAME_DIR%" log -1 --format^="%%h %%ad %%s" --date^=format:"%%Y-%%m-%%d %%H:%%M"') do echo Wersja: %%v

for /f "usebackq delims=" %%p in (`wsl -e wslpath -a "%GAME_DIR%"`) do set "WSL_DIR=%%p"
set "HELPER=%WSL_DIR%/tools/launcher/serwer_wsl.sh"

rem ---------------------------------------------------------------- 2. serwer
wsl -e bash "%HELPER%" build
if errorlevel 1 (
  echo BLAD: budowanie serwera w WSL nie powiodlo sie.
  goto :fail
)
wsl -e bash "%HELPER%" stop
echo === Uruchamiam serwer (osobne okno)...
start "Serwer gry" wsl -e bash "%HELPER%" run %PORT%
set /a tries=0
:waitserver
wsl -e bash "%HELPER%" ready
if not errorlevel 1 goto :serverready
set /a tries+=1
if %tries% geq 30 (
  echo BLAD: serwer nie wystartowal - zobacz okno "Serwer gry".
  goto :fail
)
timeout /t 1 /nobreak >nul
goto :waitserver
:serverready

if not exist "%GAME_DIR%\client\certs" mkdir "%GAME_DIR%\client\certs"
copy /y "%GAME_DIR%\server\certs\server.crt" "%GAME_DIR%\client\certs\dev_server.crt" >nul

rem ---------------------------------------------------------------- 3. Godot
if not exist "%GODOT%" (
  echo === Pobieram Godot 4.3 - jednorazowo, ok. 60 MB...
  if not exist "%TOOLS_DIR%" mkdir "%TOOLS_DIR%"
  curl.exe -fL --progress-bar -o "%TOOLS_DIR%\%GODOT_ZIP%" "https://github.com/godotengine/godot/releases/download/4.3-stable/%GODOT_ZIP%"
  if errorlevel 1 (
    echo BLAD: nie udalo sie pobrac Godota.
    goto :stopfail
  )
  tar -xf "%TOOLS_DIR%\%GODOT_ZIP%" -C "%TOOLS_DIR%"
  del "%TOOLS_DIR%\%GODOT_ZIP%"
)

rem ---------------------------------------------------------------- 4. gra
echo === Przygotowuje grafike gry...
"%GODOT%" --headless --path "%GAME_DIR%\client" --import >nul 2>&1
echo === Uruchamiam gre. Zamknij okno gry, aby zakonczyc.
"%GODOT%" --path "%GAME_DIR%\client" --resolution 450x800 -- --server-port=%PORT% >nul 2>&1

echo === Zatrzymuje serwer (zapisuje pozycje graczy)...
wsl -e bash "%HELPER%" stop
exit /b 0

:nowsl
echo BLAD: brak WSL (Linux w Windowsie), a serwer gry go potrzebuje.
echo Otworz PowerShell JAKO ADMINISTRATOR i wpisz:
echo     wsl --install -d Ubuntu
echo Potem uruchom komputer ponownie, dokoncz konfiguracje Ubuntu
echo (nazwa uzytkownika i haslo) i uruchom ten plik jeszcze raz.
goto :fail

:stopfail
wsl -e bash "%HELPER%" stop
:fail
echo.
pause
exit /b 1
