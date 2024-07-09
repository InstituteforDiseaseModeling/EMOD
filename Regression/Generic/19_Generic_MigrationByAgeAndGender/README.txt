108_Generic_MigrationByAgeAndGender

This test checks the ability to control migration by age and gender.
It uses the Local Migration file to direct
    * Males to odd nodes
    * Females to even nodes.

So that we can test having multiple files, it uses the Regional Migration
file to direct people
    *  0 < age <  40 to nodes 1, 2, 3 (young)
    * 40 < age <  80 to nodes 4, 5, 6 (middle-age)
    * 80 < age < 125 to nodes 7, 8, 9 (seniors)

The result should be:
    * young males in nodes 1 & 3
    * young females in node 2
    * middle-age males in node 5
    * middle-age females in nodes 4 & 6
    * senior males in nodes 7 & 9
    * senior females in node 8

The campaign file causes an outbreat on day 20 when the majority of the
migration has occured.  Hence, there is almost no infection outside the
targeted node/demographic.

One can use the ReportNodeDemographic.csv to see this.
