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
