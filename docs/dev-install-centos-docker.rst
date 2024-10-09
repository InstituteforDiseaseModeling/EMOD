==============================================================================
Getting started with building |EMOD_s| source code from |Centos| Docker images
==============================================================================

The |EMOD_s| source code can be built from the following |Centos| Docker images available on JFrog Artifactory:

.. csv-table::
   :header: Image name, Repository path, Description
   :widths: 5, 8, 10

   dtk-build, docker-production.packages.idmod.org/idm/centos:dtk-build, |EMOD_s| source code to build and run the |exe_l|.
   dtk-runtime, docker-production.packages.idmod.org/idm/centos:dtk-runtime, |EMOD_s| files needed for running a pre-built |Centos| |exe_l|.
   dtk-sfts, docker-production.packages.idmod.org/idm/centos:dtk-sfts, |EMOD_s| source code to build test and run the |exe_l|.

You can build these images on Windows and Linux machines. The following steps were tested and documented from machines running Linux for |Centos| 7.7 and Windows 10.

Prerequisites
-------------

* privileges to install packages (**sudo** on Linux, admin on Windows)
* An Internet connection
* Git client
* Docker client

To verify whether you have Git and Docker clients installed you can type the following at a command line prompt::

    git --version
    docker --version

Download and install prerequisites
----------------------------------

Git version 1.8.3.1 and Docker CE version 19.03.3 were used for creating this documentation.

To download and install a Git client, see https://git-scm.com/download.

To download and install a Docker client for |Centos|, see https://docs.docker.com/install/linux/docker-ce/centos/
To download and install a Docker client for Windows, see https://docs.docker.com/docker-for-windows/install/

Clone |EMOD_s| source code
--------------------------

To clone |EMOD_s| source code, type the following at a command line prompt::

    git clone https://github.com/InstituteForDiseaseModeling/EMOD.git

The next steps are to run the Docker container and build the |EMOD_s| source code from the |Centos| Docker images. If your host machine is running Linux, see :doc:`dev-install-centos-docker-linux`. If your host machine is running Windows, see :doc:`dev-install-centos-docker-win`.


.. toctree::
   :maxdepth: 2

   dev-install-centos-docker-linux
   dev-install-centos-docker-win
