@echo off
cd /d "%~dp0"
set SRC=GridManager.cpp GridJson.cpp Graph.cpp Algorithms.cpp FailureManager.cpp PowerManager.cpp PowerStation.cpp Substation.cpp TransmissionLine.cpp Consumer.cpp
if /I "%1"=="console" (
  g++ -std=c++17 Main.cpp %SRC% -o GridRescue.exe
  if errorlevel 1 goto fail
  GridRescue.exe
  pause
  exit /b 0
)
g++ -std=c++17 -O2 WebMain.cpp %SRC% -o GridRescueWeb.exe -lws2_32
if errorlevel 1 goto fail
start "" http://localhost:8080
GridRescueWeb.exe 8080
pause
exit /b 0
:fail
echo Build failed. Check that g++ is installed and on PATH.
pause
exit /b 1
