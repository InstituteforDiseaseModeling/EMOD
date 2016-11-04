GitHub-620
7/6/2016
App crashes from assertion failure in Eradication\NChooserEventCoordinator.cpp

----
App crashes running the attached sim from the following assertion failure:

Assertion failure, (initialTimeStep < numTimeSteps), is false in file Eradication\NChooserEventCoordinator.cpp at line 191

Repro
Run the following sim:
GitHub-620.zip

Result
Crashes from Assertion failure, (initialTimeStep < numTimeSteps), is false in file Eradication\NChooserEventCoordinator.cpp at line 191
----

Needed to change GetCurrentInDays() in generic to use currentTime.time instead of currentTime.Year().
If this was used in generic, Year() returns the number of days from the beginning - i.e. base_year = 0.