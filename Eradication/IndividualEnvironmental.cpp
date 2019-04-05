/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#if defined(ENABLE_ENVIRONMENTAL)

#include "NodeEnvironmental.h"
#include "IndividualEnvironmental.h"
#include "InfectionEnvironmental.h"
#include "SusceptibilityEnvironmental.h"
#include "StrainIdentity.h"

#pragma warning(disable: 4244)

SETUP_LOGGING( "IndividualEnvironmental" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(IndividualHumanEnvironmental, IndividualHuman)
    END_QUERY_INTERFACE_DERIVED(IndividualHumanEnvironmental, IndividualHuman)

    IndividualHumanEnvironmental::IndividualHumanEnvironmental( suids::suid _suid, float monte_carlo_weight, float initial_age, int gender) 
    : IndividualHuman( _suid, monte_carlo_weight, initial_age, gender)
    , exposureRoute( TransmissionRoute::Enum( 0 ) )
    , transmissionGroupMembershipByRoute()
    {
    }

    IndividualHumanEnvironmental *
    IndividualHumanEnvironmental::CreateHuman(INodeContext *context, suids::suid id, float MCweight, float init_age, int gender)
    {
        IndividualHumanEnvironmental *newindividual = _new_ IndividualHumanEnvironmental( id, MCweight, init_age, gender);

        newindividual->SetContextTo(context);
        LOG_DEBUG_F( "Created human with age=%f\n", newindividual->m_age );

        return newindividual;
    }

    IndividualHumanEnvironmental::~IndividualHumanEnvironmental()
    {
    }
    
    void IndividualHumanEnvironmental::CreateSusceptibility(float imm_mod, float risk_mod)
    {
        susceptibility = SusceptibilityEnvironmental::CreateSusceptibility(this, m_age, imm_mod, risk_mod);
    }

    void IndividualHumanEnvironmental::ReportInfectionState()
    {
        m_new_infection_state = NewInfectionState::NewInfection; 
    }

    void IndividualHumanEnvironmental::UpdateInfectiousness(float dt)
    {
        infectiousness = 0;
        for (auto infection : infections)
        {
            LOG_DEBUG("Getting infectiousness by route.\n");
            float tmp_infectiousnessFecal =  m_mc_weight * infection->GetInfectiousnessByRoute(string("environmental"));
            float tmp_infectiousnessOral = m_mc_weight * infection->GetInfectiousnessByRoute(string("contact"));

            StrainIdentity tmp_strainID;
            infection->GetInfectiousStrainID(&tmp_strainID);
            LOG_DEBUG_F("UpdateInfectiousness: InfectiousnessFecal = %f, InfectiousnessOral = %f.\n", tmp_infectiousnessFecal, tmp_infectiousnessOral);

            //deposit oral to 'contact', fecal to 'environmental' pool
            LOG_DEBUG("Getting routes.\n");

            for(auto& entry : transmissionGroupMembershipByRoute)
            {
                LOG_DEBUG_F("Found route:%s.\n",entry.first.c_str());
                if (entry.first==string("contact"))
                {
                    if (tmp_infectiousnessOral > 0.0f)
                    {
                        LOG_DEBUG_F("Depositing %f to route %s: (antigen=%d, substain=%d)\n", tmp_infectiousnessOral, entry.first.c_str(), tmp_strainID.GetAntigenID(), tmp_strainID.GetGeneticID());
                        parent->DepositFromIndividual( tmp_strainID, tmp_infectiousnessOral, entry.second, TransmissionRoute::TRANSMISSIONROUTE_CONTACT );
                        infectiousness += infection->GetInfectiousnessByRoute(string("contact"));
                    }
                }
                else if (entry.first==string("environmental"))
                {
                    if (tmp_infectiousnessFecal > 0.0f)
                    {
                        LOG_DEBUG_F("Depositing %f to route %s: (antigen=%d, substain=%d)\n", tmp_infectiousnessFecal, entry.first.c_str(), tmp_strainID.GetAntigenID(), tmp_strainID.GetGeneticID());    
                        parent->DepositFromIndividual( tmp_strainID, tmp_infectiousnessFecal, entry.second, TransmissionRoute::TRANSMISSIONROUTE_ENVIRONMENTAL );
                        infectiousness += infection->GetInfectiousnessByRoute(string("environmental"));
                    }
                }
                else
                {
                    LOG_WARN_F("unknown route %s, do not deposit anything.\n", entry.first.c_str());
                }
           }

        }
    }

    void IndividualHumanEnvironmental::UpdateGroupPopulation(float size_changes)
    {
        parent->UpdateTransmissionGroupPopulation(GetProperties()->GetOldVersion(), size_changes, this->GetMonteCarloWeight());
    }

    void IndividualHumanEnvironmental::UpdateGroupMembership()
    {
        tProperties properties = GetProperties()->GetOldVersion();
        const RouteList_t& routes = parent->GetTransmissionRoutes();
        LOG_DEBUG_F( "Updating transmission group membership for individual %d for %d routes (first route is %s).\n", this->GetSuid().data, routes.size(), routes[ 0 ].c_str() );

        for( auto& route : routes )
        {
            LOG_DEBUG_F( "Updating for Route %s.\n", route.c_str() );
            parent->GetGroupMembershipForIndividual( RouteList_t{ route }, properties, transmissionGroupMembershipByRoute[ route ] );
        }
        IndividualHuman::UpdateGroupMembership();
    }

    IInfection* IndividualHumanEnvironmental::createInfection( suids::suid _suid )
    {
        return InfectionEnvironmental::CreateInfection(this, _suid);
    }

    void IndividualHumanEnvironmental::Expose( const IContagionPopulation* cp, float dt, TransmissionRoute::Enum transmissionRoute )
    {
        exposureRoute = transmissionRoute;
        IndividualHuman::Expose( cp, dt, transmissionRoute );
    }

    void IndividualHumanEnvironmental::AcquireNewInfection( const IStrainIdentity *infstrain, int incubation_period_override )
    {
        if ( infstrain )
        {
            StrainIdentity infectingStrain;
            infstrain->ResolveInfectingStrain( &infectingStrain );
            if ( incubation_period_override == 0 )
            {
                infectingStrain.SetGeneticID( 2 );
                exposureRoute = TransmissionRoute::TRANSMISSIONROUTE_OUTDOOR;
            }
            else if (exposureRoute == TransmissionRoute::TRANSMISSIONROUTE_ENVIRONMENTAL)
            {
                infectingStrain.SetGeneticID( 0 );
            }
            else if (exposureRoute == TransmissionRoute::TRANSMISSIONROUTE_CONTACT)
            {
                infectingStrain.SetGeneticID( 1 );
            }
            else
            {
                // #pragma message( "TODO - " __FUNCTION__ )
            }

            IndividualHuman::AcquireNewInfection( &infectingStrain, incubation_period_override );
        }
        else
        {
            IndividualHuman::AcquireNewInfection( infstrain, incubation_period_override );
        }
    }

    REGISTER_SERIALIZABLE(IndividualHumanEnvironmental);

    void IndividualHumanEnvironmental::serialize(IArchive& ar, IndividualHumanEnvironmental* obj)
    {
        IndividualHuman::serialize(ar, obj);
        /* IndividualHumanEnvironmental doesn't (yet) have any member fields.
        IndividualHumanEnvironmental& individual = *dynamic_cast<IndividualHumanEnvironmental*>(obj);
        ar.startObject();
        ar.endObject();
        */
    }
}

#endif // ENABLE_POLIO
