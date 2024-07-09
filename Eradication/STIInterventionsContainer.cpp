
#include "stdafx.h"

#include "STIInterventionsContainer.h"

#include "IIndividualHuman.h"

#include "SimulationEnums.h"
#include "InterventionFactory.h"
#include "Log.h"
#include "Sugar.h"
#include "IndividualSTI.h"
#include "IRelationshipParameters.h"
#include "IndividualEventContext.h"


SETUP_LOGGING( "STIInterventionsContainer" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(STIInterventionsContainer, InterventionsContainer)
        HANDLE_INTERFACE(ISTIInterventionsContainer)
        HANDLE_INTERFACE(ISTIBarrierConsumer)
        HANDLE_INTERFACE(ISTICircumcisionConsumer)
        HANDLE_INTERFACE(ISTICoInfectionStatusChangeApply)
        HANDLE_INTERFACE(ICoitalActRiskData)
    END_QUERY_INTERFACE_DERIVED(STIInterventionsContainer, InterventionsContainer)

    STIInterventionsContainer::STIInterventionsContainer() 
    : InterventionsContainer()
    , is_circumcised(false)
    , circumcision_reduced_require(0.0)
    , risk_modifier_acquisition( 1.0 )
    , risk_modifier_transmission( 1.0 )
    , has_blocking_overrides()
    , STI_blocking_overrides()
    , snr_inv_list()
    {
        has_blocking_overrides.resize( RelationshipType::COUNT, false );
        STI_blocking_overrides.resize( RelationshipType::COUNT, Sigmoid() );
    }

    STIInterventionsContainer::~STIInterventionsContainer()
    {
    }

    void STIInterventionsContainer::Update( float dt )
    {
        // reset these to 1 so that the be modified by interventions
        risk_modifier_acquisition  = 1.0;
        risk_modifier_transmission = 1.0;

        std::fill( has_blocking_overrides.begin(), has_blocking_overrides.end(), false );

        InterventionsContainer::Update( dt );
    }

    void STIInterventionsContainer::AddNonPfaRelationshipStarter( INonPfaRelationshipStarter* pSNR )
    {
        snr_inv_list.push_back( pSNR );
    }

    void STIInterventionsContainer::StartNonPfaRelationships()
    {
        for( auto p_snr : snr_inv_list )
        {
            p_snr->StartNonPfaRelationship();
        }
        snr_inv_list.clear();
    }

    void
    STIInterventionsContainer::UpdateSTIBarrierProbabilitiesByType(
        RelationshipType::Enum rel_type,
        const Sigmoid& config_overrides
    )
    {
        if( parent->GetEventContext()->GetGender() == Gender::FEMALE )
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Females cannot have STIBarrier." );
        }

        has_blocking_overrides[ rel_type ] = true;
        STI_blocking_overrides[ rel_type ] = config_overrides;
    }

    const Sigmoid&
    STIInterventionsContainer::GetSTIBarrierProbabilitiesByRelType(
        const IRelationshipParameters* pRelParams
    )
    const
    {
        if( has_blocking_overrides[ pRelParams->GetType() ] )
        {
            LOG_DEBUG( "Using override condom config values from campaign.\n" );
            return STI_blocking_overrides[ pRelParams->GetType() ];
        }
        else
        {
            LOG_DEBUG( "Using static (default/config.json) condom config values.\n" );
            return pRelParams->GetCondomUsage();
        }
    }

    float
    STIInterventionsContainer::GetInterventionReducedAcquire()
    const
    {
        return drugVaccineReducedAcquire;
    }

    float
    STIInterventionsContainer::GetInterventionReducedTransmit()
    const
    {
        return drugVaccineReducedTransmit;
    }

    bool STIInterventionsContainer::IsCircumcised( void ) const 
    {
        return is_circumcised;
    }

    float STIInterventionsContainer::GetCircumcisedReducedAcquire() const
    {
        return circumcision_reduced_require;
    }

    void STIInterventionsContainer::ApplyCircumcision( float reduceAcquire ) 
    {
        if( parent->GetEventContext()->GetGender() == Gender::FEMALE )
        {
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Females cannot be circumcised." );
        }

        circumcision_reduced_require = reduceAcquire;
        is_circumcised = true;
    }

    void STIInterventionsContainer::ChangeProperty( const char *prop, const char* new_value)
    {
        IIndividualHumanSTI *ihsti = nullptr;
        if( s_OK != parent->QueryInterface(GET_IID(IIndividualHumanSTI), (void**) &ihsti) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanContext", "IIndividualHumanSTI" );
        }
        ihsti->UpdateSTINetworkParams(prop, new_value);
        ihsti->ClearAssortivityIndexes();

        InterventionsContainer::ChangeProperty( prop, new_value);
    }

    void
    STIInterventionsContainer::SpreadStiCoInfection()
    {
        IIndividualHumanSTI *ihsti = nullptr;
        if( s_OK != parent->QueryInterface(GET_IID(IIndividualHumanSTI), (void**) &ihsti) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanContext", "IIndividualHumanSTI" );
        }
        ihsti->SetStiCoInfectionState();
    }

    void
    STIInterventionsContainer::CureStiCoInfection()
    {
        IIndividualHumanSTI *ihsti = nullptr;
        if( s_OK != parent->QueryInterface(GET_IID(IIndividualHumanSTI), (void**) &ihsti) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanContext", "IIndividualHumanSTI" );
        }
        ihsti->ClearStiCoInfectionState();
    }

    void STIInterventionsContainer::UpdateCoitalActRiskFactors( float acqMod, float tranMod )
    {
        risk_modifier_acquisition  *= acqMod;
        risk_modifier_transmission *= tranMod;
    }

    float STIInterventionsContainer::GetCoitalActRiskAcquisitionFactor() const
    {
        return risk_modifier_acquisition;
    }

    float STIInterventionsContainer::GetCoitalActRiskTransmissionFactor() const
    {
        return risk_modifier_transmission;
    }

    REGISTER_SERIALIZABLE(STIInterventionsContainer);

    void serialize_overrides( IArchive& ar, std::map< RelationshipType::Enum, Sigmoid >& blocking_overrides )
    {
        size_t count = ar.IsWriter() ? blocking_overrides.size() : -1;

        ar.startArray(count);
        if (ar.IsWriter())
        {
            for (auto& entry : blocking_overrides)
            {
                std::string key = RelationshipType::pairs::lookup_key( entry.first );
                ar.startObject();
                    ar.labelElement("key"  ) & key;
                    ar.labelElement("value") & entry.second;
                ar.endObject();
            }
        }
        else
        {
            for (size_t i = 0; i < count; ++i)
            {
                std::string key;
                Sigmoid value;
                ar.startObject();
                    ar.labelElement("key"  ) & key;
                    ar.labelElement("value") & value;
                ar.endObject();
                RelationshipType::Enum rt = (RelationshipType::Enum)RelationshipType::pairs::lookup_value( key.c_str() );
                blocking_overrides[ rt ] = value;
            }
        }
        ar.endArray();
    }

    void STIInterventionsContainer::serialize(IArchive& ar, STIInterventionsContainer* obj)
    {
        InterventionsContainer::serialize( ar, obj );
        STIInterventionsContainer& container = *obj;
        ar.labelElement("is_circumcised"              ) & container.is_circumcised;
        ar.labelElement("circumcision_reduced_require") & container.circumcision_reduced_require;
        ar.labelElement("risk_modifier_acquisition"   ) & container.risk_modifier_acquisition;
        ar.labelElement("risk_modifier_transmission"  ) & container.risk_modifier_transmission;
        ar.labelElement("has_blocking_overrides"      ) & container.has_blocking_overrides;
        ar.labelElement("STI_blocking_overrides"      ) & container.STI_blocking_overrides;
        ar.labelElement("snr_inv_list"                ) & container.snr_inv_list;
    }
}

