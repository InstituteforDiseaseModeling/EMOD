October 11, 2016
GH-798
https://github.com/InstituteforDiseaseModeling/DtkTrunk/issues/798
NLHTIV - Eradiation.exe crashes when distributing node level intervention Outbreak to a VECTOR_SIM

===============
When running in debug you get an assertion failure.

===============
The problem is that Outbreak adds new infected people while we are iterating over the list of humans.

In Node::Update(), we had

        for( auto individual : individualHumans )
        {
            individual->Update(GetTime().time, dt);
            ...
        }

However, if an event occures during Individual::Update(), it will notify NLHTIV and it will attempt to distribute
the intervention.  Since the intervention is Outbreak, it adds a person to individualHumans while we are iterating
here in Node::Update().  If we change the 'for' statement to,

for( int i = 0 ; i < individualHumans.size() ; ++i )

the issue goes away.

I'm a little concerned about people being removed.  This solution would not work in that case.  The main way people
die or migrate and are removed from the list happens after this loop.  I think we are ok.


===============
