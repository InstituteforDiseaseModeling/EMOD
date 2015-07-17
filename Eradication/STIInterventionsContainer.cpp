/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include "STIInterventionsContainer.h"

#include "IIndividualHuman.h"

//#include "Drugs.h" // for IDrug interface
#include "SimulationEnums.h"
#include "InterventionFactory.h"
#include "Log.h"
#include "SimpleTypemapRegistration.h"
#include "Sugar.h"
#include "IndividualSTI.h"

static const char * _module = "STIInterventionsContainer";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(STIInterventionsContainer, InterventionsContainer)
        HANDLE_INTERFACE(ISTIInterventionsContainer)
        HANDLE_INTERFACE(ISTIBarrierConsumer)
        HANDLE_INTERFACE(ISTICircumcisionConsumer)
        HANDLE_INTERFACE(ISTICoInfectionStatusChangeApply)
    END_QUERY_INTERFACE_DERIVED(STIInterventionsContainer, InterventionsContainer)

    STIInterventionsContainer::STIInterventionsContainer() :
        InterventionsContainer(),
        is_circumcised(false)
    {
    }

    STIInterventionsContainer::~STIInterventionsContainer()
    {
    }
    
    void STIInterventionsContainer::Update(float dt)
    {
        InterventionsContainer::Update(dt);
    }

    // For now, before refactoring Drugs to work in new way, just check if the intervention is a
    // Drug, and if so, add to drugs list. In future, there will be no drugs list, just interventions.
    bool STIInterventionsContainer::GiveIntervention(
        IDistributableIntervention * pIV
    )
    {
        bool ret = true;

        // NOTE: Calling this AFTER the QI/GiveDrug crashes!!! Both win and linux. Says SetContextTo suddenly became a pure virtual.

        ICircumcision * pCirc = NULL;
        if( s_OK == pIV->QueryInterface(GET_IID(ICircumcision), (void**) &pCirc) )
        {
            LOG_DEBUG("Getting circumcised\n");
            ret = ret && ApplyCircumcision(pCirc);
        }

        return ret && InterventionsContainer::GiveIntervention( pIV );
    }

    void
    STIInterventionsContainer::UpdateSTIBarrierProbabilitiesByType(
        RelationshipType::Enum rel_type,
        SigmoidConfig config_overrides
    )
    {
        STI_blocking_overrides[ rel_type ] = config_overrides;
    }

    SigmoidConfig
    STIInterventionsContainer::GetSTIBarrierProbabilitiesByRelType(
        RelationshipType::Enum rel_type
    )
    const
    {
        SigmoidConfig ret;

        if( STI_blocking_overrides.find( rel_type ) != STI_blocking_overrides.end() )
        {
            LOG_DEBUG( "Using override condom config values from campaign.\n" );
            ret = STI_blocking_overrides.at( rel_type );
        }
        else
        {
            LOG_DEBUG( "Using static (default/config.json) condom config values.\n" );
            switch( rel_type )
            {
                case RelationshipType::MARITAL:
                    ret.midyear = IndividualHumanSTIConfig::condom_usage_probability_in_marital_relationships_midyear;
                    ret.rate = IndividualHumanSTIConfig::condom_usage_probability_in_marital_relationships_rate;
                    ret.early = IndividualHumanSTIConfig::condom_usage_probability_in_marital_relationships_early;
                    ret.late = IndividualHumanSTIConfig::condom_usage_probability_in_marital_relationships_late;
                break;
                case RelationshipType::INFORMAL:
                    ret.midyear = IndividualHumanSTIConfig::condom_usage_probability_in_informal_relationships_midyear;
                    ret.rate = IndividualHumanSTIConfig::condom_usage_probability_in_informal_relationships_rate;
                    ret.early = IndividualHumanSTIConfig::condom_usage_probability_in_informal_relationships_early;
                    ret.late = IndividualHumanSTIConfig::condom_usage_probability_in_informal_relationships_late;
                break;
                case RelationshipType::TRANSITORY:
                    ret.midyear = IndividualHumanSTIConfig::condom_usage_probability_in_transitory_relationships_midyear;
                    ret.rate = IndividualHumanSTIConfig::condom_usage_probability_in_transitory_relationships_rate;
                    ret.early = IndividualHumanSTIConfig::condom_usage_probability_in_transitory_relationships_early;
                    ret.late = IndividualHumanSTIConfig::condom_usage_probability_in_transitory_relationships_late;
                break;
            }
        }
        //LOG_DEBUG_F( "%s returning %f for type %s\n", __FUNCTION__, (float)ret, rel_type );
        return ret;
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

    bool STIInterventionsContainer::IsCircumcised( void ) const {
        return is_circumcised;
    }

    bool STIInterventionsContainer::ApplyCircumcision( ICircumcision *pCirc ) {
        // Need to get gender
        IIndividualHuman *ih = NULL;
        if( s_OK != parent->QueryInterface(GET_IID(IIndividualHuman), (void**) &ih) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHuman", "IIndividualHuman" );
        }

        if( ih->GetGender() == Gender::FEMALE )
        {
            return false;
        }

        if( IsCircumcised() )
        {
            return false;
        }

        is_circumcised = true;
        return true;
    }

    void STIInterventionsContainer::ChangeProperty( const char *prop, const char* new_value)
    {
        IIndividualHumanSTI *ihsti = NULL;
        if( s_OK != parent->QueryInterface(GET_IID(IIndividualHumanSTI), (void**) &ihsti) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanContext", "IIndividualHumanSTI" );
        }
        ihsti->UpdateSTINetworkParams(prop, new_value);

        InterventionsContainer::ChangeProperty( prop, new_value);
    }

    void
    STIInterventionsContainer::SpreadStiCoInfection()
    {
        IIndividualHumanSTI *ihsti = NULL;
        if( s_OK != parent->QueryInterface(GET_IID(IIndividualHumanSTI), (void**) &ihsti) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanContext", "IIndividualHumanSTI" );
        }
        ihsti->SetStiCoInfectionState();
    }

    void
    STIInterventionsContainer::CureStiCoInfection()
    {
        IIndividualHumanSTI *ihsti = NULL;
        if( s_OK != parent->QueryInterface(GET_IID(IIndividualHumanSTI), (void**) &ihsti) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanContext", "IIndividualHumanSTI" );
        }
        ihsti->ClearStiCoInfectionState();
    }
}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::STIInterventionsContainer)
namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, STIInterventionsContainer& container, const unsigned int v)
    {
        static const char * _module = "STIInterventionsContainer";
        LOG_DEBUG("(De)serializing STIInterventionsContainer\n");

        ar & container.is_circumcised;

        ar & boost::serialization::base_object<InterventionsContainer>(container);
    }
}
#endif
