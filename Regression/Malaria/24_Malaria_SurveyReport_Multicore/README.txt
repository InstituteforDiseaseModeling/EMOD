24_Malaria_SurveyReport_Multicore
DMB 3/30/2017

This regression test is being used to test the following custom reports:
- MalariaSurveyJSONAnalyzer
- MalariaSummaryReport
- MalariaImmunityReport
- SpatialReportMalariaFiltered

It was originally created to verify that the first three reporters worked in a multicore situation.

The test has been extended to include testing SpatialReportMalariaFiltered.  If the files need to be
updated, one should also use the:

- compare_spatial_reports.py.  

This script will verify that the files with different filtering are consistent with each other and 
the built-in report.  It also verifies that the built-in report has the same data as InsetChart.json.
