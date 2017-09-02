## V2.13

The EMOD v2.13 release includes many new features for all supported simulation types.

### New configuration parameters

The following configuration parameters are new in the Generic model and can be used in all other models:

+ **Custom_Reports_Filename**: The name of the file containing custom report configuration parameters. Omitting this parameter or setting it to RunAllCustomReports will load all reporters found that are valid for the given simulation type. The file must be in JSON format.
+ **Incubation_Period_Log_Mean**: The mean of log normal for the incubation period distribution. **Incubation_Period_Distribution** must be set to LOG_NORMAL_DURATION.
+ **Incubation_Period_Log_Width**: The log width of log normal for the incubation period distribution. **Incubation_Period_Distribution** must be set to LOG_NORMAL_DURATION.
+ **Infectivity_Exponential_Baseline**: The scale factor applied to **Base_Infectivity** at the beginning of a simulation, before the infectivity begins to grow exponentially. **Infectivity_Scale_Type** must be set to EXPONENTIAL_FUNCTION_OF_TIME.
+ **Infectivity_Exponential_Delay**: The number of days before infectivity begins to ramp up exponentially. **Infectivity_Scale_Type** must be set to EXPONENTIAL_FUNCTION_OF_TIME.
+ **Infectivity_Exponential_Rate**: The daily rate of exponential growth to approach to full infectivity after the delay set by **Infectivity_Exponential_Delay** has passed. **Infectivity_Scale_Type** must be set to EXPONENTIAL_FUNCTION_OF_TIME.
+ **Memory_Usage_Halting_Threshold_Working_Set_MB**: The maximum size (in MB) of working set memory before the system throws an exception and halts.
+ **Memory_Usage_Warning_Threshold_Working_Set_MB**: The maximum size (in MB) of working set memory before memory usage statistics are written to the log regardless of log level.
+ **Serialization_Time_Steps**: The list of time steps after which to save the serialized state. 0 (zero) indicates the initial state before simulation, n indicates after the nth time step. By default, no serialized state is saved.
+ **Serialized_Population_Filenames**: Array of filenames with serialized population data. The number of filenames must match the number of cores used for the simulation. The file must be in .dtk format.
+ **Serialized_Population_Path**: The root path for the serialized population files.

The following configuration parameters are new in the Vector model (no new parameters specific to the Malaria model):

+ **Cycle_Arrhenius_1**: The temperature-independent scale factor in the Arrhenius equation for feeding cycle rate.
+ **Cycle_Arrhenius_2**: The temperature-dependent scale factor in the Arrhenius equation for feeding cycle rate.
+ **Cycle_Arrhenius_Reduction_Factor**: The scale factor applied to cycle duration (from oviposition to oviposition) to reduce the duration when primary follicles are at stage II rather than I in the case of newly emerged females. **Temperature_Dependent_Feeding_Cycle** must be set to ARRHENIUS_DEPENDENCE.
+ **Drought_Egg_Hatch_Delay**: Proportion of regular egg hatching that still occurs when habitat dries up. **Enable_Drought_Egg_Hatch_Delay** must be set to 1.
+ **Egg_Arrhenius1**: The temperature-independent scale factor in the Arrhenius equation for egg hatching rate.
+ **Egg_Arrhenius2**: The temperature-dependent scale factor in the Arrhenius equation for egg hatching rate.
+ **Egg_Hatch_Density_Dependence** The effect of larval density on egg hatching rate. Possible values are DENSITY_DEPENDENCE (Egg hatching is reduced when the habitat is nearing its carrying capacity) or NO_DENSITY_DEPENDENCE (Egg hatching is not dependent upon larval density).
+ **Enable_Drought_Egg_Hatch_Delay**: Controls whether or not eggs hatch at delayed rates, set by **Drought_Egg_Hatch_Delay**, when the habitat dries up completely.
+ **Enable_Egg_Mortality**: Controls whether or not to include a daily mortality rate on the egg population, which is independent of climatic factors.
+ **Enable_Temperature_Dependent_Egg_Hatching**: Controls whether or not temperature has an effect on egg hatching, defined by **Egg_Arrhenius_1** and **Egg_Arrhenius_2**.
+ **Enable_Vector_Aging**: Controls whether or not vectors undergo senescence as they age.
+ **Nighttime_Feeding_Fraction**: The fraction of feeds on humans (for a specific mosquito species) that occur during the nighttime. Thus the fraction of feeds on humans that occur during the day equals 1 - (value of this parameter).
+ **Temperature_Dependent_Feeding_Cycle**: "The effect of temperature on the duration between blood feeds. Possible values are NO_TEMPERATURE_DEPENDENCE (No relation to temperature; days between feeds will be constant and specified by **Days_Between_Feeds** for each species), ARRHENIUS_DEPENDENCE (Use the Arrhenius equation with parameters **Cycle_Arrhenius_1** and **Cycle_Arrhenius_2**), and BOUNDED_DEPENDENCE (Use an equation bounded at 10 days at 15C and use **Days_Between_Feeds** to set the duration at 30C).

