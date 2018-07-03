## V2.18

The EMOD v2.18 release includes many new features for all supported simulation types. In particular, the TB_SIM simulation type has been deprecated and replaced with TBHIV_SIM, which does not include HIV transmission but adds the ability to model the effect of HIV coinfection on the spread of TB.

### New configuration parameters

The following configuration parameters are new in the Generic model and can be used in all other models:

+ **Enable_Maternal_Antibodies_Transmission**: Controls whether or not mothers pass antibodies to their infants.
+ **Enable_Maternal_Protection**: Controls whether or not mothers pass immunity on to their infants.
+ **Enable_Natural_Mortality**: Controls whether or not individuals are removed from the simulation due to natural (non-disease) deaths. 
+ **Enable_Skipping**: Controls whether or not the simulation uses an optimization that can increase performance by up to 50% in some cases by probablistically exposing individuals rather than exposing every single person. Useful in low-prevalence, high-population scenarios.
+ **Enable_Susceptibility_Scaling**: Controls whether or not susceptibility is scaled by time as defined by **Susceptibility_Scaling_Type**.
+ **Maternal_Linear_Slope**: Slope parameter describing the rate of waning for maternal protection, must be positive. The per-day increase in susceptibility.
+ **Maternal_Protection_Type**: "The type of maternal protection afforded to infants.
+ **Maternal_Sigmoid_HalfMaxAge**: The age in days that the level of maternal protection is half of its initial value. **Maternal_Protection_Type** must be set to SIGMOID_FRACTIONAL or SIGMOID_BINARY.
+ **Maternal_Sigmoid_SteepFac**: The steepness factor describing the rate of waning for maternal protection, must be positive. Small values imply rapid waning.
+ **Maternal_Sigmoid_SusInit**: The initial level of maternal protection at age -INF, given as susceptibility. A value of 0.0 implies total protection, a value of 1.0 implies no protection.
+ **Node_Contagion_Decay_Rate**: The fraction of contagion not carried over to the next time step.
+ **Susceptibility_Scaling_Rate**: The scaling rate for the variation in time of the log-linear susceptibility scaling. **Susceptibility_Scaling_Type** must be set to LOG_LINEAR_FUNCTION_OF_TIME.
+ **Susceptibility_Scaling_Type**: The effect of time on susceptibility.
+ **Susceptibility_Type**: Controls implementation of an individual's susceptibility.

The following configuration parameters are new in the Malaria model:

+ **Genome_Markers**: A list of the names (strings) of genome marker(s) that represent the genetic components in a strain of an infection.
+ **Resistance**: Specifies the drug resistance multiplier.

The following configuration parameters are new in the TBHIV model:

+ **ART_Reactivation_Factor**: Factor for ART reducing reactivation in HIV+.
+ **CD4_Num_Steps**: Number of time steps in CD4 forward vector for reactivation.
+ **CD4_Time_Step**: The length of steps in the CD4 longitudinal tracker.
+ **TB_Drug_Cure_Rate**: The daily rate at which treatment with an anti-TB drug causes disease clearance.
+ **TB_Drug_Cure_Rate_HIV**: The daily probability of active TB infection being cured in an HIV+ person under drug treatment.
+ **TB_Drug_Cure_Rate_MDR**: The daily probability of active MDR infection being cured in an HIV+ or HIV- person under drug treatment.
+ **TB_Drug_Inactivation_Rate_HIV**: The daily probability of active drug-sensitive TB infection becoming latent in an HIV+ person, not currently on ART, under drug treatment.
+ **TB_Drug_Inactivation_Rate_MDR**: The daily probability of active MDR infection becoming latent in an HIV+ or HIV-  person under drug treatment.
+ **TB_Drug_Mortality_Rate_HIV**: The daily probability of death for HIV+ individual with drug sensitive active infection under drug treatment.
+ **TB_Drug_Mortality_Rate_MDR**: The daily probability of death for individual with MDR active infection under drug treatment.
+ **TB_Drug_Relapse_Rate_HIV**: The daily probability that a drug-sensitive active infection in an HIV+ individual not currently on ART will inactivate but subsequently relapse under drug treatment.
+ **TB_Drug_Relapse_Rate_MDR**: The daily probability that an active MDR-TB infection (in HIV+ or HIV-) will inactivate but subsequently relapse under drug treatment.
+ **TB_Drug_Resistance_Rate_HIV**: The daily probability that an (HIV+) individual with drug-sensitive TB will acquire MDR-TB under drug treatment.

### New campaign parameters

The following campaign classes are new for the Generic model and can be used in all other models:

#### IncidenceEventCoordinator (generic)

The **IncidenceEventCoordinator** coordinator class distributes interventions based on the number of events counted over a period of time.

#### MultiNodeInterventionDistributor (generic)

The **MultiNodeInterventionDistributor** intervention class distributes multiple node-level interventions when the distributor only allows specifying one intervention.

#### WaningEffectCombo (generic)

The **WaningEffectCombo** class is used within individual-level interventions and allows for specifiying a list of effects when the intervention only has one **WaningEffect** defined. These effects can be added or multiplied.

The following campaign classes are new for the Malaria model:

