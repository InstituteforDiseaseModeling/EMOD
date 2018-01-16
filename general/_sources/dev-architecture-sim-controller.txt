===============================
Simulation controller subsystem
===============================

The simulation controller is the top-level structure for the epidemiological model. The controller’s
capabilities are simple, running a single simulation in a single time direction at a constant
rate. It exists to support future capabilities, such as running multiple simulations, pausing a
simulation, or bootstrapping a simulation from an archived simulation. It contains two components:
simulation and reporters.

The following figure shows a high-level view of the system components of |EMOD_s| and how they are
related to each other.

.. figure:: ../images/dev/ArchSystemComponents.png

    |EMOD_s| system components

Simulation
==========

The simulation component contains core functionality that models the behavior of a disease without any
interventions and extended functionality to include migration, climate, or other input data to
create a more realistic simulation. Disease transmission may be more or less complex depending on
the disease being modeled.

Campaign management
-------------------

The simulation component also includes a campaign manager sub-component for including disease
interventions in a simulation.

Reporter
========

The reporter component creates output reports for both simulation-wide aggregate reporting and
spatial reporting.


.. toctree::
   :maxdepth: 2

   dev-architecture-core
   dev-architecture-campaigns
   dev-architecture-reporter