The following configuration parameters are new in the STI model:

+ **STI_Coinfection_Acquisition_Multiplier**: The per-act HIV acquisition probability multiplier for individuals with the STI coinfection flag.
+ **STI_Coinfection_Transmission_Multiplier**: The per-act HIV transmission probability multiplier for individuals with the STI coinfection flag.

The following configuration parameters are new in the HIV model:

+ **Heterogeneous_Infectiousness_LogNormal_Scale**: Scale parameter of a log normal distribution that governs an infectiousness multiplier. The multiplier represents heterogeneity in infectivity and adjusts **Base_Infectivity**.
+ **Report_HIV_ByAgeAndGender_Add_Relationships**: Sets whether or not the ReportHIVByAgeAndGender.csv output file will contain data by relationship type on population currently in a relationship and ever in a relationship.  A sum of those in two or more partnerships and a sum of the lifetime number of relationships in each bin will be included.
+ **Report_HIV_ByAgeAndGender_Add_Transmitters**: When set to to true (1), the "transmitters" column is included in the output report.  For a given row, "Transmitters" indicates how many people that have transmitted the disease meet the specifications of that row.
+ **Report_HIV_ByAgeAndGender_Collect_Age_Bins_Data**: "This array of floats allows the user to define the age bins used to stratify the report by age. Each value defines the minimum value of that bin, while the next value defines the maximum value of the bin. The maximum number of age bins is 100.
+ **Report_HIV_ByAgeAndGender_Collect_Gender_Data**: Controls whether or not the report is stratified by gender; to enable gender stratification, set to true (1).
+ **Report_HIV_ByAgeAndGender_Collect_Intervention_Data**: Stratifies the population by adding a column of 0s and 1s depending on whether or not the person has the indicated intervention. This only works for interventions that remain with a person for a period of time, such as ART, VMMC, vaccine/PrEP, or a delay state in the cascade of care. You can name the intervention by adding a parameter **Intervention_Name** in the campaign file, and then give this parameter the same **Intervention_Name**. This allows you to have multiple types of vaccines, VMMCs, etc., but to only stratify on the type you want.
+ **Report_HIV_ByAgeAndGender_Collect_On_Art_Data**: Controls whether or not the output report is stratified by those people who are on ART and those who are not. Set to true (1) to enable stratification by ART status.

No new configuration parameters were added to the Airborne or TB models.

### New demographics parameters

The following demographics parameters are new or updated in the Generic model and can be used in all other models:

+ **NodeProperties**: An array that contains parameters that add properties to nodes in a simulation. For example, you can define values for intervention status, risk, and other properties and assign values to different nodes.
+ **Property**: The individual or node property type for which you will assign arbitrary values to create groups. You can then move individuals or nodes into or out of different groups or target interventions to particular groups. The InterventionStatus property is new and allows you to tag individuals or nodes based on intervention status, so that receiving an intervention can affect how other interventions are distributed. Use with **Disqualifying_Properties** and **New_Property_Value** in the campaign file.

### New campaign parameters

The following campaign parameters and intervention classes were added.

The addition of NodeProperties in the demographics file also necessitated the addition of **Node_Property_Restrictions** to control how interventions are distributed based on the property values assigned to each node.

The new property type InterventionStatus also necessitated the new campaign parameters **Disqualifying_Properties** and **New_Property_Value** to control how interventions are distributed based on the interventions already received. Disqualifying_Properties prevents an intervention from being distributed to individuals or nodes with certain property values. **New_Property_Value** updates the property value after they receive an intervention. For example, a household may be ineligible for clinical treatment for a length of time if they already received treatment during a drug campaign. These campaign parameters were previously only available for individuals in the HIV simulation type and were known as **Abort_States** and **Valid_Cascade_States**.

A new CommunityHealthWorkerEventCoordinator was added to set characteristics such as size of shipment, days between shipments, or maximum treatments distributed per day when interventions are distributed by community health workers.

Several new parameters were added to control effect of vaccines and disease exposure on immunity. For example, whether the effect of receiving more than one vaccine is multiplicative or additive or if there are immune boosting or priming effects.

Detailed descriptions for each of the new campaign parameters will be added shortly.

For more information, see the complete [EMOD documentation](https://institutefordiseasemodeling.github.io/EMOD/index.html).
