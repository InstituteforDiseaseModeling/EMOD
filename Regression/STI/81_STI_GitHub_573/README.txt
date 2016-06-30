6-22-2016
GitHub-573
Crash on Assertion failure, (relationships.size()), is false in file Eradication\StiTransmissionReporter.cpp at line 99

----
Crashes on the following assertion failure when running the attached sim
LinkedForSimId_8f5ba631-0138-e611-93fa-f0921c167864.zip

Assertion failure, (relationships.size()), is false in file Eradication\StiTransmissionReporter.cpp at line 99

Call stack:

>   Eradication.exe!onAssert__(const char * filename, int lineNum, const char * variable) Line 23   C++
    Eradication.exe!Kernel::StiTransmissionReporter::onTransmission(Kernel::IIndividualHuman * individual) Line 99  C++
    Eradication.exe!Kernel::Node::reportNewInfection(Kernel::IIndividualHuman * ih) Line 2538   C++
    Eradication.exe!Kernel::Node::updateNodeStateCounters(Kernel::IIndividualHuman * ih) Line 2525  C++
    Eradication.exe!Kernel::Node::Update(float dt) Line 1402    C++
    Eradication.exe!Kernel::NodeSTI::Update(float dt) Line 189  C++
    Eradication.exe!Kernel::Simulation::Update(float dt) Line 694   C++
    Eradication.exe!RunSimulation<Kernel::ISimulation>(Kernel::ISimulation & sim, int steps, float dt) Line 258 C++
    Eradication.exe!DefaultController::execute_internal() Line 537  C++
    Eradication.exe!ControllerInitWrapper(int argc, char * * argv, IdmMpi::MessageInterface * pMpi) Line 622    C++
    Eradication.exe!MPIInitWrapper(int argc, char * * argv) Line 211    C++
    Eradication.exe!main(int argc, char * * argv) Line 161  C++
    Eradication.exe!__tmainCRTStartup() Line 536    C

Repro
Extract attached zip and navigate to the extracted directory.
<Path to Eradication.exe> -C config.json -I \\bayesianfil01\idm\public\input\tip\MigrationTest -O testing

This sim uses STI/60_STI_All_Rel_Types_All_Rel_States as a base. An outbreak event was added and Base_Infectivity
was cranked up to 0.5. Also has a couple reports enabled (RelationshipStart and Transmission).

Result
Assertion failure, (relationships.size()), is false in file Eradication\StiTransmissionReporter.cpp at line 99
job aborted:
[ranks] message
[0] process exited without calling finalize
----

The problem turned out to be that a couple could consumate, a partner decide to migrate and terminate the
relationship, and then the transmission report would try to update based on new infections.  To solve the problem,
we moved when the report gets updated.  Instead of waiting for the NewInfection event, we broadcast a new STINewInfection
event when the infection is acquired.  This allows the report to be updated immediately instead of waiting until other
things can change the state of the individual.
