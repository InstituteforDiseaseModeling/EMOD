## V2.20

The EMOD v2.20 release includes support for typhoid disease modeling, including new campaign classes: **EnvironmentalDiagnostic**, **TyphoidCarrierDiagnostic**, **TyphoidVaccine**, and **TyphoidWASH**. 

**ImmunityBloodTest** was added for identifying whether an individual’s immunity meets a specified threshold and then broadcasts an event based on the results. This new campaign class can be used with all supported disease modeling sim types. 

**InterventionForCurrentPartners** can be used with STI and HIV sim types and provides a mechanism for the partners of individuals in the care system to also seek care. 

**OutbreakIndividualTBorHIV** extends **OutbreakIndividual** and allows for specifying HIV or a specific strain of infection for TB. 

In addition, configuration and campaign parameters that set the type of distribution (uniform, Gaussian, etc.) of infectiousness, incubation period, and delivery of interventions have been refactored. The number of distributions available and naming conventions used are now consistent across the configuration and campaign files. This change does not affect the distributions used in the demographics files.

A beta release of new campaign classes (not yet fully tested) are included to support surveillance of events, where events are listened to, detected, and broadcast when a threshold has been met. These classes include: **BroadcastCoordinatorEvent**, **BroadcastNodeEvent**, **DelayEventCoordinator**,  **SurveillanceEventCoordinator**, and **TriggeredEventCoordinator**.


### New configuration parameters

For the generic simulation type, the following new configuration parameters are available:

+ **Enable_Infectivity_Reservoir**: Controls whether or not an exogeneous reservoir of infectivity will be included in the simulation and allows for the infectivity in a node to be increased additively. When set to 1 (true), the demographics parameter **InfectivityReservoirSize** is expected in **NodeAtttributes** for each node.
+ **Minimum_End_Time**: The minimum time step the simulation must reach before checking for early termination conditions. **Enable_Abort_Zero_Infectivity** must be set to 1 (true).
+ **Enable_Abort_Zero_Infectivity**: Controls whether or not the simulation should be ended when total infectivity falls to zero. Supported only in single-node simulations.
+ **Random_Number_Generator_Policy**: The policy that determines if random numbers are generated for objects in a simulation on a per-core or per-node basis.
+ **Enable_Random_Generator_From_Serialized_Population**: The type of random number generator to use for objects in a simulation. Must set the RNG seed in **Run_Number**.


### New configuration parameters (Distribution)

Note: These configuration parameters are part of the refactoring of distribution parameters.

For the generic simulation type, the following new configuration parameters are available:

