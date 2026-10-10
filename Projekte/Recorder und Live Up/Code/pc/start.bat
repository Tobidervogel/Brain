@echo off
rem Startet den Audio-Recorder minimiert mit niedriger Prioritaet.
cd /d "%~dp0"
set HF_HUB_DISABLE_SYMLINKS_WARNING=1
start "Audio-Recorder" /min /belownormal .venv\Scripts\python.exe recorder.py
