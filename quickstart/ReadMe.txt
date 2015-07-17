The EMOD Quick Start v2.0 requires, at a minimum, a 64-bit Microsoft Windows 7 or Microsoft Windows Server 2008 R2 for local simulations, and Microsoft Windows HPC Server 2008 R2 (V3) (Service Pack 2, SP2, to current SP4) for high performance cluster (remote) simulations. 

A 32-bit environment is NOT supported and any other operating system environment is NOT supported at this time.

|****
|**** It is highly recommended to go to  http://idmod.org/idmdoc/#EMOD/QSinstallation/Prerequisite%20Software.htm for  additional instructions and information on downloading the prerequisite software. 
|****
|****

The Quick Start contains scripts to create all of the graphs associated with the Disease Tutorials. In order to runs these scripts, you will need to install the following software to generate the simulation output graphs:

- Python 2.7
- pip.py (installer for Python packages)
  - dateutil  (required for matplotlib)
  - pyparsing (required for matplotlib)
  - six       (required for matplotlib)
- NumPy
- matplotlib

Additionally, some of the graphs for the HIV Tutorials may require:
- R
- Matlab
- Statistics and Machine Learning Toolbox (MathWorks TM) used with the Matlab scripts
- Mathematica

There are tools that may be helpful for installing Python, NumPy, and matplotlib which you can use at your own discretion, such as pip (https://pip.pypa.io/en/latest/installing.html) or Enthought Canopy (the express version is free and can be downloaded from https://www.enthought.com/downloads/). However, the instructions at http://idmod.org/idmdoc/#EMOD/QSinstallation/Prerequisite%20Software.htm will provide information specfic to this release of the Quick Start.

If you have any additional questions or comments, please email IDMSupport@intven.com.