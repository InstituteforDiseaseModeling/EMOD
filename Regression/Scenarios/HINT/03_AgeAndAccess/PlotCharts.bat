@SETLOCAL
@SET PATH=%PATH%;C:\Python27
start python.exe ..\..\Scripts\plotAllCharts.py A_BaselineOutbreak\output\InsetChart.json BaselineOutbreak
start python.exe ..\..\Scripts\plotPropertyReport.py A_BaselineOutbreak\output\PropertyReport.json

start python.exe ..\..\Scripts\plotAllCharts.py B_AgeTargetedVaccine\output\InsetChart.json AgeTargetedVaccine
start python.exe ..\..\Scripts\plotPropertyReport.py B_AgeTargetedVaccine\output\PropertyReport.json

start python.exe ..\..\Scripts\plotAllCharts.py C_AccessTargetedVaccine\output\InsetChart.json AccessTargetedVaccine
start python.exe ..\..\Scripts\plotPropertyReport.py C_AccessTargetedVaccine\output\PropertyReport.json
@ENDLOCAL