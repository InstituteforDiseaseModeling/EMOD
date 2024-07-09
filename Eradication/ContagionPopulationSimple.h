
#pragma once

#include "IContagionPopulation.h"

namespace Kernel
{
    // This is a very simple implementation of the IContagionPopulation.
    // It should be used when the IInfectable does not need to select a
    // specific strain.
    class ContagionPopulationSimple : public IContagionPopulation
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
    public:
        ContagionPopulationSimple( int antigenID, float quantity );
        ContagionPopulationSimple( IStrainIdentity * strain, float quantity );

        //IContagionPopulation
        virtual int  GetAntigenID(void) const override;
        virtual float GetTotalContagion( void ) const override;
        virtual bool ResolveInfectingStrain( IStrainIdentity* strainId ) const override;

    protected:
        float m_ContagionQuantity;
        int   m_AntigenID;
    };
}
