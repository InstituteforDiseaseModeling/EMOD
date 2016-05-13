#!/bin/bash

# Variable declaration section.
FontReset="\x1b[39;49;00m"
FontRed="\x1b[31;01m"
FontGreen="\x1b[32;01m"
FontBlink="\e[5;32;40m"
FontYellow="\x1b[33;01m"
FontBlue="\x1b[34;01m"
FontMagenta="\x1b[35;01m"
FontCyan="\x1b[36;01m"
OSTypeCheck=$(cat /etc/*-release | grep 'ID="centos"')
OSVersionCheck=$(cat /etc/*-release | grep 'VERSION_ID="7"')
declare -a RequiredBasePackages=("epel-release" "python-pip")
declare -a EMODPackageRequired=("wget" "curl" "python-devel" "boost" "boost-mpich" "boost-mpich-devel" "gcc-c++" "numpy" "scons" "python-matplotlib" "mpich" "mpich-devel" "git" "xorg-x11-xauth" "git-lfs" "PyYAML")
declare -a EMODPythonLibraryRequired=("numpy" "xlrd")
EMODGitHubURL="https://github.com/InstituteforDiseaseModeling/"
declare -a EMODSoftware=("EMOD" "EMOD-InputData")
WarningMessage="\n${FontRed}Warning: $FontReset"
IDMSupportEmail="idm-support@intven.com"
LineBreak="${FontGreen}********************************************************************************$FontReset\n"

# Clear the screen for better presentation.
stty sane
clear

# Display an informational banner if the "test" argument is passed.
# This allows a user to check the script flow while disabling functionality that would modify the environment.
TestState=0
if [ $1 ] && [ $1 == "test" ]
then
  TestState=1
  printf "${FontYellow}********************************************************************************
* TEST STATE ENABLED. Your system will not be changed.              *
********************************************************************************${FontReset}\n"
fi

# Change to the user's home directory and present an IDM banner to the user.
cd ~
printf "
  ___           _   _ _         _          __                               
 |_ _|_ __  ___| |_(_) |_ _   _| |_ ___   / _| ___  _ __                    
  | || '_ \/ __| __| | __| | | | __/ _ \ | |_ / _ \| '__|                   
  | || | | \__ \ |_| | |_| |_| | ||  __/ |  _| (_) | |                      
 |___|_|_|_|___/\__|_|\__|\__,_|\__\___|_|_|  \___/|_|      _ _             
 |  _ \(_)___  ___  __ _ ___  ___  |  \/  | ___   __| | ___| (_)_ __   __ _ 
 | | | | / __|/ _ \/ _\` / __|/ _ \ | |\/| |/ _ \ / _\` |/ _ \ | | '_ \ / _\` |
 | |_| | \__ \  __/ (_| \__ \  __/ | |  | | (_) | (_| |  __/ | | | | | (_| |
 |____/|_|___/\___|\__,_|___/\___| |_|  |_|\___/ \__,_|\___|_|_|_| |_|\__, |
                                                                      |___/ 


"

# Eject the user if they attempt to run the script as root.
if [ "$EUID" -eq 0 ]
then
  printf "${WarningMessage}You cannot execute this script as root. Exiting the script.\n\n"
  exit 0
fi

# Display a welcome message to the user.
printf "Welcome! IDM's Epidemiological MODeling software (EMOD) provides a qualitative and analytical means to model infectious disease control and eradication. IDM provides the EMOD source and input data files to accelerate the exploration of disease eradication through the use of computational modeling.
This script is an example of creating an environment for using EMOD. It was designed and tested to run on an Azure CentOS 7 virtual machine."

# Check the user's version, displaying a message and exiting the script if not found to be supported.
if [ ${#OSTypeCheck} -eq 0 ] || [ ${#OSVersionCheck} -eq 0 ]
then
  printf "\n${WarningMessage}This script is only supported on CentOS 7. Exiting the script.\n\n"
  exit 0
fi

# Provide an imformational message about the script's process to the user.
printf "\n\nThis script will guide you through the following process:
1. Attempt to update your system. (Optional)
2. Install software packages needed to build the EMOD executable and run simulations. (Required)
3. Modify your \$PATH variable. (Optional)
4. Download the EMOD source and input data files from IDM's GitHub repository. (Optional)
You'll need:
1. sudo privileges to install packages.
2. 15GB free in your home directory.
3. An Internet connection.\n\n"

# Prompt the user to update their system, providing a warning if they decline.
read -p "Are you ready to begin? (y/n) " AnswerYN
case ${AnswerYN:0:1} in
  y|Y )
    printf "\nBeginning the process.\n\n"
  ;;
  * )
    printf "${WarningMessage}Exiting the script.\n\n"
    exit 0
  ;;
esac

# Elevate the user to root level to update the system and install dependencies.
# The user is prompted and the script will then execute a sudo command.
# The system will then prompt for a password.
read -p "You'll need to elevate your permissions. Are you ready to sudo to root? (y/n) " AnswerYN
case ${AnswerYN:0:1} in
  y|Y )
    if [ $TestState -eq 0 ]
    then
      printf "\n"
      sudoCheck=$(sudo sh -c 'echo $UID')
      if [ -z $sudoCheck ]
      then
        printf "${WarningMessage}Without sudo permissions you cannot install the packages required by EMOD. Exiting the script.\n\n"
        exit 0
      elif [ $sudoCheck -eq 0 ]
      then
        printf "\n${FontYellow}********************************************************************************
* Caution!  You are now sudo'd to root. If you escape this script, remember    *
* to reduce your permissions to prevent any accidental and/or catastrophic     *
* damage to your system.                                                       *
********************************************************************************${FontReset}\n\n"
      else
        printf "${WarningMessage}Elevation to sudo failed. The required software packages cannot be installed. Exiting the script.\n\n"
        exit 0
      fi
    else
      printf "\n${FontYellow}TEST STATE ENABLED: sudo option disabled${FontReset}\n\n"
    fi
  ;;
  * )
    printf "${WarningMessage}Without sudo permissions, the prerequisite libraries cannot be installed. Exiting the script.\n\n"
    exit 0
  ;;
esac

# Prompt the user to udpate their system, providing a warning if they decline.
# First, epel-release and python-pip have to be installed before all else.
read -p "Are you ready to update your system? This is required. (y/n) " AnswerYN
case ${AnswerYN:0:1} in
  y|Y )
    printf "\nThe system will begin the update process (yum -y update) which may take some time. It also requires the installation of the epel-release RPM repository and Python's pip library.\n\n${LineBreak}"
    if [ ${TestState} -eq 0 ]
    then
      for BasePackageName in "${RequiredBasePackages[@]}"
      do
        while ! rpm -qa | grep -qw ^${BasePackageName}; do
          sudo yum -y install ${BasePackageName}
          if ! rpm -qa | grep -qw ^${BasePackageName}; then
            read -p "The package ${BasePackageName} is still not found. It may be due to network latency in downloading the software. Try again? (y/n) " AnswerYN
            case ${AnswerYN:0:1} in
              y|Y )
                printf "\n\nAttempting to install the package again.\n"
              ;;
              * )
                printf "\nWithout the required packages installed, the script cannot continue. You can try to install the package using the following command:
     ${FontYellow}sudo yum install ${BasePackageName}${FontReset}

Exiting the script.\n\n"
                exit 0
              ;;
            esac
          fi
          if rpm -qa | grep -qw python-pip; then
            sudo pip install --upgrade pip
          fi
        done
      done
    else
      printf "\n${FontYellow}TEST STATE ENABLED: sudo yum -y update${FontReset}\n"
    fi
    printf ${LineBreak}
    printf "\n${FontGreen}The system update process has completed.${FontReset} Installing third-party software packages required by EMOD.\n"
  ;;
  * )
    printf "\nWithout all of the up-to-date dependencies installed, the script cannot continue. Exiting the script.\n"
    exit 0
  ;;
esac

# Four loops to search for packages and libraries.
# The first two loops set the length of the status bar while the last two increase the user's status bar.
# Initially these tasks look like they could be combined, but the printf statement for status
# is needed and interrupts the flow.
printf "\nEMOD requires the following packages and their dependencies:"
Counter=1
StatusBar=""
for PackageRequired in "${EMODPackageRequired[@]}"
do
  printf "\n     ${Counter}. ${PackageRequired}"
  Counter=$((Counter + 1))
  StatusBar=${StatusBar}"\\u178C"
done
printf "\n\nThe following Python packages need to be installed using pip:"
Counter=1
for PIPRequired in "${EMODPythonLibraryRequired[@]}"
do
  printf "\n     ${Counter}. ${PIPRequired}"
  Counter=$((Counter + 1))
  StatusBar=${StatusBar}"\\u178C"
done

# Create two arrays of the packages that are missing.
printf "\n\nChecking to see if the required packages and libraries are on your system.\n\nStatus:\n${FontYellow}${StatusBar}${FontReset}\n"
declare -a EMODMissing
for PackageRequired in "${EMODPackageRequired[@]}"
do
  printf "${FontGreen}\\u178C${FontReset}"
  if ! rpm -qa | grep -qw ^${PackageRequired}; then
    EMODMissing+=(${PackageRequired}) 
  fi
done

declare -a EMODPIPMissing
for PIPRequired in "${EMODPythonLibraryRequired[@]}"
do
  printf "${FontGreen}\\u178C${FontReset}"
  PIPResult=`pip list | grep "$PIPRequired"`
  if [ -z "$PIPResult" ]
  then
    EMODPIPMissing+=(${PIPRequired})
  fi
done

# Once the EMODMissing array exists, present them to the user in an ordered list.
if [ ${#EMODMissing[@]} -gt 0 ]; then
  printf "\n\nThe following software required by EMOD is missing on your system:\n"
  printf "\nPackages:\n"
  Counter=1
  for PackageMissing in "${EMODMissing[@]}"
  do
    printf "\n     ${Counter}. ${PackageMissing}"
    Counter=$((Counter + 1))
  done
  printf "\n"
fi
if [ ${#EMODPIPMissing[@]} -gt 0 ]; then
  printf "\nPython Libraries:\n"
  Counter=1
  for PIPMissing in "${EMODPIPMissing[@]}"
  do
    printf "\n     ${Counter}. ${PIPMissing}"
    Counter=$((Counter + 1))
  done
  printf "\n"
fi
if [ ${#EMODMissing[@]} -gt 0 ] || [ ${#EMODPIPMissing[@]} -gt 0 ]; then
  # Prompt the user and automatically install the missing packages.
  # After the packages are installed pip is then used to install the required Python numby library.
  # Note the curl command to add the repository. This is for the git-lfs packaging.
  printf "\n"
  read -p "Are you ready to install the packages and libraries? (y/n) [This is required.] " AnswerYN
  case ${AnswerYN:0:1} in
    y|Y )
      if [ ${TestState} -eq 0 ]
      then
        printf "\n"
        sudo curl -s https://packagecloud.io/install/repositories/github/git-lfs/script.rpm.sh | sudo bash
      else
        printf "\n"
        printf "${LineBreak}"
      fi
      # Loop through the packages, checking if successfully installed each time.
      # Prompt the user if the package cannot be found. This is in place becuase some times network latency can impact the installation of packages  Trying again sometimes results in a positive installation.
      for Package in "${EMODMissing[@]}"
      do
        if [ ${TestState} -eq 0 ]
        then
          while ! rpm -qa | grep -qw ^${Package}; do
            sudo yum -y install ${Package}
            if ! rpm -qa | grep -qw ^${Package}; then
              read -p "The package ${Package} did not install correctly. Try again? (y/n) " AnswerYN
              case ${AnswerYN:0:1} in
                y|Y )
                  printf "\n\nAttempting to install the package again.\n"
                ;;
                * )
                  printf "\nWithout the required packages, the script cannot continue. You can try to install the package using the following command:
     ${FontYellow}sudo yum install ${Package}${FontReset}

Exiting the script.\n\n"
                  exit 0
                ;;
              esac
            fi
          done
        else
          printf "${FontYellow}TEST STATE ENABLED: sudo yum -y ${Package}\n"
        fi
      done
      for MissingPIP in "${EMODPIPMissing[@]}"
      do
        if [ ${TestState} -eq 0 ]
        then
          printf "\nInstalling ${MissingPIP}"
          sudo pip install ${MissingPIP}
        else
          printf "${FontYellow}TEST STATE ENABLED: sudo pip install ${MissingPIP}\n"
        fi
      done
    ;;
    * )
      printf "${WarningMessage}Without the required packages installed, EMOD will not run. Exiting the script.\n\n"
      exit 0
    ;;
  esac
else
  printf "\n\nAll of the required EMOD dependencies are on your system. No additional software needs to be installed.\n"
fi

# Ask the user if they want to download the EMOD source.
# Create a directory within their home and then prompt for GitHub credentials.
# The git client will then prompt the user for their credentials.
# Visual feedback on the clone status is displayed by the git client.
# Note the special case for the EMOD-InputData repository. This is the only
# one that uses lfs support.
printf "\nYour environment is now ready to get the EMOD source and data input files (~6GB) from GitHub.\n\n"
read -p "Do you want to download the EMOD source? (y/n)? " AnswerYN
case ${AnswerYN:0:1} in
  y|Y )
    printf "\nA new directory needs to be created for the EMOD source and the input data files. This directory will be located in your home directory and must not currently exist. Special characters will be automatically stripped.\n\n"
    DirectoryExists=0
    until [ ${DirectoryExists} -eq 1 ]
    do
      read -p "What is the name of the directory you would like to create? (Default: IDM) " NewDirectory
      if [ -z ${NewDirectory} ]
      then
        NewDirectory="IDM"
      else
        NewDirectory=${NewDirectory//[\ \!\@\#\$\%\^\&\*\(\)\=\+\[\]\\\{\}\|\;\:\'\"\,\.\/\<\>\?]/}
      fi
      if [ ! -d "${NewDirectory}" ]
      then
        DirectoryExists=1
        if [ ${TestState} -eq 0 ]
        then
          mkdir ~/${NewDirectory}
          printf "\nA directory named ${FontYellow}${NewDirectory}${FontReset} was created in your home directory.\n"
          cd ~/${NewDirectory}
        else
          printf "\n${FontYellow}TEST STATE ENABLED: Directory ${NewDirectory} not created.${FontReset}\n"
        fi
      else
        DirectoryExists=0
        printf "\nA directory named ${FontRed}${NewDirectory}${FontReset} already exists in your home directory. Please enter another name.\n"
      fi
    done

    printf "\n${LineBreak}"
    for ToDownload in "${EMODSoftware[@]}"
    do
      if [ ${TestState} -eq 0 ]
      then
        git clone ${EMODGitHubURL}${ToDownload}
      else
        printf "${FontYellow}TEST STATE ENABLED: git clone ${EMODGitHubURL}${ToDownload}${FontReset}\n"
      fi
    done
    if [ -d "EMOD-InputData" ]
    then
      cd EMOD-InputData
      git lfs fetch
      git lfs checkout
      cd ..
    fi
    cd ~
    printf "${LineBreak}"
    printf "\nThe download from GitHub has finished. The EMOD source and input data files are located at ${FontGreen}~/${NewDirectory}${FontReset}.\n"
  ;;
  * )
    printf "\nYour environment is ready but you did not download the EMOD source.\n\nIf you want to manually download the source, execute the following from your command prompt:"
    for ToDownload in "${EMODSoftware[@]}"
    do
      printf "\n     ${FontGreen}git clone ${EMODGitHubURL}${ToDownload}${FontReset}"
    done
    printf "\n"
  ;;
esac

# Define an environment variable for the .bashrc file.
# This is dynamic based upon the user's selection to download the source.
if [ ${NewDirectory} ]
then
  declare -a BashChanges=("export EMOD_ROOT=~/${NewDirectory}/EMOD" "export PATH=\$PATH:/usr/lib64/mpich/bin/" "export PATH=\$PATH:.:\$EMOD_ROOT/Scripts/")
  ln -s ~/${NewDirectory}/EMOD-InputData ~/${NewDirectory}/EMOD/InputData
else
  declare -a BashChanges=("export PATH=\$PATH:.:/usr/lib64/mpich/bin/")
fi

# Check that environment variables are in the .bashrc file.
if [ $(grep "EMOD SOFTWARE CHANGES" ~/.bashrc | wc -l) -gt 0 ]
then
  ENVChangesNeeded=0
else
  ENVChangesNeeded=1
fi

# Add variables to the .bashrc file if they aren't found.
# Display commands to the user and remind them to source the file.
if [ ${ENVChangesNeeded} -eq 1 ]
then
  printf "\nThe following PATH values need to be included in your environment file:"
  for AddToBash in "${BashChanges[@]}"
  do
    printf "\n     ${FontYellow}${AddToBash}${FontReset}"
  done
  printf "\n\n"
  read -p "Do you want to add the PATH values to your .bashrc file? (y/n) " AnswerYN
  case ${AnswerYN:0:1} in
    y|Y )
      if [ ${TestState} -eq 0 ]
      then
        echo "" >> ~/.bashrc
        echo "# BEGIN EMOD SOFTWARE CHANGES HERE" >> ~/.bashrc
        for AddToBash in "${BashChanges[@]}"
        do
          echo ${AddToBash} >> ~/.bashrc 
        done
        echo "# END EMOD SOFTWARE CHANGES HERE" >> ~/.bashrc
      else
        printf "\n${FontYellow}TEST STATE ENABLED: .bashrc changes would occur here${FontReset}"
      fi
      printf "\n\nThe environment variables have been added to your .bashrc file. You'll need to source this file for the changes to take effect during your current session.\n"
    ;;
    * )
      printf "${WarningMessage}You will need to add the following to your .bashrc file to ensure the EMOD software runs:\n\n${FontGreen}${BashChange}${FontReset}\n\n"
    ;;
  esac
else
  if [ ${TestState} -eq 0 ]
  then
    printf "\nYour .bashrc file appears to be up-to-date. No additional changes were needed.\n"
  else
    printf "${FontYellow}TEST STATE ENABLED: .bashrc file already contains values${FontReset}\n"
  fi
fi

# Display the final message, completing the script run.
printf "\n${FontGreen}Your EMOD environment set-up is complete.${FontReset}
Remember to source your .bashrc files to make the environment changes available for this session.
For instructions on building the EMOD executable, go to the IDM documentation at ${FontGreen}http://idmod.org/idmdoc/#EMOD/EMODBuildAndRegression/MonolithicSCons.htm${FontReset}.\n\n"
