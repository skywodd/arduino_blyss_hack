Scripts and tools description : 
- clean_dump.bat : Remove all previous decoded text files
- decode_all.bat : Decode all OLS files into human-readable text files
- echo_all.bat : Take result from all text files and output RF hex-values to stdout
- line2dump.py : Compress multiple lines in one with separator
- ols2data.py : Target and decode Blyss RF frames found a raw OLS files 
- raw2line.py : Turn BusPirate dump to clear hex values tables

Input / Output files description :
- trame_xx.dump.txt : (output) Formatted hex values of EEPROM dump
- trame_xx.ols : (input) Raw OLS files
- trame_xx.ols.txt : (output) Console ouput of the RF decoding script