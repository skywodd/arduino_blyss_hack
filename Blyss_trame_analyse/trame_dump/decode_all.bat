@echo off
echo Starting batch decoding ...
for %%f in (*.ols) do type %%f | ols2data.py > %%f.txt
echo Batch decoding done !