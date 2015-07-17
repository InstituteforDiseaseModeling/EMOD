@SETLOCAL
@SET PATH=%PATH%;C:\Python27
python.exe ..\..\Scripts\plotSIRChannels.py -c SEIR -t "Generic SEIR with vital dynamics" output\InsetChart.json
@ENDLOCAL