+ **Incubation_Period_Distribution**: The distribution type to use for assigning the incubation period to each individual in the population. Each individual’s value is a random draw from the distribution.
+ **Infectious_Period_Distribution**: The distribution type to use for assigning the infectious period to each individual in the population. Each individual’s value is a random draw from the distribution.
+ **Incubation_Period_Mean_1**: The mean of the first exponential distribution when **Incubation_Period_Distribution** is set to DUAL_EXPONENTIAL_DISTRIBUTION.
+ **Incubation_Period_Mean_2**: The mean of the second exponential distribution when **Incubation_Period_Distribution** is set to DUAL_EXPONENTIAL_DISTRIBUTION.
+ **Incubation_Period_Proportion_1**: The proportion of individuals in the first exponential distribution when **Incubation_Period_Distribution** is set to DUAL_EXPONENTIAL_DISTRIBUTION.
+ **Infectious_Period_Mean_1**: The mean of the first exponential distribution when **Infectious_Period_Distribution** is set to DUAL_EXPONENTIAL_DISTRIBUTION.
+ **Infectious_Period_Proportion_1**: The proportion of individuals in the first exponential distribution when **Infectious_Period_Distribution** is set to DUAL_EXPONENTIAL_DISTRIBUTION.
+ **Infectious_Period_Mean_2**: The mean of the second exponential distribution when Infectious_Period_Distribution is set to DUAL_EXPONENTIAL_DISTRIBUTION.
+ **Incubation_Period_Constant**: The incubation period to use for all individuals.
+ **Infectious_Period_Constant**: The infectious period to use for all individuals.
+ **Incubation_Period_Exponential**: The mean incubation period when **Incubation_Period_Distribution** is set to EXPONENTIAL_DISTRIBUTION.
+ **Infectious_Period_Exponential**: The mean infectious period when **Infectious_Period_Distribution** is set to EXPONENTIAL_DISTRIBUTION.
+ **Incubation_Period_Gaussian_Mean**: The mean of the incubation period when **Incubation_Period_Distribution** is set to GAUSSIAN_DISTRIBUTION.
+ **Incubation_Period_Gaussian_Std_Dev**: The standard deviation of the incubation period when **Incubation_Period_Distribution** is set to GAUSSIAN_DISTRIBUTION.
+ **Infectious_Period_Gaussian_Mean**: The mean of the infectious period when **Infectious_Period_Distribution** is set to GAUSSIAN_DISTRIBUTION.
+ **Incubation_Period_Lambda**: The scale value for the incubation period when **Incubation_Period_Distribution** is set to WEIBULL_DISTRIBUTION.
+ **Infectious_Period_Max**: The maximum infectious period when **Infectious_Period_Distribution** is set to UNIFORM_DISTRIBUTION.
+ **Infectious_Period_Min**: The minimum infectious period when **Infectious_Period_Distribution** is set to UNIFORM_DISTRIBUTION.
+ **Incubation_Period_Proportion_0**: The proportion of individuals to assign a value of zero days incubation when **Incubation_Period_Distribution** is set to DUAL_CONSTANT_DISTRIBUTION.
+ **Incubation_Period_Peak_2_Value**: The incubation period value to assign to the remaining individuals when **Incubation_Period_Distribution** is set to DUAL_CONSTANT_DISTRIBUTION.
+ **Infectious_Period_Proportion_0**: The proportion of individuals to assign a value of zero days infectiousness when **Infectious_Period_Distribution** is set to DUAL_CONSTANT_DISTRIBUTION.
+ **Infectious_Period_Peak_2_Value**: The infectious period value to assign to the remaining individuals when **Infectious_Period_Distribution** is set to DUAL_CONSTANT_DISTRIBUTION.
+ **Infectious_Period_Gaussian_Std_Dev**: The standard deviation of the infectious period when **Infectious_Period_Distribution** is set to GAUSSIAN_DISTRIBUTION.
+ **Infectious_Period_Lambda**: The scale value for the infectious period when **Infectious_Period_Distribution** is set to WEIBULL_DISTRIBUTION.
+ **Incubation_Period_Log_Normal_Width**: The width of the incubation period when **Incubation_Period_Distribution** is set to LOG_NORMAL_DISTRIBUTION.
+ **Infectious_Period_Log_Normal_Width**: The width of the infectious period when **Infectious_Period_Distribution** is set to LOG_NORMAL_DISTRIBUTION.
+ **Incubation_Period_Max**: The maximum incubation period when **Incubation_Period_Distribution** is set to UNIFORM_DISTRIBUTION.
+ **Incubation_Period_Min**: The minimum incubation period when **Incubation_Period_Distribution** is set to UNIFORM_DISTRIBUTION.
+ **Incubation_Period_Poisson_Mean**: The mean of the incubation period when **Incubation_Period_Distribution** is set to POISSON_DISTRIBUTION.
+ **Infectious_Period_Poisson_Mean**: The mean of the infectious period with **Infectious_Period_Distribution** is set to POISSON_DISTRIBUTION.
+ **Incubation_Period_Kappa**: The shape value for the incubation period when **Incubation_Period_Distribution** is set to WEIBULL_DISTRIBUTION.
+ **Infectious_Period_Kappa**: The shape value for the infectious period when **Infectious_Period_Distribution** is set to WEIBULL_DISTRIBUTION.
+ **Incubation_Period_Log_Normal_Mean**: The mean of the incubation period when **Incubation_Period_Distribution** is set to LOG_NORMAL_DISTRIBUTION.
+ **Infectious_Period_Log_Normal_Mean**: The mean of the infectious period when **Infectious_Period_Distribution** is set to LOG_NORMAL_DISTRIBUTION.


### New configuration parameters (Beta)

Note: These configuration parameters are currently in beta release and have not yet been fully tested.

For the generic simulation type, the following new configuration parameters are available:

