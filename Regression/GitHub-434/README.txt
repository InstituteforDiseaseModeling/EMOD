May 2, 2016
GitHub-434
https://github.com/InstituteforDiseaseModeling/DtkTrunk/issues/434

It turns out this is due to a problem in MpiDataExchanger. 
We need to add the sending of the size_request to the outbound_requests 
so that we wait for the receiving cores to get the size value.

===============================
Example Error Message Below
===============================


00:07:15 [0] [I] [Simulation] Update(): Time: 2846.0 Rank: 0 StatPop: 327 Infected: 124

job aborted:
[ranks] message

[0-2] fatal error
Fatal error in MPI_Recv: Other MPI error, error stack:
MPI_Recv(buf=0x000000000603EE40, count=1, MPI_UNSIGNED, src=3, tag=0, MPI_COMM_WORLD, status=0x000000000603EDB8) failed
Out of memory

[3] terminated

[4] fatal error
Fatal error in MPI_Recv: Other MPI error, error stack:
MPI_Recv(buf=0x000000000603EE40, count=1, MPI_UNSIGNED, src=3, tag=0, MPI_COMM_WORLD, status=0x000000000603EDB8) failed
Out of memory

[5] terminated

[6] fatal error
Fatal error in MPI_Recv: Other MPI error, error stack:
MPI_Recv(buf=0x000000000603EE40, count=1, MPI_UNSIGNED, src=3, tag=0, MPI_COMM_WORLD, status=0x000000000603EDB8) failed
Out of memory

[7-12] terminated

[13] fatal error
Fatal error in MPI_Recv: Other MPI error, error stack:
MPI_Recv(buf=0x000000000603EE40, count=1, MPI_UNSIGNED, src=3, tag=0, MPI_COMM_WORLD, status=0x000000000603EDB8) failed
Out of memory

[14-23] terminated

---- error analysis -----

[0-2,4,6,13] on IDMPPHPC01-0016
mpi has detected a fatal error and aborted \\idmppfil01\IDM\home\jgerardin\bin\4ef6c5c0f0615f3e3e145ab935d4e7ac\Eradication.exe

---- error analysis -----
