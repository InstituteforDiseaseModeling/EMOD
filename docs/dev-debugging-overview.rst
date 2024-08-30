=====================
Debugging and testing
=====================

When you build |exe_s| or |linux_binary| from the |EMOD_s| source code, it's important to debug the
code and run regression tests to be sure your changes didn't inadvertently change |EMOD_s| functionality.
You can run simulations directly from Visual Studio to step through the code, troubleshoot any build
errors you encounter, and run the standard |EMOD_s| regression tests or create a new regression test.

.. include:: reuse/warning-schema.txt

.. toctree::
   :maxdepth: 2

   dev-logging
   dev-simulations
   dev-trouble-build
   dev-regression
   dev-sft
