import dtk_compareReportSerialization_Immunity
import dtk_compareReportSerialization_Summary
import json

def application(output_path):
    with open('output/result.json', 'w') as f: 
        print( "Reading serialized population and generating data" )
        result_immunity = dtk_compareReportSerialization_Immunity.GenerateReportDataFromPopSerialization(output_path)
        result_summary = dtk_compareReportSerialization_Summary.GenerateReportDataFromPopSerialization(output_path)
        
        result = {}
        result.update(result_immunity)
        result.update(result_summary)
        json.dump( result, f, sort_keys=True, indent=4, separators=(',', ': ') )
