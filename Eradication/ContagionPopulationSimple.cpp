
#include "stdafx.h"

#include "ContagionPopulationSimple.h"
#include "IStrainIdentity.h"
#include "Log.h"

SETUP_LOGGING( "ContagionPopulationSimple" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(ContagionPopulationSimple)
        HANDLE_INTERFACE(IContagionPopulation)
    END_QUERY_INTERFACE_BODY(ContagionPopulationSimple)

    ContagionPopulationSimple::ContagionPopulationSimple( int antigenID, float quantity )
        : m_ContagionQuantity( quantity )
        , m_AntigenID( antigenID )
    {
    }

    ContagionPopulationSimple::ContagionPopulationSimple( IStrainIdentity * strain, float quantity )
        : m_ContagionQuantity( quantity )
        , m_AntigenID( strain->GetAntigenID() )
    {
    }

    int ContagionPopulationSimple::GetAntigenID( void ) const
    {
        return m_AntigenID;
    }

    float ContagionPopulationSimple::GetTotalContagion( void ) const
    {
        return m_ContagionQuantity;
    }

    bool ContagionPopulationSimple::ResolveInfectingStrain( IStrainIdentity* strainId ) const
    {
        strainId->SetAntigenID( m_AntigenID );
        strainId->SetGeneticID( 0 );

        return true;
    }
}
