@SETLOCAL
@SET PATH=%PATH%;C:\Python27
python.exe ..\..\Scripts\plotSIRChannels.py -c SEIRW -t "Generic SEIRS" output\InsetChart.json
@ENDLOCAL