+ **Report_Surveillance_Event_Recorder**: Enables or disables the ReportSurveillanceEventRecorder.csv output report. When enabled (set to 1) reports will be generated for the broadcasted valid coordinator events, as specified in **Report_Surveillance_Event_Recorder_Events**.
+ **Report_Coordinator_Event_Recorder_Events**: The list of events to include or exclude in the ReportCoordinatorEventRecorder.csv output report, based on how **Report_Coordinator_Event_Recorder_Ignore_Events_In_List** is set. This list must not be empty and is dependent upon **Report_Coordinator_Event_Recorder** being enabled. In addition, the events must be defined in **Customer_Coordinator_Events**.
+ **Report_Surveillance_Event_Recorder_Events**: The list of events to include or exclude in the ReportSurveillanceEventRecorder.csv output report, based on how **Report_Surveillance_Event_Recorder_Ignore_Events_In_List** is set. This list must not be empty and is dependent upon **Report_Surveillance_Event_Recorder** being enabled.
+ **Report_Coordinator_Event_Recorder**: Enables or disables the ReportCoordinatorEventRecorder.csv output report for coordinator events. When enabled (set to 1) reports will be generated for the broadcasted valid coordinator events, as specified in **Report_Coordinator_Event_Recorder_Events**.
+ **Report_Surveillance_Event_Recorder_Stats_By_IPs**: Specifies an array of (optional) individual property keys, as defined in **IndividualProperties** in the demographics file, to be added to the ReportSurveillanceEventRecorder.csv output report. For each key:value pair there will be two additional columns (Key:Value:NumIndividuals, Key:Value:NumInfected) added to the report. For example, with a Risk property key assigned the values of LOW and HIGH there would then be four additional columns (Risk:LOW:NumIndividuals, Risk:LOW:NumInfected, Risk:HIGH:NumIndividuals, Risk:HIGH:NumInfected). An empty array equals no additional columns added.
+ **Report_Surveillance_Event_Recorder_Ignore_Events_In_List**: If set to false (0), only the events listed in the **Report_Surveillance_Event_Recorder_Events** array will be included in the ReportSurveillanceEventRecorder.csv output report. If set to true (1), only the events listed in the array will be excluded, and all other events will be included. If you want to return all events from the simulation, leave the events array empty.
+ **Report_Coordinator_Event_Recorder_Ignore_Events_In_List**: If set to false (0), only the events listed in the **Report_Coordinator_Event_Recorder_Events** array will be included in the ReportCoordinatorEventRecorder.csv output report. If set to true (1), only the events listed in the array will be excluded, and all other events will be included. If you want to return all events from the simulation, leave the events array empty.
+ **Report_Node_Event_Recorder_Events**: The list of node events to include or exclude in the ReportNodeEventRecorder.csv output report, based on how **Report_Node_Event_Recorder_Ignore_Events_In_List** is set.
+ **Report_Node_Event_Recorder_Node_Properties**: Specifies an array of (optional) node property keys, as defined in **NodeProperties** in the demographics file, to be added as additional columns to the ReportNodeEventRecorder.csv output report.
+ **Report_Node_Event_Recorder**: Enables or disables the ReportNodeEventRecorder.csv output report. When enabled (set to 1) reports will be generated for the broadcasted valid node events, as specified in **Report_Node_Event_Recorder_Events**.
+ **Report_Node_Event_Recorder_Stats_By_IPs**: Specifies an array of (optional) individual property keys, as defined in **IndividualProperties** in the demographics file, to be added to the ReportNodeEventRecorder.csv output report. For each key:value pair there will be two additional columns (Key:Value:NumIndividuals, Key:Value:NumInfected) added to the report. For example, with a Risk property key assigned the values of LOW and HIGH there would then be four additional columns (Risk:LOW:NumIndividuals, Risk:LOW:NumInfected, Risk:HIGH:NumIndividuals, Risk:HIGH:NumInfected). An empty array equals no additional columns added.
+ **Report_Node_Event_Recorder_Ignore_Events_In_List**: If set to false (0), only the node events listed in the **Report_Node_Event_Recorder_Events** array will be included in the ReportNodeEventRecorder.csv output report. If set to true (1), only the node events listed in the array will be excluded, and all other node events will be included. If you want to return all node events from the simulation, leave the node events array empty.


### New demographics parameters

+ **InfectivityReservoirEndTime**: The ending of the exogeneous reservoir of infectivity. This parameter is conditional upon the configuration parameter, **Enable_Infectivity_Reservoir**, being enabled (set to 1).
+ **InfectivityReservoirSize**: The quantity-per-timestep added to the total infectivity present in a node; it is equivalent to the expected number of additional infections in a node, per timestep. For example, if timestep is equal to a day, then setting **InfectivityReservoirSize** to a value of 0.1 would introduce an infection every 10 days from the exogenous reservoir. This parameter is conditional upon the configuration parameter, **Enable_Infectivity_Reservoir**, being enabled (set to 1).
+ **InfectivityReservoirStartTime**: The beginning of the exogeneous reservoir of infectivity. This parameter is conditional upon the configuration parameter, **Enable_Infectivity_Reservoir**, being enabled (set to 1).


### New campaign parameters

The following campaign classes are new and can be used in the (specified) models:

### ImmunityBloodTest (generic)

