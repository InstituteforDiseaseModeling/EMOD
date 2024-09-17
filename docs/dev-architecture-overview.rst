=====================
|EMOD_s| architecture
=====================

|EMOD_l| is implemented in C++ and has two subsystems: the environment and the simulation controller.
The environment contains the interfaces to the simulation controller subsystem and the external
inputs and outputs. The simulation controller contains the epidemiological model (simulation and
campaign management), and reporters that capture output data and create reports from a simulation.
The following figure shows a high-level view of the system components of |EMOD_s| and how they are
related to each other.

.. figure:: images/dev/ArchSystemComponents.png

    |EMOD_s| system components

.. include:: reuse/warning-schema.txt

.. toctree::
   :maxdepth: 3

   dev-architecture-environment
   dev-architecture-sim-controller
