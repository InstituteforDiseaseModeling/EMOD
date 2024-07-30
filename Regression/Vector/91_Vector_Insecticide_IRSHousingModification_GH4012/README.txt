GH-4012
1/14/2020

ISSUE
Basically, if the killing_modifier or blocking_modifier times the mathcing
initial_effect in the campaign file is bigger than 1, I get an error of

IllegalOperationException:
Exception in Eradication\Eradication.cpp at 78 in FPE_SignalHandler.
Floating Point Exception, signal: 8.


RESOLUTION
The problem turned out to be in RANDOMBASE::binomial_approx().  One probability
in VectorPopulation was small and it was used as a divisor.  This created a 
"probability" >> 1 and binomial_approx() did not handle it well.  By adding code
in binomial_approx() to ensure the probability is between 0 and 1, the 
Floating Point exception is resolved.

As far as the vector code generating a "probability" > 1.0, it is not obvious 
how to ensure the calculation doesn't generate a value > 1.  Since the value is 
only given to binomial_approx(), it seemed reasonable to solve this problem and 
protect against other future issues.

