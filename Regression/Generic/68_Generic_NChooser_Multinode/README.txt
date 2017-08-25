GH-1607
NChooser incorrectly re-scales for each node instead of just once
6/1/2017

-----

DanK ran into an issue where he had a scenario with multiple nodes
 and a Base_Population_Scale_Factor < 1.0. The scale factor was being 
applied for each node instead of just once.

-----

This test is based on 22_Generic_NChooser.  It takes that test but splits
the population across two nodes.  The results should be very similar even
without migration.

The "Human Infectious Reservoir" is half what it is for test 22 because
it is an average per node. That is, the value in this test should be half
what it is in test 22 because this test has two nodes.