The **ImmunityBloodTest** intervention class identifies whether an individual’s immunity meets a specified threshold (as set with the **Positive_Threshold_AcquisitionImmunity** campaign parameter) and then broadcasts an event based on the results; positive has immunity while negative does not.

### InterventionForCurrentPartners (HIV, STI)

The **InterventionForCurrentPartners** intervention class provides a mechanism for the partners of individuals in the care system to also seek care. Partners do not need to seek testing at the same time; a delay may occur between the initial test and the partner’s test. If a relationship has been paused, such as when a partner migrates to a different node, the partner will not be contacted.

### OutbreakIndividualTBorHIV (tuberculosis)

The **OutbreakIndividualTBorHIV** class extends **OutbreakIndividual** class and allows for specifying HIV or a specific strain of infection for TB.

### EnvironmentalDiagnostic (typhoid)

The **EnvironmentalDiagnostic** intervention class identifies contaminated locations by sampling the environment, comparing the value to a threshold, and broadcasting either a positive or negative node event.

### TyphoidCarrierDiagnostic (typhoid)

The **TyphoidCarrierDiagnostic** class extends **SimpleDiagnostic** class and allows for positive test diagnostic when an individual is a chronic typhoid carrier.

### TyphoidVaccine (typhoid)

The **TyphoidVaccine** intervention class identifies contaminated locations by sampling the environment, comparing the value to a threshold, and broadcasting either a positive or negative node event.

### TyphoidWASH (typhoid)

The **TyphoidWASH** intervention class acts on exposure through either the contact contagion population or the environmental contagion population in the simulation. The intervention can be configured to reduce either exposure dose or exposure frequency for each route, simulating effects of water, sanitation, and hygiene (WASH) interventions.


### New campaign parameters (Beta)

Note: These campaign classes and associated parameters are currently in beta release and have not yet been fully tested.

The following Beta campaign classes are new and can be used in the (specified) models:

### BroadcastCoordinatorEvent (generic)

The **BroadcastCoordinatorEvent** coordinator class broadcasts the event you specify. This can be used with the campaign class, **SurveillanceEventCoordinator**, that can monitor and listen for events received from **BroadcastCoordinatorEvent** and then perform an action based on the broadcasted event. You can also use this for the reporting of the broadcasted events by setting the configuration parameters, **Report_Node_Event_Recorder** and **Report_Surveillance_Event_Recorder**, which listen to events to be recorded.

### BroadcastNodeEvent (generic)

The **BroadcastNodeEvent** coordinator class broadcasts node events. This can be used with the campaign class, **SurveillanceEventCoordinator**, that can monitor and listen for events received from **BroadcastNodeEvent** and then perform an action based on the broadcasted event. You can also use this for the reporting of the broadcasted events by setting the configuration parameters, **Report_Node_Event_Recorder** and **Report_Surveillance_Event_Recorder**, which listen to events to be recorded.

### DelayEventCoordinator (generic)

The **DelayEventCoordinator** coordinator class insert delays into coordinator event chains. This campaign event is typically used with **BroadcastCoordinatorEvent** to broadcast events after the delays.

### SurveillanceEventCoordinator (generic)

The **SurveillanceEventCoordinator** coordinator class listens for and detects events happening and then responds with broadcasted events when a threshold has been met. This campaign event is typically used with other classes, such as **BroadcastCoordinatorEvent**, **TriggeredEventCoordinator**, and **DelayEventCoordinator**.

### TriggeredEventCoordinator (generic)

The **TriggeredEventCoordinator** coordinator class listens for trigger events, begins a series of repetitions of intervention distributions, and then broadcasts an event upon completion. This campaign event is typically used with other classes that broadcast and distribute events, such as **BroadcastCoordinatorEvent**, **DelayEventCoordinator**, and **SurveillanceEventCoordinator**.


### Deprecated configuration parameters

**Base_Population_Scale_Factor** has been renamed to **x_Base_Population**, which is grouped together with the other scale factor parameters beginning with x_. The functional remains the same. **Enable_Demographics_Gender** has been deprecated. **Animal_Reservoir_Type** has been replaced with **Enable_Infectivity_Reservoir**.

The following configuration parameters have been deprecated as a result of the refactoring of distribution parameters for better consistency across the configuration and campaign files.

+ **Incubation_Period_Log_Mean**
+ **Incubation_Period_Log_Width**
+ **Infectious_Period_Mean**
+ **Infectious_Period_Std_Dev**


### Deprecated campaign parameters

The following campaign parameters have been deprecated as a result of the refactoring of distribution parameters for better consistency across the configuration and campaign files.

