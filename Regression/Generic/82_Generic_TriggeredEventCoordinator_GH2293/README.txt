GH-2293
Surveillance: "Brittany'sBug": If the triggered events are out of order, they do not hear the events they listen for
3/17/2018

----
This is very strange, looks like the ability of Coordinator events to hear events depends on where they are in the campaign file?

the two campaign files are only different in the order of the Coordinators (in this case triggered coordinators with diagnostic attached).

if you look at the node and coordinator event recorders you can see that in the "broken" campaign, the triggered diagnostics do not run 
even though the triggered they react to are being sent out the "Decision_Diagnostic_{}" are triggered by the Vaccinate_{}, and in the 
"broken" campaign are listed before the coordinator that sends out the trigger, which happens and should run, but don't, if you re-arrange 
the campaings where the Decision_Diagnostic follows the surveillancecoordinatr with the Vaccinate_{} trigger, they run.

BrittanyBug.zip

I understand that things don't happen all at once, but we should have triggered listeners hear all the events. 
This might be a known issue, but I haven't ran into this before.

----
The solution was to just set a starting/stopping flag in the notifyOnEvent() and reset the Standard Event Coordinator
parameters in the Update() method.  This means it will:
- get told to start in UpdateNodes(), 
- wait until the next time step when Update() is called, 
- reset the standard event coordinator parameters,
- distribute stuff in the UpdateNodes() method
This causes a one timestep delay between when it is told to start and when it does.

----

