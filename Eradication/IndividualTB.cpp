/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#ifdef ENABLE_TB

#include "IndividualTB.h"
#include "NodeTB.h"
#include "IndividualEventContext.h"
#include "InfectionTB.h"
#include "SusceptibilityTB.h"
#include "TBInterventionsContainer.h"
#include "TBContexts.h"

static const char* _module = "IndividualHumanTB";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(IndividualHumanTB, IndividualHumanAirborne)
        HANDLE_INTERFACE(IIndividualHumanTB)
        HANDLE_INTERFACE(IIndividualHumanTB2)
    END_QUERY_INTERFACE_DERIVED(IndividualHumanTB, IndividualHumanAirborne)

    void IndividualHumanTB::InitializeStaticsTB( const Configuration* config ) // just called once!
    {
        LOG_DEBUG( "Configure\n" );
        SusceptibilityTBConfig fakeImmunity;
        fakeImmunity.Configure( config );
        InfectionTBConfig fakeInfection;
        fakeInfection.Configure( config );

        //Superinfection not yet supported for TB sims
        if (IndividualHumanConfig::superinfection )
        {
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Enable_Superinfection", IndividualHumanConfig::superinfection ? "1" : "0", "Simulation_Type", "TB_SIM" );
        }
    }

    IndividualHumanTB *IndividualHumanTB::CreateHuman( INodeContext *context, suids::suid id, float MCweight, float init_age, int gender, float init_poverty)
    {
        IndividualHumanTB *newindividual = _new_ IndividualHumanTB(id, MCweight, init_age, gender, init_poverty);

        newindividual->SetContextTo(context);
        LOG_DEBUG_F( "Created human with age=%f\n", newindividual->m_age );

        return newindividual;
    }

    void IndividualHumanTB::InitializeHuman()
    {
        IndividualHuman::InitializeHuman();

        if(Environment::getInstance()->Log->CheckLogLevel(Logger::DEBUG, "EEL"))
        {
            tProperties* pProp = GetEventContext()->GetProperties();
            Environment::getInstance()->Log->LogF(Logger::DEBUG, "EEL","t=%d,hum_id=%d,new_hum_state=%d,Props=%s \n", int(parent->GetTime().time), GetSuid().data, 0,  (*pProp)[ "QualityOfCare" ].c_str() );
        }
    }

    void IndividualHumanTB::CreateSusceptibility(float imm_mod, float risk_mod)
    {
        susceptibility = SusceptibilityTB::CreateSusceptibility(this, m_age, imm_mod, risk_mod);
    }

    void IndividualHumanTB::UpdateInfectiousness(float dt)
    {
        // Updates soc_network here, based on individual's infections and the match between their social_connections and the community network
        // Big simplification for now.  Just binary infectivity depending on latent/active state of infection
        infectiousness = 0;
  
        if ( infections.size() == 0 ) 
            return;

        for (auto infection : infections)
        {
            infectiousness += infection->GetInfectiousness();
            float tmp_infectiousness =  m_mc_weight * infection->GetInfectiousness() * susceptibility->GetModTransmit() * interventions->GetInterventionReducedTransmit();
            StrainIdentity tmp_strainIDs;
            infection->GetInfectiousStrainID(&tmp_strainIDs);
            if ( tmp_infectiousness )
            {
                parent->DepositFromIndividual(&tmp_strainIDs, tmp_infectiousness, &transmissionGroupMembership);
            }
            if(infectiousness > 0) break; // TODO: reconsider only counting FIRST active infection in container
        }

        // Effects of transmission-reducing immunity/interventions.  Can set a maximum individual infectiousness here
        // TODO: if we want to actually truncate infectiousness at some maximum value, then QueueDepositContagion will have to be postponed as in IndividualVector
        if (infectiousness > 1)
        {
            infectiousness *= susceptibility->GetModTransmit() * interventions->GetInterventionReducedTransmit();
        }
        else
        {
            infectiousness *= susceptibility->GetModTransmit() * interventions->GetInterventionReducedTransmit();
        }

    }

    bool IndividualHumanTB::SetNewInfectionState(InfectionStateChange::_enum inf_state_change)
    {
        //trigger node level interventions = THIS IS DONE IN NODETB
        ((NodeTB*)parent)->OnNewInfectionState(inf_state_change, this);
        //trigger individual level interventions = THIS IS DONE HERE.
        //GHH duplicated code for TBActivation, TBActivationSmearPos, TBActivationSmearNeg, and TBActivationExtrapulm
        //this is intentional for future use of different triggers by smear status although it is not done yet
        if(Environment::getInstance()->Log->CheckLogLevel(Logger::DEBUG, "EEL"))
        {
            Environment::getInstance()->Log->LogF(Logger::DEBUG, "EEL","t=%d,hum_id=%d,new_inf_state=%lu,inf_id=%d \n", int(parent->GetTime().time), GetSuid().data, inf_state_change, -1 );
        }        
        if ( IndividualHuman::SetNewInfectionState(inf_state_change) )
        {
            // Nothing is currently set in the base function (death and disease clearance are handled directly in the Update function)
        }
        else if ( inf_state_change == InfectionStateChange::Cleared )
        {
            m_new_infection_state = NewInfectionState::NewlyCleared;                  //  Additional reporting of cleared infections
        }
        else if ( inf_state_change == InfectionStateChange::TBActivationPresymptomatic )   //  Latent infection that became active
        {
            //broadcaster->TriggerNodeEventObservers(GetEventContext(), IndividualEventTriggerType::TBActivationPresymptomatic);
        }        
        else if ( inf_state_change == InfectionStateChange::TBActivation )   //  Latent infection that became active
        {
            m_new_infection_state = NewInfectionState::NewlyActive;
        }
        else if ( inf_state_change == InfectionStateChange::TBActivationSmearPos )   //  Latent infection that became active
        {
            m_new_infection_state = NewInfectionState::NewlyActive;
        }
        else if ( inf_state_change == InfectionStateChange::TBActivationSmearNeg )   //  Latent infection that became active
        {
            m_new_infection_state = NewInfectionState::NewlyActive;
        }
        else if ( inf_state_change == InfectionStateChange::TBActivationExtrapulm )   //  Latent infection that became active
        {
            m_new_infection_state = NewInfectionState::NewlyActive;
        }
        else if ( inf_state_change == InfectionStateChange::ClearedPendingRelapse )   //  Latent infection that became active
        {
            m_new_infection_state = NewInfectionState::NewlyInactive; 
        }
        else if ( inf_state_change == InfectionStateChange::TBInactivation ) //  Active infection that became latent
        {
            m_new_infection_state = NewInfectionState::NewlyInactive;
        }
        else
        {
            return false;
        }

        return true;
    }
    
    void IndividualHumanTB::RegisterInfectionIncidenceObserver(
        IInfectionIncidenceObserver * pObserver 
    )
    {
        infectionIncidenceObservers.insert( pObserver );
    }

    void IndividualHumanTB::UnRegisterAllObservers(
        IInfectionIncidenceObserver * pObserver 
    )
    {
        infectionIncidenceObservers.erase( pObserver );
    }

    void IndividualHumanTB::onInfectionIncidence() // TBD: use map later so we can have many different types of observers separated into their own list
    {
        for (auto pObserver : infectionIncidenceObservers)
        {
            pObserver->notifyOnInfectionIncidence( this );
        }
    }

    void IndividualHumanTB::onInfectionMDRIncidence()
    {
        for (auto pObserver : infectionIncidenceObservers)
        {
            pObserver->notifyOnInfectionMDRIncidence( this );
        }
    }


    bool IndividualHumanTB::HasActiveInfection() const
    {
        for (auto infection : infections)
        {
            if(infection->IsActive())
            {
                return true;
            }
        }
        return false;
    }

    bool IndividualHumanTB::HasLatentInfection() const
    {
        LOG_DEBUG_F( "%s: infections.size() = %d.\n", __FUNCTION__, infections.size() );
        for (auto infection : infections)
        {
            if(!infection->IsActive())
            {
                return true;
            }
        }
        return false;
    }
    
    bool IndividualHumanTB::HasPendingRelapseInfection() const
    {
        LOG_DEBUG_F( "%s: infections.size() = %d.\n", __FUNCTION__, infections.size() );
        for (auto infection : infections)
        {
            InfectionTB * pointerITB = (InfectionTB *) infection;
            if(pointerITB->IsPendingRelapse())
            {
                return true;
            }
        }
        return false;
    }

    bool IndividualHumanTB::IsFastProgressor() const
    {
        for (auto infection : infections)
        {
            InfectionTB * pointerITB = (InfectionTB *) infection;
            if(pointerITB->IsFastProgressor())
            {
                return true;
            }
        }
        return false;
    }

    bool IndividualHumanTB::IsImmune() const
    {
        return susceptibility->IsImmune();
    }

    bool IndividualHumanTB::IsSmearPositive() const
    {
        for (auto infection : infections)
        {
            InfectionTB * pointerITB = (InfectionTB *) infection;
            if(pointerITB->IsSmearPositive())
            {
                return true;
            }
        }
        return false;
    }

    bool IndividualHumanTB::IsExtrapulmonary() const
    {
        for (auto infection : infections)
        {
            InfectionTB * pointerITB = (InfectionTB *) infection;
            if(pointerITB->IsExtrapulmonary())
            {
                return true;
            }
        }
        return false;
    }

    bool IndividualHumanTB::IsMDR() const
    {
        for (auto infection : infections)
        {
            InfectionTB * pointerITB = (InfectionTB *) infection;
            if(pointerITB->IsMDR())
            {
                return true;
            }
        }
        return false;
    }
    bool IndividualHumanTB::IsEvolvedMDR() const
    {
        for (auto infection : infections)
        {
            InfectionTB * pointerITB = (InfectionTB *) infection;
            if(pointerITB->EvolvedResistance() && pointerITB->IsMDR())
            {
                return true;
            }
        }
        return false;
    }

    bool IndividualHumanTB::IsTreatmentNaive() const
    {
         // Query for intervention container, in future cache TB Intervention container when we create it 
        IIndividualHumanInterventionsContext *context = GetInterventionsContext();
        ITBInterventionsContainer * itbivc = nullptr;

        if (s_OK != context->QueryInterface(GET_IID(ITBInterventionsContainer), (void**)&itbivc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "ITBInterventionsContainer", "IIndividualHumanInterventionsContext" );
        }

        return itbivc->GetTxNaiveStatus();
    }

    bool IndividualHumanTB::HasFailedTreatment() const
    {
        // Query for intervention container
        IIndividualHumanInterventionsContext *context = GetInterventionsContext();
        ITBInterventionsContainer * itbivc = nullptr;

        if (s_OK != context->QueryInterface(GET_IID(ITBInterventionsContainer), (void**)&itbivc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "ITBInterventionsContainer", "IIndividualHumanInterventionsContext" );
        }

        return itbivc->GetTxFailedStatus();
    }

    bool IndividualHumanTB::HasEverRelapsedAfterTreatment() const
    {
        // Query for intervention container
        IIndividualHumanInterventionsContext *context = GetInterventionsContext();
        ITBInterventionsContainer * itbivc = nullptr;

        if (s_OK != context->QueryInterface(GET_IID(ITBInterventionsContainer), (void**)&itbivc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "ITBInterventionsContainer", "IIndividualHumanInterventionsContext" );
        }

        return itbivc->GetTxEverRelapsedStatus();
    }

    bool IndividualHumanTB::HasActivePresymptomaticInfection() const
    {
        for (auto infection : infections)
        {
            InfectionTB * pointerITB = (InfectionTB *) infection;
            if(infection->IsActive() && !pointerITB->IsSymptomatic())
            {
                return true;
            }
        }
        return false;
    }
    bool IndividualHumanTB::IsOnTreatment() const 
    { 
        // Query for intervention container
        IIndividualHumanInterventionsContext *context = GetInterventionsContext();
        ITBInterventionsContainer * itbivc = nullptr;

        if (s_OK != context->QueryInterface(GET_IID(ITBInterventionsContainer), (void**)&itbivc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "ITBInterventionsContainer", "IIndividualHumanInterventionsContext" );
        }

        if ( itbivc->GetNumTBDrugsActive() > 0 ) 
        {
            return true;
        }
        else
        {
            return false;
        }
    }

    float IndividualHumanTB::GetDurationSinceInitInfection() const
    {
        float ret = -1.0f;
        for(auto infection: infections) //note this functin only works with one infection, it will return the answer for the first infection only
        {
            InfectionTB * pointerITB = (InfectionTB *) infection;
            ret = pointerITB->GetDurationSinceInitialInfection(); 
        }
        return ret;
    }


    int IndividualHumanTB::GetTime() const
    {
        return int(parent->GetTime().time);
    }

    IndividualHumanTB::IndividualHumanTB(suids::suid _suid, float monte_carlo_weight, float initial_age, int gender, float initial_poverty) :
        IndividualHumanAirborne(_suid, monte_carlo_weight, initial_age, gender, initial_poverty)
    {
    }

    IInfection* IndividualHumanTB::createInfection( suids::suid _suid )
    {
        InfectionTB* new_inf = InfectionTB::CreateInfection(this, _suid);
        return static_cast<IInfection*>(new_inf);
    }
    
    void IndividualHumanTB::setupInterventionsContainer()
    {
        interventions = _new_ TBInterventionsContainer();
    }

    REGISTER_SERIALIZABLE(IndividualHumanTB);

    void IndividualHumanTB::serialize(IArchive& ar, IndividualHumanTB* obj)
    {
        IndividualHumanAirborne::serialize(ar, obj);
        // IndividualHumanTB doesn't have any additional fields.
    }
}

#endif // ENABLE_TB
