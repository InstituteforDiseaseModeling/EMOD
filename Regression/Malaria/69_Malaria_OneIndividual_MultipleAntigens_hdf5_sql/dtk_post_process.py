import dtk_malaria_patients_report_to_hdf5_arrays
import dtk_malaria_patients_report_to_sql

print "Entering json to hdf5 conversion"
def application(output_path):
	print "Running json to hdf5 conversion"
	dtk_malaria_patients_report_to_hdf5_arrays.application(output_path)
	dtk_malaria_patients_report_to_sql.application(output_path)
