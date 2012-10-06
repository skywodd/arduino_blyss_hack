@echo off
for %%f in (*.ols.txt) do echo %%f : && type %%f | tail -n 1
pause