The PrepareLinuxEnvironment script is an example of creating an environment for using the Epidemiological MODeling software (EMOD). The script was designed and tested to run on an Azure CentOS 7 virtual machine.

For Windows users, go to https://institutefordiseasemodeling.github.io/EMOD/general/dev-install-overview.html for instructions on downloading the EMOD source and the prerequisite software.

PrepareLinuxEnvironment.sh is an interactive script that will ask if you agree to the following:
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

Before you run the script you'll need to execute the following:

     chmod 755 PrepareLinuxEnvironment.sh

To run PrepareLinuxEnvironment.sh, execute the following at the command prompt:

     ./PrepareLinuxEnvironment.sh

If you want to walk through the script workflow but make no permanent system changes or download any files, you can run the script in test mode.

     ./PrepareLinuxEnvironment.sh test

Please note, it is recommended to read the script using a monotype.

The following diagram shows the program flow of the PrepareLinuxEnvironment script.

                        +-------+
                        | Start |
                        +-------+
                            |
                    +----------------+
                    | Display Banner |
                    +----------------+
                            |
                    +----------------+
                    | CentOS 7 check |->--no---------------------->+
                    +----------------+                             |
                            |                                      |
                           yes                                     |
                            |                                      |
                    +-----------------+                            |
                    | Prompt to begin |->--no--------------------->+
                    +-----------------+                            |
                            |                                      |
                           yes                                     |
                            |                                      |
                +------------------------+                         |
                | Enter sudo credentials |->--no/fail------------->+
                +------------------------+                         |
                            |                                      |
                           yes                                     |
                            |                                      |
                   +------------------+                            |
                   | Prompt to update |->--no--------------------->+
                   +------------------+                            |
                            |                                      |
                           yes                                     |
                            |                                      |
             +-----------------------------+                       |
             | yum -y install epel-release |                       |
             | yum -y install python-pip   |                       |
             | Upgrade pip                 |                       |
             +-----------------------------+                       |
                            |                                      |
           +----------------------------------+                    |
           | Check for EMOD required packages |                    |
           +----------------------------------+                    |
                            |                                      |
             +-----------------------------+                       |
   +---no--<-| Need EMOD required packages |->--no---------------->|
   |         +-----------------------------+                       |
   |                        |                                      |
   |                       yes                                     |
   |                        |                                      |
   |   +------------------------------------------+                |
   |   | Prompt to install packages and libraries |->--no--------->+
   |   +------------------------------------------+                |
   |                        |                                      |
   |                       yes                                     |
   |                        |                                      |
   |        +-------------------------------+                      |
   |        | Add git-lfs repository        |                      |
   |        | yum -y install [package name] |                      |
   |        | pip install [library name]    |                      |
   |        +-------------------------------+                      |
   |                        |                                      |
   +----------------------->+                                      |
                            |                                      |
           +----------------------------------+                    |
           | Prompt to download EMOD software |->--no----------+   |
           +----------------------------------+                |   |
                            |                                  |   |
                           yes                                 |   |
                            |                                  |   |
                            +<-----------------------------+   |   |
                            |                              |   |   |
          +--------------------------------------+         |   |   |
          | Prompt and create a source directory |->--fail-+   |   |
          +--------------------------------------+             |   |
                            |                                  |   |
                         created                               |   |
                            |                                  |   |
     +------------------------------------------------+        |   |
     | Download EMOD software from GitHub (git clone) |        |   |
     | LFS fetch the input files                      |        |   |
     +------------------------------------------------+        |   |
                            |                                  |   |
                            +<---------------------------------+   |
                            |                                      |
            +---------------------------------+                    |
            | Check for environment variables |                    |
            +---------------------------------+                    |
                            |                                      |
           +-----------------------------------+                   |
           | Prompt to update the .bashrc file |->--no-----+       |
           +-----------------------------------+           |       |
                            |                              |       |
                           yes                             |       |
                            |                              |       |
               +-------------------------+                 |       |
               | Update the .bashrc file |                 |       |
               +-------------------------+                 |       |
                            |                              |       |
                            +<-----------------------------+       |
                            |                                      |
                +-----------------------+                          |
                | Present final message |                          |
                +-----------------------+                          |
                            |                                      |
                            +<-------------------------------------+
                            |
                         +------+
                         | Done |
                         +------+

[end of the readme.txt file]
