GH-4730 - SpaceSpraying works slightly different for track_all vs compartment sampling when initial_effect = 1

One could get similar results between track_all and compartment as long as the effect was < 1.
With a value of 1 for the effect compartment/cohort, just killed all of the vectors.
It was supposed to only kill the ones that were gestating.  SpaceSpraying kills only the 
vectors that are not feeding - i.e. the ones that are gestating.