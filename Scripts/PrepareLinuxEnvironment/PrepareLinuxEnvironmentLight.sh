#!/bin/bash

# Clear the screen for better presentation.
clear

# Change to the user's home directory and present an IDM banner to the user.
cd ~
print "Institute for Diesease Modeling
EMOD Software Linux Environment Preparation Script

This script is an example of creating an environment for using the Epidemiological MODeling software (EMOD). The script was designed and tested to run on an Azure CentOS 7 virtual machine.

For Windows users, go to http://idmod.org/idmdoc/#EMOD/EMODBuildAndRegression/BuildingEMODTOC.htm for instructions on downloading the EMOD source and the prerequisite software.

This script will:
  1. Elevate permissions through sudo. (Required)
  2. Update the system base packages. (Optional)
  3. Install missing dependency packages after checking for them. (Required)
  4. Add necessary environment variables. (Optional)
  5. Download the EMOD source and Input Data files (Optional)
    a. Create a directory for EMOD source and input data files.
    b. Prompt for GitHub permissions.
    c. Download the source, including the files that use large file support (lfs).

Before running the script, you will need:
   1. sudo privileges to install packages.
   2. 15GB free in your home directory.
   3. An Internet connection.

"

read -p "Are you ready to begin? (y/n) " AnswerYN
case ${AnswerYN:0:1} in
  y|Y )
    printf "\n\nStarting now.\n\n"
  ;;
  * )
    printf "\n\nExiting. No changes have been made to your environment.\n\n"
    exit 0
  ;;
esac


# Eject the user if they attempt to run the script as root.
if [ "$EUID" -eq 0 ]
then
  printf "${WarningMessage}This script should not be executed as root. Exiting.\n\n"
  exit 0
fi

# Check the user's version, displaying a message and exiting the script if not found to be supported.
OSTypeCheck=$(cat /etc/*-release | grep 'ID="centos"')
OSVersionCheck=$(cat /etc/*-release | grep 'VERSION_ID="7"')
if [ ${#OSTypeCheck} -eq 0 ] || [ ${#OSVersionCheck} -eq 0 ]
then
  printf "\n${WarningMessage}CentOS 7 is currently the only version supported by this script and the EMOD software. Exiting.\n\n"
  exit 0
fi

# Install it
sudo yum -y install epel-release
sudo yum -y install python-pip
sudo yum -y install wget
sudo yum -y install curl
sudo yum -y install python-devel
sudo yum -y install boost
sudo yum -y install boost-mpich
sudo yum -y install boost-mpich-devel
sudo yum -y install gcc-c++
sudo yum -y install numpy
sudo yum -y install scons
sudo yum -y install python-matplotlib
sudo yum -y install mpich
sudo yum -y install mpich-devel
sudo yum -y install git
sudo yum -y install xorg-x11-xauth
sudo yum -y install git-lfs
sudo yum -y install PyYAML
sudo pip install --upgrade pip

# Add the repository for lfs.
sudo curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.rpm.sh | sudo bash

# Python pip library installation.
sudo pip install numpy
sudo pip install xlrd

# Create an IDM directory and download the EMOD software.
NewDirectory="IDM"
if [ ! -d "${NewDirectory}" ]
then 
  printf "A directory named ${NewDirectory} already exists. Exiting.\n"
else
  mkdir ~/${NewDirectory}
  cd ~/${NewDirectory}
  git clone https://github.com/InstituteforDiseaseModeling/EMOD
  git clone https://github.com/InstituteforDiseaseModeling/EMOD-InputData
  cd EMOD-InputData
  git lfs fetch
  git lfs checkout
  cd ~
  declare -a BashChanges=("export EMOD_ROOT=~/${NewDirectory}/EMOD" "export PATH=\$PATH:/usr/lib64/mpich/bin/" "export PATH=\$PATH:.:\$EMOD_ROOT/Scripts/")
  ln -s ~/${NewDirectory}/EMOD-InputData ~/${NewDirectory}/EMOD/InputData
  echo "" >> ~/.bashrc
  echo "# BEGIN EMOD SOFTWARE CHANGES HERE" >> ~/.bashrc
  for AddToBash in "${BashChanges[@]}"
  do
    echo ${AddToBash} >> ~/.bashrc 
  done
  echo "# END EMOD SOFTWARE CHANGES HERE" >> ~/.bashrc
  printf "Your environment is now ready to use the EMOD Software. Remember to source your .bashrc file for the new environment variables to take effect.\n"
fi
