@echo off
for %%f in (*.dump.txt) do type %%f | raw2line.py | line2dump.py > %%f
pause