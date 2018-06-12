/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "STIInterventionsContainer.h"

#include "IIndividualHuman.h"

#include "SimulationEnums.h"
#include "InterventionFactory.h"
#include "Log.h"
#include "Sugar.h"
#include "IndividualSTI.h"
#include "IRelationshipParameters.h"

SETUP_LOGGING( "STIInterventionsContainer" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(STIInterventionsContainer, InterventionsContainer)
        HANDLE_INTERFACE(ISTIInterventionsContainer)
        HANDLE_INTERFACE(ISTIBarrierConsumer)
        HANDLE_INTERFACE(ISTICircumcisionConsumer)
        HANDLE_INTERFACE(ISTICoInfectionStatusChangeApply)
    END_QUERY_INTERFACE_DERIVED(STIInterventionsContainer, InterventionsContainer)

    STIInterventionsContainer::STIInterventionsContainer() 
    : InterventionsContainer()
    , is_circumcised(false)
    , circumcision_reduced_require(0.0)
    , STI_blocking_overrides()
    {
    }

    STIInterventionsContainer::~STIInterventionsContainer()
    {
    }
    
    void
    STIInterventionsContainer::UpdateSTIBarrierProbabilitiesByType(
        RelationshipType::Enum rel_type,
        const Sigmoid& config_overrides
    )
    {
        STI_blocking_overrides[ rel_type ] = config_overrides;
    }

    const Sigmoid&
    STIInterventionsContainer::GetSTIBarrierProbabilitiesByRelType(
        const IRelationshipParameters* pRelParams
    )
    const
    {
        if( STI_blocking_overrides.find( pRelParams->GetType() ) != STI_blocking_overrides.end() )
        {
            LOG_DEBUG( "Using override condom config values from campaign.\n" );
            return STI_blocking_overrides.at( pRelParams->GetType() );
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
        // Need to get gender
        IIndividualHuman *ih = nullptr;
        if( s_OK != parent->QueryInterface(GET_IID(IIndividualHuman), (void**) &ih) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHuman", "IIndividualHuman" );
        }

        if( ih->GetGender() == Gender::FEMALE )
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
        ar.labelElement("STI_blocking_overrides"      ); serialize_overrides( ar, container.STI_blocking_overrides );
    }
}

