import dtk_test.dtk_compareReportSerialization_Immunity as cSI
import dtk_test.dtk_compareReportSerialization_Summary as cSS
import json

def application(output_path):
    with open('output/result.json', 'w') as f: 
        print( "Reading serialized population and generating data" )
        result_immunity = cSI.GenerateReportDataFromPopSerialization(output_path)
        result_summary = cSS.GenerateReportDataFromPopSerialization(output_path)
        
        result = {}
        result.update(result_immunity)
        result.update(result_summary)
        json.dump( result, f, sort_keys=True, indent=4, separators=(',', ': ') )
