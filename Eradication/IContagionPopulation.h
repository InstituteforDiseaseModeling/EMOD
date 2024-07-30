
#pragma once

#include "ISupports.h"

namespace Kernel
{
    struct IStrainIdentity;

    // An object that implements this interface is allowing the user to select
    // a strain from a population.  It is used by IInfectable which in turn is
    // used by ITransmissionGroups.  ITransmissionGroups will ExposeToContagion()
    // an IInfectable entity.  It will give to the IInfectable a IContagionPopulation
    // so that the IInfectable can determine if it got infected and by what strain.
    struct IContagionPopulation: public ISupports
    {
        virtual int  GetAntigenID(void) const = 0;
        virtual float GetTotalContagion( void ) const = 0;
        virtual bool ResolveInfectingStrain( IStrainIdentity* strainId ) const = 0;
    };
}