#### AdherentDrug (malaria)

The **AdherentDrug** class extends AntiMalarialDrug class and allows for incorporating different patterns of adherence to taking the drug.

#### BitingRisk (malaria, vector)

The **BitingRisk** class is used with individual-level interventions and allows for adjusting the relative risk of being bitten by a vector.

#### OutbreakIndividualMalaria (malaria)

The **OutbreakIndividualMalaria** class extends **OutbreakIndividual** class and allows for specifying a specific strain of infection.

The following campaign class is new for the TBHIV model:

#### TBHIVConfigurableTBdrug (tuberculosis)

The **TBHIVConfigurableTBdrug** class is an individual level intervention for TB treatment. The intervention applies TB drug effects to the progression, associated mortality, transmission and acquisition of TB infections in HIV positive and negative individuals.

### Deprecated demographics parameters

**ImmunityDistributionFlag**, **ImmunityDistribution1**, and **ImmunityDistribution2**  were renamed to **SusceptibilityDistributionFlag**, **SusceptibilityDistribution1**, and **SusceptibilityDistribution2**. In previous versions of EMOD, the naming was counterintuitive to the functionality. For example, setting a value of 1 for the immunity indicated zero immunity/complete susceptibility. Now the parameters more accurately reflect that you are setting a susceptibility value. The functionality is the same.

### Deprecated config parameters

**Immunity_Transmission_Factor**, **Immunity_Mortality_Factor**, and **Immunity_Acquisition_Factor** were renamed to **Post_Infection_Transmission_Multiplier**, **Post_Infection_Mortality_Multiplier**, and **Post_Infection_Acquistion_Multiplier**. The functionality is the same.

### Deprecated campaign parameters

The TB_SIM simulation type has been deprecated and replaced with TBHIV_SIM, which does not include HIV transmission but adds the ability to model the effect of HIV coinfection on the spread of TB. 

The following campaign classes, which have not yet been fully tested with the TBHIV simulation type, have been disabled:

* HealthSeekingBehaviorUpdate
* HealthSeekingBehaviorUpdateable
* AntiTBPropDepDrug
* SmearDiagnostic

In previous versions of EMOD, you could set the tendency of individuals to seek out health care using **HealthSeekingBehaviorUpdateable** and then update the value of the **Tendency** parameter using **HealthSeekingBehaviorUpdate**. Now, you use individual properties to update individuals when they receive the **SimpleHealthSeekingBehavior** intervention, as you would to control the flow of individuals through other intervention classes. For example, you could create an individual property with HSBold and HSBnew values in the demographics file and assign all individuals to HSBold. Then you could distribute the first **SimpleHealthSeekingBehavior** (with one Tendency value) to all HSBold individuals and use **New_Property_Value** to assign them to HSBnew after receiving the intervention. The next **SimpleHealthSeekingBehavior** intervention (with a different Tendency value) could be distributed, setting **Disqualifying_Properties** to HSBold and, if desired, using **New_Property_Value** to reassign HSBold to those individuals.

Similarly, **AntiTBPropDepDrug** was disabled and superseded with **TBHIVConfigurableTBDrug**, which allows for drug effects based on HIV status and where dependence on **IndividualProperties** is configured through **Property_Restrictions**. In addition, **AntiTBPropDepDrub** can be replaced with **AntiTBDrug**, also using **Property_Restrictions** and new property values to target particular individuals with drug interventions for tuberculosis without HIV coinfections.

**SmearDiagnostic** was disabled and can be replaced with **DiagnosticTreatNeg**. While SmearDiagnostic would only broadcast when an individual had a positive smear diagnostic, **DiagnosticTreatNeg** has the added benefit of broadcasting negative and default diagnostic test events.

**TB_Drug_Clearance_Rate_HIV** and **TB_Drug_Clearance_Rate_MDR** parameters have been renamed to **TB_Drug_Cure_Rate_HIV** and **TB_Drug_Cure_Rate_MDR**.

### Error messages
When attempting to run an intervention using one of the disabled tuberculosis classes, such as **AntiTBPropDepDrug**, **HealthSeekingBehaviorUpdate**, **HealthSeekingBehaviorUpdateable**, and **SmearDiagnostic**, you will receive an error similar to the following: "00:00:01 [0] [I] [JsonConfigurable] Using the default value ( "Intervention_Name" : "HealthSeekingBehaviorUpdateable" ) for unspecified parameter.
00:00:02 [0] [E] [Eradication] 00:00:02 [0] [E] [Eradication]
GeneralConfigurationException:
Exception in SimulationEventContext.cpp at 242 in Kernel::SimulationEventContextHost::LoadCampaignFromFile.
Array out of bounds"

Note: These campaign classes have been disabled because they have not yet been fully tested with the TBHIV simulation type.

### EMOD schema and simulation types
In the schema for the **Simulation_Type** parameter the enum values list additional simulation types which are not supported by EMOD. IDM-supported values include GENERIC_SIM, VECTOR_SIM, MALARIA_SIM, TBHIV_SIM, STI_SIM, and HIV_SIM.

For more information, see the complete [EMOD documentation](http://idmod.org/documentation).