+ **BitingRisk Constant**
+ **BitingRisk Risk_Distribution_Type**
+ **BitingRisk Exponential_Mean**
+ **BitingRisk Gaussian_Mean**
+ **BitingRisk Gaussian_Std_Dev**
+ **BitingRisk Uniform_Max**
+ **BitingRisk Uniform_Min**
+ **CommunityHealthWorkerEventCoordinator Initial_Amount**
+ **CommunityHealthWorkerEventCoordinator Initial_Amount_Distribution_Type**
+ **CommunityHealthWorkerEventCoordinator Initial_Amount_Mean**
+ **CommunityHealthWorkerEventCoordinator Initial_Amount_Std_Dev**
+ **DelayedIntervention Delay_Distribution**
+ **DelayedIntervention Delay_Period**
+ **DelayedIntervention Delay_Period_Mean**
+ **DelayedIntervention Delay_Period_Scale**
+ **DelayedIntervention Delay_Period_Shape**
+ **DelayedIntervention Delay_Period_Std_Dev**
+ **DelayEventCoordinator Delay_Distribution**
+ **DelayEventCoordinator Delay_Period**
+ **DelayEventCoordinator Delay_Period_Mean**
+ **DelayEventCoordinator Delay_Period_StdDev**
+ **HIVDelayedIntervention Delay_Distribution**
+ **HIVDelayedIntervention Delay_Period**
+ **HIVDelayedIntervention Delay_Period_Mean**
+ **HIVDelayedIntervention Delay_Period_Scale**
+ **HIVDelayedIntervention Delay_Period_Shape**
+ **HIVDelayedIntervention Delay_Period_Std_Dev**
+ **HIVMuxer Delay_Distribution**
+ **HIVMuxer Delay_Period**
+ **HIVMuxer Delay_Period_Mean**
+ **HIVMuxer Delay_Period_Scale**
+ **HIVMuxer Delay_Period_Shape**
+ **HIVMuxer Delay_Period_Std_Dev**
+ **MigrateFamily Duration_At_Node_Distribution_Type**
+ **MigrateFamily Duration_At_Node_Exponential_Period**
+ **MigrateFamily Duration_At_Node_Fixed**
+ **MigrateFamily Duration_At_Node_Gausian_Mean**
+ **MigrateFamily Duration_At_Node_Gausian_StdDev**
+ **MigrateFamily Duration_At_Node_Uniform_Max**
+ **MigrateFamily Duration_At_Node_Uniform_Min**
+ **MigrateFamily Duration_Before_Leaving_Distribution_Type**
+ **MigrateFamily Duration_Before_Leaving_Exponential_Period**
+ **MigrateFamily Duration_Before_Leaving_Fixed**
+ **MigrateFamily Duration_Before_Leaving_Gausian_Mean**
+ **MigrateFamily Duration_Before_Leaving_Gausian_StdDev**
+ **MigrateFamily Duration_Before_Leaving_Uniform_Max**
+ **MigrateFamily Duration_Before_Leaving_Uniform_Min**
+ **MigrateIndividuals Duration_At_Node_Distribution_Type**
+ **MigrateIndividuals Duration_At_Node_Exponential_Period**
+ **MigrateIndividuals Duration_At_Node_Fixed**
+ **MigrateIndividuals Duration_At_Node_Gausian_Mean**
+ **MigrateIndividuals Duration_At_Node_Gausian_StdDev**
+ **MigrateIndividuals Duration_Before_Leaving_Distribution_Type**
+ **MigrateIndividuals Duration_Before_Leaving_Exponential_Period**
+ **MigrateIndividuals Duration_Before_Leaving_Fixed**
+ **MigrateIndividuals Duration_Before_Leaving_Gausian_Mean**
+ **MigrateIndividuals Duration_Before_Leaving_Gausian_StdDev**
+ **MigrateIndividuals Duration_Before_Leaving_Uniform_Max**
+ **MigrateIndividuals Duration_Before_Leaving_Uniform_Min**
+ **MigrateIndividuals Duration_At_Node_Uniform_Max**
+ **MigrateIndividuals Duration_At_Node_Uniform_Min**
+ **UsageDependentBednet Expiration_Distribution_Type**
+ **UsageDependentBednet Expiration_Percentage_Period_1**
+ **UsageDependentBednet Expiration_Period**
+ **UsageDependentBednet Expiration_Period_1**
+ **UsageDependentBednet Expiration_Period_2**
+ **UsageDependentBednet Expiration_Period_Mean**
+ **UsageDependentBednet Expiration_Period_Std_Dev**


For more information, see the complete [EMOD documentation](http://idmod.org/documentation).