GitHub-459
Can Only Call initConfigComplexType() Within Configure()

initConfigComplexType() makes use of a static variable (IJsonConfigurable::generic_container)
that Configure() uses and clears. If this method is used in a constructor and Configure() is
not called immediately another class could be called and think that the data in the static
variable belongs to that class.

The situation that made this occur was:

- Individual had HIVPiecewiseByYearAndSexDiagnostic intervention
- Individual migrated
- HIVPiecewiseByYearAndSexDiagnostic was constructed as part of serialization
- generic_container was updated in the constructor for Time_Value_Map
- Configure() is not called
- NLHTIV created ARTDropout and ARTDropout that it had a Time_Value_Map

Solution A: Move initConfigComplexType() calls to Configure()m - Low immediate risk
Solution B: Try to make generic_container non-static - High immediate risk