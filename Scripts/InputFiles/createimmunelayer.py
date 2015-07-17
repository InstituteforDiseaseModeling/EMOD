import json
import os
import scipy # for calculating means

input_path = "C:\\Users\\ewenger\\LOCAL\\SVN\\Data_Files\\Namawala"
demog_name = "Namawala_single_node_demographics.json"

binned_report_path = "\\\\rivendell.emod.int\\Emod\\home\\ewenger\\output\\simulations\\2012_12_06_17_09_54_755000"

# copy and modify values
demog_json_file = open( os.path.join( input_path, demog_name) )
demog_json = json.loads( demog_json_file.read() )

br_json_file = open( os.path.join( binned_report_path, "output", "BinnedReport.json") )
br_json = json.loads( br_json_file.read() )

for node in demog_json["Nodes"]:
    node.pop("IndividualAttributes")
    node.pop("NodeAttributes")

demog_json["Defaults"] = { "MSP_mean_antibody_distribution" : {},
                           "nonspec_mean_antibody_distribution" : {},
                           "PfEMP1_mean_antibody_distribution" : {},
                           "MSP_variance_antibody_distribution" : {},
                           "nonspec_variance_antibody_distribution" : {},
                           "PfEMP1_variance_antibody_distribution" : {}
                           }

age_bins = br_json["Header"]["Subchannel_Metadata"]["NumBinsPerAxis"][0]
num_pop_groups = age_bins + 1

pop_groups = [0] # needs extra zero since there are n_ages + 1 bins in demographics layer
lastage=0
for age in br_json["Header"]["Subchannel_Metadata"]["ValuesPerAxis"][0]:
    pop_groups.append((age/365.0 + lastage)/2)
    lastage = age/365.0

pop_groups[-1] = pop_groups[-2] + 25 #HACK

for k,v in demog_json["Defaults"].items():
    v = { "NumDistributionAxes": 1,
          "AxisNames": [ "age" ],
          "AxisUnits": [ "years" ],
          "AxisScaleFactors": [ 365 ],
          "NumPopulationGroups": [ num_pop_groups ],
          "PopulationGroups": [ pop_groups ],
          "ResultUnits": "mean fraction of antibody variants",
          "ResultScaleFactor": 1,
          "ResultValues": []
         }
    demog_json["Defaults"][k] = v


br_channel_titles = ["MSP", "Non-Specific", "PfEMP1"]
output_channel_titles = ["MSP", "nonspec", "PfEMP1"]

for antibody_type_idx in range(0,len(br_channel_titles)):

    pfemp_mean_results = [0] # needs extra zero since there are n_ages + 1 bins in demographics layer
    pfemp_std_results = [0]

    for age_idx in range(0,age_bins):
        pfemp     = br_json["Channels"]["Sum " + br_channel_titles[antibody_type_idx] + " Variant Fractions"]["Data"][age_idx][-365:]
        ss_pfemp  = br_json["Channels"]["Sum of Squared " + br_channel_titles[antibody_type_idx] + " Variant Fractions"]["Data"][age_idx][-365:]
        statpop   = br_json["Channels"]["Population"]["Data"][age_idx][-365:]

        mean_pfemp = []
        std_pfemp  = []
        for val,ss,pop in zip(pfemp,ss_pfemp,statpop):
            if pop > 0:
                mean = val/pop
                variance = ss/pop - mean**2
            else:
                mean = 0
                variance = 0
            mean_pfemp.append(mean)
            if variance < 0:
                std_pfemp.append(0)
            else:
                std_pfemp.append(variance**0.5)

        #print(scipy.mean(mean_pfemp), scipy.mean(std_pfemp))
        pfemp_mean_results.append(scipy.mean(mean_pfemp))
        pfemp_std_results.append(scipy.mean(std_pfemp))

    demog_json["Defaults"][output_channel_titles[antibody_type_idx] + "_mean_antibody_distribution"]["ResultValues"]     = pfemp_mean_results
    demog_json["Defaults"][output_channel_titles[antibody_type_idx] + "_variance_antibody_distribution"]["ResultValues"] = pfemp_std_results

f = open( os.path.join(input_path, "Namawala_single_node_demographics_IMMUNE_TEST.json"), 'w' )
f.write( json.dumps( demog_json, sort_keys=True, indent=4 ) )
f.close()