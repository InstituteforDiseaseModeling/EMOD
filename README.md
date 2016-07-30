## V2.8
What's New in EMOD V2.8
The EMOD source v2.8 release includes new and updated features as well as updates to the schema.
+ WaningEffectMapLinear and WaningEffectMapPiecewise capabilities have been added to provide more power and flexibility in defining how interventions can wane by allowing you to specify a detailed waning curve.
    + WaningEffectMapLinear allows you to specify a detailed waning curve using linear interpolation between points.
    + WaningEffectMapPiecewise allows you to specify a detailed waning curve using a piecewise/step function.
+ ControlledVaccine extends SimpleVaccine by adding controls to when an individual can and cannot be revaccinated.
+ ReferenceTrackingEventCoordinator allows you to define a particular coverage of an intervention that should be present in the population over time.
+ ReferenceTrackingEventCoordinatorHIV builds on the ReferenceTrackingEventCoordinator by adding an HIV specific state that individuals must have in order to get the intervention.
+ The schema for the config parameters has been updated to now only require config.json parameters if the dependent feature is enabled. For example, previous to v2.8 you had to include a parameter for Maternal_Transmission_Probability even if Enable_Maternal_Transmission was set to 0 (False/OFF). This update produces smaller config.json files making the configuration files easier to create, view, and edit.
+ For the list of breaking changes go to the [IDM documentation](http://idmod.org/idmdoc).

## V2.7.0
The EMOD source v2.7 release enhances the STI and HIV models with new and updated features.
+ Both multi-node and multi-core capabilities have been added to support spatial simulations including relationship migration.
+ Relationship types have been expanded:
    + A fourth relationship type, Commercial, has been added.
    + The maximum number of simultaneous partnership for an individual is increased to 63.
+ The HIVByAgeAndGender report now captures the data for HIV status.
+ Many of the relationship-oriented parameters have been moved from the configuration file to the demographics file including concurrency, pair-forming, extra-relationship rates. For the list of breaking changes go to the [IDM documentation](http://idmod.org/idmdoc).

##V2.6.0
With this release, you can now run EMOD on CentOS 7.1.
+ The EMOD executable has been tested and is supported on CentOS 7.1. EMOD has also been successfully built and run on Ubuntu, SUSE and Arch. If you have issues building the EMOD executable, you can contact IDM at IDMSupport@idmod.org.
+ The PrepareLinuxEnvironment script is available on GitHub. It is an example of creating an environment for building EMOD and cloning the source code. The script was designed and tested to run on an Azure CentOS 7.1 virtual machine but you can modify it for other distributions.

EMOD on Windows continues to be supported on Windows 10 and Windows Server 2012 for local simulations, and Windows HPC Server 2012 for remote simulations. For information on the prerequisites needed to build EMOD, go to [Prerequisite Software] (http://idmod.org/idmdoc/#EMOD/EMODBuildAndRegression/Prerequisite%20Software.htm).

For information on building EMOD, go to [Building the EMOD Executable](http://idmod.org/idmdoc/#EMOD/EMODBuildAndRegression/Building%20the%20EMOD%20Executable.htm).

##V2.5.0
This release: 
+ Supports disease-specific builds.
+ Extends the EMOD functionality with Python.
+ Simulates micro-spatial modeling at household level.
+ Models spatial migration of disease vectors with greater controls.
+ Runs multicore (HPC) simulations for all disease types.

For information on prerequisite software, building the EMOD executable, and learning about the disease models, go to the [IDM documentation](http://idmod.org/idmdoc).

##v2.0.0 HIV Release
+ This is the first release of the STI and HIV models. 
+ The STI contact network enables users to configure up to three relationship types with different durations, gender-specific levels of concurrency, age patterns of formation, and preference functions.
+ The HIV-specific implementation of the STI network model adds co-factors and interventions affecting transmission, disease progression on and off therapy, and detailed and time-variable linkage and retention along the care continuum. 
For tutorials on these new models, as well as the other disease models, prerequisite software, and building the EMOD executable, go to the [IDM documentation](http://idmod.org/idmdoc).

<a href="https://zenhub.com"><img src="https://raw.githubusercontent.com/ZenHubIO/support/master/zenhub-badge.png"></a>
