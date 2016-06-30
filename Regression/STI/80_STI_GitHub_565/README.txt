6-20-2016
GitHub-565
Crashes on assertion failure: Assertion failure, (partner), is false in file Eradication\Relationship.cpp at line 400

----
Crashes on assertion failure when running this attached sim
RelationshipCpp400Assert.zip:

Assertion failure, (partner), is false in file Eradication\Relationship.cpp at line 400
job aborted:
[ranks] message
[0] process exited without calling finalize

CallStack:

    msvcr110.dll!000007feefb7299c() Unknown
    msvcr110.dll!000007feefb6d3d2() Unknown
    msvcr110.dll!000007feefb740da() Unknown
>   Eradication.exe!onAssert__(const char * filename, int lineNum, const char * variable) Line 23   C++
    Eradication.exe!Kernel::Relationship::GetPartner(Kernel::IIndividualHumanSTI * individual) Line 400 C++
    Eradication.exe!Kernel::StiTransmissionReporter::onTransmission(Kernel::IIndividualHuman * individual) Line 105 C++
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
    kernel32.dll!00000000778359bd() Unknown
    ntdll.dll!000000007796a2e1()    Unknown
Repro
Extract attached zip and navigate to the extracted directory.
-C config.json -I \bayesianfil01\idm\public\input\tip\MigrationTest -O testing

This sim uses STI/60_STI_All_Rel_Types_All_Rel_States as a base. An outbreak event was added and Base_Infectivity was cranked up.
 Also has a couple reports enabled (Report_Coital_Acts and Report_Transmission).

Result
Assertion failure, (partner), is false in file Eradication\Relationship.cpp at line 400
job aborted:
[ranks] message
[0] process exited without calling finalize
----

The issue here is that the couple consummates and then one of them decides to migrate and Pause the relationship. 
The Transmission report is then updated after these two events.

The solution was to not set the male/female partner to null when pausing.  However, this required checking if
the partner is absent before using the pointer to the partner.