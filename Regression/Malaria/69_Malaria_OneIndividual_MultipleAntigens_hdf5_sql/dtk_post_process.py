import dtk_test.dtk_malaria_patients_report_to_hdf5_arrays
import dtk_test.dtk_malaria_patients_report_to_sql
import sys

print( "Importing json to hdf5 conversion" )
def application(output_path):
    print( "Running json to hdf5 conversion" )
    dtk_test.dtk_malaria_patients_report_to_hdf5_arrays.application(output_path)
    dtk_test.dtk_malaria_patients_report_to_sql.application(output_path)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        ShowUsage()
        exit(0)

    application( sys.argv[1] )
