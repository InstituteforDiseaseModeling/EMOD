5/10/2017
GH-1289 - Community Health Worker (CHW) gives out more interventions than directed.

----
Svetlana - Jaline found the issue and I was able to repro it. Attaching the files. The issue first happens
at time step 60 and then repeats several times, with the worst being around time step 415 where 
up to 36 out of 5 interventions were given.

DanB - Hmm. The code is not working quite right but the idea was to maintain an average rate, not a max rate. 
For example, if Distributation_Rate is 5, CHW could hand out 4 one day and 6 the next to achieve 5 per day.

Jaline - I'd wanted a max rate, since CHWs realistically have a max capacity that doesn't increase if they 
had lower demand earlier.
----
If you run the code before this change, you should see distributions of > 50 (Max_Distributions_Per_Day)
before it runs out of stock.  With the change, it does not distribute more than 50.