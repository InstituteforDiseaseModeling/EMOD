/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include <list>
#include <map>
#include <fstream>
#include "Exceptions.h"
#include "Log.h"
#include "EventTrigger.h"
#include "Configuration.h"
#include "ConfigurationImpl.h"

#include "IndividualCoInfection.h"
#include "IContagionPopulation.h"
#include "IndividualEventContext.h"
#include "InfectionTB.h"
#include "SusceptibilityTB.h"
#include "TBInterventionsContainer.h"

#include "InfectionHIV.h"
#include "SusceptibilityHIV.h"
#include "HIVInterventionsContainer.h"
#include "TBHIVParameters.h"

#include "MasterInterventionsContainer.h"
#include "IIndividualHumanHIV.h"

#include "NodeEventContext.h"
#include "Infection.h"
#include "NodeTBHIV.h"
#include "SimulationConfig.h"

#include "NodeDemographics.h"
#include "StrainIdentity.h"


SETUP_LOGGING( "IndividualCoInfection" )


namespace Kernel
{
    map <float,float> IndividualHumanCoInfectionConfig::CD4_act_map;
    float IndividualHumanCoInfectionConfig::ART_extra_reactivation_reduction = 1.0f;

    map <float, float> IndividualHumanCoInfectionConfig::TB_CD4_Infectiousness_Map;
    map <float,float> IndividualHumanCoInfectionConfig::TB_CD4_Susceptibility_Map;
    map<float,float> IndividualHumanCoInfectionConfig::TB_CD4_Primary_Progression_Map;
    vector <float> IndividualHumanCoInfectionConfig::TB_CD4_infectiousness;
    vector <float> IndividualHumanCoInfectionConfig::TB_CD4_susceptibility;
    vector <float> IndividualHumanCoInfectionConfig::TB_CD4_strata_for_infectiousness_susceptibility;
    vector <float> IndividualHumanCoInfectionConfig::TB_CD4_Primary;
    float IndividualHumanCoInfectionConfig::coinfection_mortality_on_ART = 0.0f;
    float IndividualHumanCoInfectionConfig::coinfection_mortality_off_ART = 0.0f;
    bool IndividualHumanCoInfectionConfig::enable_coinfection = false;
    bool IndividualHumanCoInfectionConfig::enable_coinfection_mortality = false;
    bool IndividualHumanCoInfectionConfig::enable_exogenous = false;


    //CD4_act_map.insert(std::pair<float,float>(0.0f,0.0f)); 
    GET_SCHEMA_STATIC_WRAPPER_IMPL(TBHIV.Individual,IndividualHumanCoInfectionConfig)
    BEGIN_QUERY_INTERFACE_BODY(IndividualHumanCoInfectionConfig)
    END_QUERY_INTERFACE_BODY(IndividualHumanCoInfectionConfig)

    BEGIN_QUERY_INTERFACE_DERIVED(IndividualHumanCoInfection, IndividualHumanAirborne)
        HANDLE_INTERFACE(IIndividualHumanCoInfection)
        HANDLE_INTERFACE(IIndividualHumanTB)
        HANDLE_INTERFACE(IIndividualHumanHIV) 
    END_QUERY_INTERFACE_DERIVED(IndividualHumanCoInfection, IndividualHumanAirborne)


   IndividualHumanCoInfection::IndividualHumanCoInfection() :
        IndividualHumanAirborne(),
        infectioncount_tb( 0 ),
        infectioncount_hiv( 0 ),
        m_bool_exogenous( false ),
        susceptibility_tb(),
        susceptibility_hiv(),
        susceptibilitylist(),
        infection2susceptibilitymap()
    {}

    IndividualHumanCoInfection::IndividualHumanCoInfection( suids::suid _suid, float monte_carlo_weight, float initial_age, int gender) :
        IndividualHumanAirborne(_suid, monte_carlo_weight, initial_age, gender),
        infectioncount_tb(0), 
        infectioncount_hiv(0),
        m_bool_exogenous(false),
        infectionMDRIncidenceCounter(0),
        mdr_evolved_incident_counter(0),
        new_mdr_fast_active_infection_counter(0),
        infection2susceptibilitymap()
    {

    } 

    IndividualHumanCoInfection::~IndividualHumanCoInfection( void ) 
    { 
        susceptibilitylist.clear(); //susceptibility_hiv and susceptibility_tb are contained in this list
        
        // newInfectionlist and Individual::infections contain pointers to the same infections (see AcquireNewInfection)
        // so calling delete on the pointers in newInfectionlist can cause an exception
        // because the infections were already deleted (using the pointers in Individual::infections)
        // same is true for infection2susceptibilitymap
        infection2susceptibilitymap.clear();

        //delete susceptibility_tb; //will be deleted in individual destructor
    }

    bool IndividualHumanCoInfectionConfig::Configure( const Configuration* config ) // just called once!
    {
        initConfigTypeMap( "Enable_Coinfection",                          &enable_coinfection,            Enable_Coinfection_Incidence_DESC_TEXT, false );

        initConfigTypeMap( "CoInfection_Mortality_Rate_On_ART",           &coinfection_mortality_on_ART,  CoInfection_Mortality_Rate_On_ART_DESC_TEXT, 0.0f, FLT_MAX, 0.0f, "Enable_Coinfection");
        initConfigTypeMap( "CoInfection_Mortality_Rate_Off_ART",          &coinfection_mortality_off_ART, CoInfection_Mortality_Rate_Off_ART_DESC_TEXT, 0.0f, FLT_MAX, 0.0f, "Enable_Coinfection");
        initConfigTypeMap( "TB_CD4_Infectiousness",                       &TB_CD4_infectiousness,         TB_CD4_Infectiousness_DESC_TEXT, 0.0f, 1.0f, 1.0f, false, "Enable_Coinfection");  //capped at 1.0 by biology should be less infectious
        initConfigTypeMap( "TB_CD4_Susceptibility",                       &TB_CD4_susceptibility,         TB_CD4_Susceptibility_DESC_TEXT, 1.0f, FLT_MAX, 1.0, false, "Enable_Coinfection");  //should always be as susceptible or more biological bound
        initConfigTypeMap( "TB_CD4_Primary_Progression",                  &TB_CD4_Primary,                TB_CD4_Primary_Progression_DESC_TEXT, 1.0f, FLT_MAX, 1.0f, false, "Enable_Coinfection");
        initConfigTypeMap( "TB_CD4_Strata_Infectiousness_Susceptibility", &TB_CD4_strata_for_infectiousness_susceptibility, TB_CD4_Strata_Infectiousness_Susceptibility_DESC_TEXT, 0.0f, FLT_MAX, 1.0f, false, "Enable_Coinfection");
        initConfigTypeMap( "ART_Reactivation_Factor",                     &ART_extra_reactivation_reduction, ART_Reactivation_Factor_DESC_TEXT , 0.0, 1.0, 1.0, "Enable_Coinfection");
        initConfigTypeMap( "TB_Enable_Exogenous",                         &enable_exogenous,              TB_Enable_Exogenous_DESC_TEXT , false);
        initConfigTypeMap( "Enable_Coinfection_Mortality",                &enable_coinfection_mortality,  Enable_Coinfection_Mortality_DESC_TEXT, false, "Enable_Vital_Dynamics");
        LOG_DEBUG( "Configure\n" );
     
        bool bret = JsonConfigurable::Configure(config);

        ConstructTBInfSusCD4Maps();   // static maps for susceptibility and infectiousness based on CD4
       
        return bret;
    }

    void IndividualHumanCoInfection::InitializeStaticsCoInfection( const Configuration* config )
    {
        SusceptibilityTBConfig tb_immunity_config;
        tb_immunity_config.Configure( config );
        InfectionTBConfig tb_infection_config;
        tb_infection_config.Configure( config );
        IndividualHumanCoInfectionConfig individual_config;
        individual_config.Configure( config );

        if( IndividualHumanCoInfectionConfig::enable_coinfection )
        {
            // Now create static, constant map between CD4 and factor for increased reactivation rate. This is only done once.
            IndividualHumanCoInfectionConfig::CD4_act_map = tb_infection_config.GetCD4Map();
            SusceptibilityHIVConfig hiv_immunity_config;
            hiv_immunity_config.Configure( config );
            InfectionHIVConfig hiv_infection_config;
            hiv_infection_config.Configure( config );
        }
    }

    IndividualHumanCoInfection *IndividualHumanCoInfection::CreateHuman(INodeContext *context, suids::suid _suid, float MCweight, float init_age, int gender)
    {
        IndividualHumanCoInfection *newindividual = _new_ IndividualHumanCoInfection(_suid, MCweight, init_age, gender);
        newindividual->SetContextTo(context);
        newindividual->m_has_ever_been_onART= 0;
        newindividual->m_has_ever_tested_positive_for_HIV = 0;
        LOG_DEBUG_F("Created human with age=%f\n", newindividual->m_age);

        return newindividual;
    }

    void IndividualHumanCoInfection::CreateSusceptibility(float imm_mod, float risk_mod)
    {
        susceptibility_tb = SusceptibilityTB::CreateSusceptibility( this, m_age, imm_mod, risk_mod );
        if( IndividualHumanCoInfectionConfig::enable_coinfection )
        {
            susceptibility_hiv = SusceptibilityHIV::CreateSusceptibility( this, m_age, imm_mod, risk_mod );
            susceptibilitylist.push_back( susceptibility_hiv );
        }
        susceptibilitylist.push_back( susceptibility_tb );
    }

    const std::list< Susceptibility* > &
    IndividualHumanCoInfection::GetSusceptibilityList()
    {
        return susceptibilitylist;
    }

    void IndividualHumanCoInfection::AcquireNewInfection( const IStrainIdentity *infstrain, int incubation_period_override )
    {
        LOG_DEBUG( "AcquireNewInfection\n" );
        StrainIdentity newStrainId;
        if (infstrain != nullptr)
        {
            infstrain->ResolveInfectingStrain(&newStrainId); // get the substrain ID
        }

        int numInfs = infectioncount_tb;
        if ((IndividualHumanConfig::superinfection && (numInfs < IndividualHumanConfig::max_ind_inf)) || numInfs == 0)
        {
            LOG_DEBUG_F( "Individual %d acquired new TB infection.\n", GetSuid().data );
            cumulativeInfs++;
            m_is_infected = true;
            infection_list_t newInfectionlist;
            createInfection( parent->GetNextInfectionSuid(), newInfectionlist ); //newInfectionlist gets populated in createInfection()

            for (auto newinf : newInfectionlist)
            {
                IInfectionTB* pinfection = NULL;
                if (s_OK == newinf->QueryInterface(GET_IID( IInfectionTB ), (void**)&pinfection) )
                { 
                    newinf->SetParameters(&newStrainId, incubation_period_override);

                    if (HasHIV() )
                    {
                        ModActivate();
                    }

                    newinf->InitInfectionImmunology(susceptibility_tb);
                    LOG_DEBUG( "Adding infection to infections list.\n" );
                    infections.push_front(newinf);
                    infection2susceptibilitymap[newinf] = susceptibility_tb;
                    infectioncount_tb++;
                    infectiousness += newinf->GetInfectiousness();
                    ReportInfectionState(); // can specify different reporting in derived classes
                }
            }

            LOG_VALID_F( "Individual %lu acquired %d new infections \n", suid.data, newInfectionlist.size() );
        }
    }

    void IndividualHumanCoInfection::AcquireNewInfectionHIV( const IStrainIdentity *infstrain, int incubation_period_override ) 
    {
        LOG_DEBUG( "AcquireNewInfectionHIV\n" );
        //code is nearly duplicate to AcquireNewInfection but only gives new infection to HIV(the non-transmitting infection)
        //below I've commented out stuff that is actually specific to the TB infection

        //cumulativeInfs++; for now cumulativeInfs only counts TB infections
        //m_is_infected = true;

        //Now acquire the HIV infection, this is used for the OutbreakHIV
        //note, use the original createInfection (which does not take the newInfectionlist) for the HIV 
        //for each created infection, set the parameters and the initinfectionimmunology

        //Don't give HIV to someone TWICE

        if (HasHIV() )
        {
            return;
        }

        Infection* newinf = createInfection( parent->GetNextInfectionSuid()  );

        IInfectionHIV* pinfectionHIV = NULL;
        //this should always be true, just use for debugging
        if (s_OK == newinf->QueryInterface(GET_IID( IInfectionHIV ), (void**)&pinfectionHIV) )
        {
            newinf->SetParameters(const_cast<IStrainIdentity*>(infstrain), incubation_period_override);
            newinf->InitInfectionImmunology(susceptibility_hiv);

            LOG_DEBUG( "Adding infection to infections list.\n" );
            infections.push_front(newinf);
            infection2susceptibilitymap[newinf] = susceptibility_hiv;
            infectioncount_hiv++;
            LifeCourseLatencyUpdateAll();
            
            IIndividualHumanHIV* hiv_person = NULL;

            if (s_OK != this->QueryInterface(GET_IID(IIndividualHumanHIV), (void **) &hiv_person) )
            {
                  throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "hiv_person", "IndividualHumanCoInfection", "IIndividualHumanHIV");
            }

            hiv_person->GetHIVInterventionsContainer()->BroadcastNewHIVInfection();
        }    

        //LOG_VALID_F( " Individual %lu acquired %d new infections \n", suid.data, newInfectionlist.size() );   // newInfectionlist got shadowed in AcquireNewInfection()
    }

    void IndividualHumanCoInfection::SetForwardTBAct( std::vector<float> &vin)
    {
        for (auto susceptibility : susceptibilitylist)
        {
            ISusceptibilityTB* pTBsus = nullptr;
            if (s_OK == susceptibility->QueryInterface(GET_IID( ISusceptibilityTB ), (void**)&pTBsus) && HasHIV())
            {
                SetTBActivationVector(vin);
                pTBsus->SetCD4ActFlag(true);
                break;
            }
        }
    }


    void IndividualHumanCoInfection::Update(float currenttime, float dt)
    {
        LOG_VALID_F("%s\n", __FUNCTION__);
        float infection_timestep = dt;
        int numsteps = 1;

        // eventually need to correct for timestep in case of individuals moving among communities with different adapted time steps

        StateChange = HumanStateChange::None;

        //  Aging
        if (IndividualHumanConfig::aging) { UpdateAge(dt); }

        // Adjust time step for infections as specified by infection_updates_per_tstep.  A value of 0 reverts to a single update per timestep for backward compatibility.
        // There is no special meaning of 1 being hourly.  For hourly infection updates with a tstep of one day, one must now specify 24.
        if (IndividualHumanConfig::infection_updates_per_tstep > 1 )
        {
            // infection_updates_per_tstep is now an integer > 1, so set numsteps equal to it,
            // allowing the subdivision dt into smaller infection_timestep
            numsteps = IndividualHumanConfig::infection_updates_per_tstep;
            infection_timestep = dt / numsteps;
        }

        // Process list of infections
        if (infections.size() == 0) // don't need to process infections or go hour by hour
        {
            for (auto *susceptibility : susceptibilitylist)
            {
                susceptibility->Update(dt); 
            }

            interventions->InfectiousLoopUpdate( dt );
            interventions->Update(dt);
        }
        else
        {
            for (int i = 0; i < numsteps; i++)
            {
                for (infection_list_t::iterator it = infections.begin(); it != infections.end();)
                {
                    auto infection = (*it);

                    IInfectionTB* pinfTB = NULL;
                    /*if (s_OK != infection->QueryInterface(GET_IID( IInfectionTB ), (void**)&pinfTB) )
                    {    
                        LOG_DEBUG("This infection is not TB\n");
                    }
                    if (s_OK == infection->QueryInterface(GET_IID( IInfectionTB ), (void**)&pinfTB) )
                    {    
                        LOG_DEBUG("This infection is  TB\n");
                    }*/

                    // Update infection (both HIV and TB)
                    infection->Update(infection_timestep, infection2susceptibilitymap[infection] );

                    InfectionStateChange::_enum inf_state_change = infection->GetStateChange(); 
                    if (inf_state_change != InfectionStateChange::None && s_OK != infection->QueryInterface(GET_IID( IInfectionTB ), (void**)&pinfTB) ) 
                    {
                        LOG_DEBUG("InfectionStateChange due to HIV infection \n");
                    }
                    else if (inf_state_change != InfectionStateChange::None && s_OK == infection->QueryInterface(GET_IID( IInfectionTB ), (void**)&pinfTB) ) 
                    {
                        LOG_DEBUG("InfectionStateChange due to TB infection \n");
                    }
                    if (inf_state_change != InfectionStateChange::None) 
                    {
                        //Need to adjust this code when HIV is also using the InfectionStateChange
                        IInfectionTB* pinfTB = NULL;
                        if (s_OK == infection->QueryInterface(GET_IID( IInfectionTB ), (void**)&pinfTB) )
                        {    
                            SetNewInfectionState(inf_state_change);  //this is ONLY used for REPORTING! so only report for TB
                        }

                        // Notify susceptibility of cleared infection and remove from list
                        if ( inf_state_change == InfectionStateChange::Cleared ) 
                        { 
                            //if (immunity) { infection2susceptibilitymap[infection]->UpdateInfectionCleared(); } //Immunity update: survived infection, note the notification of Susceptibility HIV contains only a placeholder now
                            if (IndividualHumanConfig::enable_immunity) { static_cast<Susceptibility*>(infection2susceptibilitymap[infection])->UpdateInfectionCleared(); } //Immunity update: survived infection, note the notification of Susceptibility HIV contains only a placeholder now

                            IInfectionTB* pinfTB = NULL;
                            if (s_OK == infection->QueryInterface(GET_IID( IInfectionTB ), (void**)&pinfTB) )
                            {    
                                infectioncount_tb--;
                                LOG_DEBUG("This deleted infection is TB\n");
                            }
                            IInfectionHIV* pinfHIV = NULL;
                            if (s_OK == infection->QueryInterface(GET_IID( IInfectionHIV ), (void**)&pinfHIV) )
                            {    
                                infectioncount_hiv--;
                                LOG_DEBUG("This deleted infection is HIV\n");
                            }

                            delete *it;
                            it = infections.erase(it); 

                            continue; 
                        }
                    }
                    // Set human state change and stop updating infections if the person has died
                    if ( inf_state_change == InfectionStateChange::Fatal ) 
                    {
                        IInfectionTB* pinfTB2 = NULL;
                        IInfectionHIV* pinfHIV2 = NULL;
                        if (s_OK == infection->QueryInterface(GET_IID(IInfectionTB), (void**)&pinfTB2) )
                        {
                            Die(HumanStateChange::KilledByInfection);  // was StateChange = HumanStateChange::KilledByInfection;
                            break;
                        }
                        else if (s_OK == infection->QueryInterface(GET_IID(IInfectionHIV), (void**) &pinfHIV2) )
                        {
                            Die(HumanStateChange::KilledByOpportunisticInfection); 
                            break;
                        }
                        else
                        {

                            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "pinfTB2 or pinfHIV2", "Infection", "IInfectionHIV or IInfectionTB");
                        }
                    }
                    //}
                    it++;
                }

                if (IndividualHumanConfig::enable_immunity) { 
                    for (auto susceptibility : susceptibilitylist)
                    {
                        susceptibility->Update(infection_timestep); 
                    } 
                }      // Immunity update: mainly decay of immunity
                if (StateChange == HumanStateChange::KilledByInfection || StateChange == HumanStateChange::KilledByOpportunisticInfection) { break; } // If individual died, no need to keep simulating infections.
                interventions->InfectiousLoopUpdate( infection_timestep );
            }
            if( StateChange != HumanStateChange::KilledByInfection || StateChange == HumanStateChange::KilledByOpportunisticInfection )
            {
                interventions->Update( dt );
            }
        }

        applyNewInterventionEffects(dt);

        // Trigger "every-update" event observers 
        if (broadcaster)
        {
            broadcaster->TriggerNodeEventObservers(GetEventContext(), EventTrigger::EveryUpdate);
        }

        //  Get new infections
        ExposeToInfectivity(dt, &transmissionGroupMembership); // Need to do it even if infectivity==0, because of diseases in which immunity of acquisition depends on challenge (eg malaria)
        
        // Exogenous re-infection of Latently infected here dt = 0 is flag for this
        if (IndividualHumanCoInfectionConfig::enable_exogenous)
        {
            ExposeToInfectivity(0, &transmissionGroupMembership);
        }

        //  Is there an active infection for statistical purposes?
        //m_is_infected = (infections.size() > 0);
        m_is_infected = (infectioncount_tb > 0); //only count as infected if they have a tb infection (used for reportTB)


        if (StateChange == HumanStateChange::None)
        {
            CheckVitalDynamics(currenttime, dt);
            CheckHIVVitalDynamics(dt);
        }

        if (StateChange == HumanStateChange::None && GET_CONFIGURABLE(SimulationConfig)->migration_structure) // Individual can't migrate if they're already dead
        {
            CheckForMigration(currenttime, dt);
        }
    }

    void IndividualHumanCoInfection::CheckHIVVitalDynamics(float dt)  
    {
        if( IndividualHumanCoInfectionConfig::enable_coinfection_mortality )
        {
            // "HIVMortalityDistribution" is added to map in Node::SetParameters if 'enable_CoInfection_mortality' flag is set and you add the demographic file for HIV mortality
            //NOTE: HIV Mortality rates are amongst HIV pos people only!
            if ( HasHIV() && HasActiveInfection() && !HasActivePresymptomaticInfection() && GetTBInterventionsContainer()->GetNumTBDrugsActive() == 0 )
            {
                float t_m_HIV_mortality_rate;   
                
                if (GetHIVInterventionsContainer()->OnArtQuery() )
                {
                    t_m_HIV_mortality_rate = IndividualHumanCoInfectionConfig::coinfection_mortality_on_ART;
                }
                else
                {
                    t_m_HIV_mortality_rate = IndividualHumanCoInfectionConfig::coinfection_mortality_off_ART;
                }
                if(randgen->e() < (t_m_HIV_mortality_rate* dt) )
                {
                    LOG_DEBUG_F("Individual %lu died of CoInfection at age %f with m_HIV_mortality_rate = %f.\n", GetSuid().data, GetAge() / DAYSPERYEAR, t_m_HIV_mortality_rate );
                    LOG_VALID_F("Individual %lu moved from Active to Death . \n", GetSuid().data);
                    Die(HumanStateChange::KilledByInfection) ;
                }
            }
        }
    }

 

    bool IndividualHumanCoInfection::GetExogenousTBStateChange() const
    {
        return m_bool_exogenous;
    }

    void IndividualHumanCoInfection::Expose(const IContagionPopulation* cp, float dt, TransmissionRoute::Enum transmission_route)
    {
        StrainIdentity strainIDs;

        //GHH had to add contagionpopulation.h to header for this section to work 
        strainIDs.SetAntigenID(cp->GetAntigenID()); // find antigenID of the strain to get infectivity from, is the individual already infected by the contagion of this antigen type?  
        ISusceptibilityTB* pISTB = NULL;
        if (s_OK != susceptibility_tb->QueryInterface(GET_IID(ISusceptibilityTB), (void**) &pISTB) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "susceptibility_tb", "Susceptibility", "ISusceptibilityTB");
        }

   
        //BGW use dt=0 for exogenous reinfection code will be called for the latent class (note we still need to fix this up for proper genetics)
        float suscept_mod = pISTB->GetModAcquire(this);
        LOG_VALID_F( "%s: Individual %d with CD4count=%f has susceptibility CD4mod=%f.\n", __FUNCTION__, GetSuid().data, GetCD4(), suscept_mod );

        if (dt == 0)
        {
            // figure out if primary progression will happen
            if ( (randgen->e() < (pISTB->GetFastProgressorFraction()) ) && (HasLatentInfection()) && !(GetTBInfection()->IsFastProgressor() || GetTBInfection()->IsPendingRelapse() ) )
            {
                float dt_true =   GET_CONFIGURABLE(SimulationConfig)->Sim_Tstep; //Needed to get the timestep due to dt = 0 flag
                if (randgen->e() < EXPCDF(-cp->GetTotalContagion()*dt_true*suscept_mod*interventions->GetInterventionReducedAcquire())) // infection results from this strain?
                {
                    GetTBInfection()->ExogenousLatentSlowToFast();
                    cp->ResolveInfectingStrain(&strainIDs);
                    SetExogenousInfectionStrain(&strainIDs);

                }
            }
        }
        else
        {
            if (!InfectionExistsForThisStrain(&strainIDs)) // no existing infection of this antigenic type, so determine infection from exposure
            {
                //GHH temp changed to susceptibility_tb since this is for pools, which is specific for infectiousness, 
                //deal with HIV later (it has no strain tracking now anyways)

                if (randgen->e() < EXPCDF(-cp->GetTotalContagion()*dt*suscept_mod*interventions->GetInterventionReducedAcquire())) // infection results from this strain?
                {
                    cp->ResolveInfectingStrain(&strainIDs); // get the substrain ID
                    AcquireNewInfection(&strainIDs);
                }
            }
            else
            {
                // multiple infections of the same type happen here
                if (randgen->e() < EXPCDF(-cp->GetTotalContagion()*dt * suscept_mod*interventions->GetInterventionReducedAcquire())) // infection results from this strain?
                {
                    cp->ResolveInfectingStrain(&strainIDs);
                    AcquireNewInfection(&strainIDs); // superinfection of this antigenic type
                }
            }
        }
    }

    void IndividualHumanCoInfection::UpdateInfectiousness(float dt)
    {
        // Big simplification for now.  Just binary infectivity depending on latent/active state of infection
        infectiousness = 0;

        for (auto infection : infections)
        {
            IInfectionTB* pinfTB = NULL;
            if (s_OK != infection->QueryInterface(GET_IID( IInfectionTB ), (void**)&pinfTB) )
            {
                //LOG_DEBUG("This infection is not TB, do not add to total infectiousness in IndividualCoInfection (assume HIV is not infectious now) \n");
                continue;
            }
            infectiousness += infection->GetInfectiousness();
            float tmp_infectiousness =  m_mc_weight * infection->GetInfectiousness() * dynamic_cast <SusceptibilityTB*> (infection2susceptibilitymap[infection]) ->getModTransmit(this) * interventions->GetInterventionReducedTransmit();
            string state = "Latent";
            if (GetTBInfection()->IsActive())
            {
                if (GetTBInfection()->IsExtrapulmonary())
                    state = "Extrapulmonary";
                else if (HasActivePresymptomaticInfection())
                    state = "Presymptomatic";
                else if (GetTBInfection()->IsSmearPositive())
                    state = "SmearPositive";
                else
                    state = "SmearNegative";
            }
            LOG_VALID_F("%s Individual %d, has total_infectiousness= %f, weight= %f, orig_infectiousness= %f, CD4mod= %f, intervention reduced transmit= %f, CD4count= %f, state= %s , fast progressor=%d  \n",
                    __FUNCTION__, GetSuid().data, tmp_infectiousness, m_mc_weight, infection->GetInfectiousness(), dynamic_cast <SusceptibilityTB*> (infection2susceptibilitymap[infection])->getModTransmit(this),
                    interventions->GetInterventionReducedTransmit(), GetCD4(), state.c_str(), GetTBInfection()->IsFastProgressor());

            StrainIdentity tmp_strainIDs;
            infection->GetInfectiousStrainID(&tmp_strainIDs);
            if( tmp_infectiousness )
            {
                parent->DepositFromIndividual( tmp_strainIDs, tmp_infectiousness, &transmissionGroupMembership);
            }

            // TODO: in IndividualTB we only count FIRST active infection in container, here we comment that out? reconsider only counting FIRST active infection in container
        }

        // Effects of transmission-reducing immunity/interventions.  Can set a maximum individual infectiousness here
        // TODO: if we want to actually truncate infectiousness at some maximum value, then QueueDepositContagion will have to be postponed as in IndividualVector
        if (infectiousness > 0)
        {
            infectiousness *= dynamic_cast <SusceptibilityTB*>(susceptibility_tb) ->getModTransmit(this) * interventions->GetInterventionReducedTransmit();
        }
    
    }

    bool IndividualHumanCoInfection::SetNewInfectionState(InfectionStateChange::_enum inf_state_change)
    {
        //trigger node level interventions
        ( (NodeTBHIV * ) parent ) ->SetNewInfectionState(inf_state_change, this);

        if ( IndividualHuman::SetNewInfectionState(inf_state_change) )
        {
            // Nothing is currently set in the base function (death and disease clearance are handled directly in the Update function)
        }
        else if ( inf_state_change == InfectionStateChange::Cleared )
        {
            m_new_infection_state = NewInfectionState::NewlyCleared;                  //  Additional reporting of cleared infections
            LOG_VALID_F( " Individual %lu has infectionstatechange Cleared \n", suid.data);
        }
        else if ( inf_state_change == InfectionStateChange::TBActivationPresymptomatic )   //  Latent infection that became active
        {
        // nothing right now
            LOG_VALID_F( "%s: Individual %lu has infectionstatechange TBActivationPresymptomatic.\n", __FUNCTION__, suid.data);
        }        
        else if ( inf_state_change == InfectionStateChange::TBActivation || inf_state_change == InfectionStateChange::TBActivationExtrapulm || inf_state_change == InfectionStateChange::TBActivationSmearNeg || inf_state_change == InfectionStateChange::TBActivationSmearPos )   //  Latent infection that became active
        {
            m_new_infection_state = NewInfectionState::NewlyActive;

            LOG_VALID_F( " Individual %lu has infectionstatechange TBActivation \n", suid.data);
        }
        else if (inf_state_change == InfectionStateChange::TBInactivation || inf_state_change == InfectionStateChange::ClearedPendingRelapse) //  Active infection that became latent
        {
            m_new_infection_state = NewInfectionState::NewlyInactive;
            LOG_VALID_F(" Individual %lu has infectionstatechange TBInactivation \n", suid.data);
        }
        else
        {
            return false;
        }

        return true;
    }

    bool IndividualHumanCoInfection::HasActiveInfection() const
    {
        bool ret = false;
        for (auto infection : infections)
        {
            IInfectionTB* pinfTB = NULL;
            if (s_OK != infection->QueryInterface(GET_IID( IInfectionTB ), (void**)&pinfTB) )
            {
                //LOG_DEBUG("This infection is not TB, so it cannot contribute to whether the individual has active infection \n");
                continue;
            }

            //must have tb infection to get down here
            if(infection->IsActive())
            {
                ret = true;
                break;
            }
        }
        LOG_DEBUG_F( "Individual %lu reporting %d for HasActiveInfection with %d infections.\n", suid.data, ret, infections.size() );
        return ret;
    }

    void IndividualHumanCoInfectionConfig::ConstructTBInfSusCD4Maps()
    {
        int size_strata = static_cast <int> ( TB_CD4_strata_for_infectiousness_susceptibility.size() );
        int size_prog = static_cast <int> (TB_CD4_Primary.size());
        int size_infect = static_cast <int> (TB_CD4_infectiousness.size());
        int size_sus = static_cast <int> (TB_CD4_Primary.size());

        if (size_prog != size_strata)
        {
            throw IncoherentConfigurationException(__FILE__, __LINE__, __FUNCTION__, "Length Primary Progression Vector", size_prog, "Length of CD4 Strata", size_strata);
        }
        else if (size_infect != size_strata)
        {
            throw IncoherentConfigurationException(__FILE__, __LINE__, __FUNCTION__, "Length CD4 Infectiousness Vector", size_infect, "Length of CD4 Strata", size_strata);
        }
        else if (size_sus != size_strata)
        {
            throw IncoherentConfigurationException(__FILE__, __LINE__, __FUNCTION__, "Length CD4 Susceptibility Vector", size_sus, "Length of CD4 Strata", size_strata);
        }

        auto it_primary_progression = TB_CD4_Primary.cbegin();
        auto it_infectiousness = TB_CD4_infectiousness.cbegin();
        auto it_cd4_strata = TB_CD4_strata_for_infectiousness_susceptibility.cbegin();
        auto it_susceptibility = TB_CD4_susceptibility.cbegin();

        //PUT IN ERROR CHECKING TO VERIFY VECTORS ARE IDENTICAL LENGTH
        while (  (it_infectiousness!= TB_CD4_infectiousness.cend() ) && (it_cd4_strata != TB_CD4_strata_for_infectiousness_susceptibility.cend() ) &&  (it_susceptibility != TB_CD4_susceptibility.cend() ) && (it_primary_progression != TB_CD4_Primary.cend() ) )
        {
            TB_CD4_Infectiousness_Map[*it_cd4_strata] = *it_infectiousness++;
            TB_CD4_Primary_Progression_Map[*it_cd4_strata] = *it_primary_progression++;
            TB_CD4_Susceptibility_Map[*it_cd4_strata++] = *it_susceptibility++;
            
        }
    }

    float IndividualHumanCoInfection::GetTBCD4InfectiousnessMap(float CD4) const
    {   //Do some quick linear interpolation
        //Do some quick linear interpolation hmm sort of replicating code here but in principle might want differnt interpolations so OK for now
        auto it_bound_above = IndividualHumanCoInfectionConfig::TB_CD4_Infectiousness_Map.lower_bound(CD4);
        auto it_bound_below = it_bound_above;
        if( it_bound_below == IndividualHumanCoInfectionConfig::TB_CD4_Infectiousness_Map.begin() )
        {
            return it_bound_below->second;
        }
        else if (it_bound_above == IndividualHumanCoInfectionConfig::TB_CD4_Infectiousness_Map.end())
        {
            it_bound_above--; 
            return it_bound_above->second;
        }
        else
        {
            it_bound_below--;
        }

        {
            float loc_infectiousness;
            loc_infectiousness = it_bound_below->second + (it_bound_above->second -  it_bound_below->second)/(it_bound_above->first - it_bound_below->first) * (CD4 - it_bound_below->first);
            return loc_infectiousness;
        }
    }

    float IndividualHumanCoInfection::GetCD4SusceptibilityMap( float CD4) const
    {
        //Do some quick linear interpolation hmm sort of replicating code here but in principle might want differnt interpolations so OK for now
        auto it_bound_above = IndividualHumanCoInfectionConfig::TB_CD4_Susceptibility_Map.lower_bound(CD4);
        auto it_bound_below = it_bound_above;
        if( it_bound_below == IndividualHumanCoInfectionConfig::TB_CD4_Susceptibility_Map.begin() )
        {
            return it_bound_below->second;
        }
        else if (it_bound_above == IndividualHumanCoInfectionConfig::TB_CD4_Susceptibility_Map.end())
        {
            it_bound_above--;
            return it_bound_above->second;
        }
        {
            it_bound_below--;
        }

        {
            float loc_susceptibility;
            loc_susceptibility = it_bound_below->second + (it_bound_above->second -  it_bound_below->second)/(it_bound_above->first - it_bound_below->first) * (CD4 - it_bound_below->first);
            return loc_susceptibility;
        }
    }

        float IndividualHumanCoInfection::GetCD4PrimaryMap( float CD4) const
    {
        //Do some quick linear interpolation hmm sort of replicating code here but in principle might want differnt interpolations so OK for now
        auto it_bound_above = IndividualHumanCoInfectionConfig::TB_CD4_Primary_Progression_Map.lower_bound(CD4);
        auto it_bound_below = it_bound_above;
        if( it_bound_below == IndividualHumanCoInfectionConfig::TB_CD4_Primary_Progression_Map.begin() )
        {
            return it_bound_below->second;
        }
        else if (it_bound_above == IndividualHumanCoInfectionConfig::TB_CD4_Primary_Progression_Map.end())
        {
            it_bound_above--;
            it_bound_above->second;
        }
        else
        {
            it_bound_below--;
        }

        {
            float loc_primary;
            loc_primary = it_bound_below->second + (it_bound_above->second -  it_bound_below->second)/(it_bound_above->first - it_bound_below->first) * (CD4 - it_bound_below->first);
            return loc_primary;
        }
    }

    bool IndividualHumanCoInfection::HasLatentInfection() const
    {
        LOG_DEBUG_F( "%s: infections.size() = %d.\n", __FUNCTION__, infections.size() );
        for (auto infection : infections)
        {
            IInfectionTB* pinfTB = NULL;
            if (s_OK != infection->QueryInterface(GET_IID( IInfectionTB ), (void**)&pinfTB) )
            {
                //LOG_DEBUG("This infection is not TB, so it cannot contribute to whether the individual has latent infection \n");
                continue;
            }

            //must have tb infection to get down here
            if(!infection->IsActive())
            {
                return true;
            }
        }
        return false;
    }

    bool IndividualHumanCoInfection::HasTB() const
    {
        LOG_DEBUG_F( "%s: infections.size() = %d.\n", __FUNCTION__, infections.size() );
        for (auto infection : infections)
        {
            IInfectionTB* pinfTB = NULL;
            if (s_OK != infection->QueryInterface(GET_IID( IInfectionTB ), (void**)&pinfTB) )
            {
                //LOG_DEBUG("This infection is not TB, so it cannot contribute to whether the individual has latent infection \n");
                continue;
            }
            //must have tb infection to get down here
            return true;

        }
        return false;
    }

    bool IndividualHumanCoInfection::IsSmearPositive() const
    {
        for (auto infection : infections)
        {
            IInfectionTB* pointerITB = NULL;
            if (s_OK != infection->QueryInterface(GET_IID( IInfectionTB ), (void**)&pointerITB) )
            {
                //LOG_DEBUG("This infection is not TB, so it cannot contribute to whether the individual has latent infection \n");
                continue;
            }

            if(pointerITB->IsSmearPositive())
            {
                return true;
            }
        }
        return false;
    }

    bool IndividualHumanCoInfection::IsMDR() const
    {
        for (auto infection : infections)
        {
            IInfectionTB* pointerITB = NULL;
            if (s_OK != infection->QueryInterface(GET_IID( IInfectionTB ), (void**)&pointerITB) )
            {
                //LOG_DEBUG("This infection is not TB, so it cannot contribute to whether the individual has latent infection \n");
                continue;
            }

            if(pointerITB->IsMDR()) 
            {
                return true;
            }
        }
        return false;
    }

    bool IndividualHumanCoInfection::IsExtrapulmonary() const
    {
        for (auto infection : infections)
        {
            IInfectionTB* pointerITB = NULL;
            if (s_OK != infection->QueryInterface(GET_IID( IInfectionTB ), (void**)&pointerITB) )
            {
                //LOG_DEBUG("This infection is not TB, so it cannot contribute to whether the individual has latent infection \n");
                continue;
            }

            if(pointerITB->IsExtrapulmonary()) 
            {
                return true;
            }
        }
        return false;
    }

    bool IndividualHumanCoInfection::HasActivePresymptomaticInfection() const
    {
        for (auto infection : infections)
        {
            IInfectionTB* pointerITB = NULL;
            if (s_OK != infection->QueryInterface(GET_IID( IInfectionTB ), (void**)&pointerITB) )
            {
                //LOG_DEBUG("This infection is not TB, so it cannot contribute to whether the individual has latent infection \n");
                continue;
            }

            if(!pointerITB->IsSymptomatic() && pointerITB->IsActive() ) 
            {
                return true;
            }
        }
        return false;
    }

    bool IndividualHumanCoInfection::HasPendingRelapseInfection() const
    {     
        for (auto infection : infections)
        {
            IInfectionTB* pointerITB = NULL;
            if (s_OK != infection->QueryInterface(GET_IID( IInfectionTB ), (void**)&pointerITB) )
            {
                //LOG_DEBUG("This infection is not TB, so it cannot contribute to whether the individual has latent infection \n");
                continue;
            }

            if(pointerITB->IsPendingRelapse())
            {
                return true;
            }
        }
        return false;
    }

    bool IndividualHumanCoInfection::HasEverRelapsedAfterTreatment() const
    {
        // Query for intervention container
        IIndividualHumanInterventionsContext *context = NULL;
        ITBInterventionsContainer * itbivc = NULL;

        context = GetInterventionsContext();
        if (s_OK != context->QueryInterface(GET_IID(ITBInterventionsContainer), (void**)&itbivc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "ITBInterventionsContainer", "IIndividualHumanInterventionsContext" );
        }

        return itbivc->GetTxEverRelapsedStatus();
    }

    bool IndividualHumanCoInfection::HasFailedTreatment() const
    {
        // Query for intervention container
        IIndividualHumanInterventionsContext *context = NULL;
        ITBInterventionsContainer * itbivc = NULL;

        context = GetInterventionsContext();
        if (s_OK != context->QueryInterface(GET_IID(ITBInterventionsContainer), (void**)&itbivc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "ITBInterventionsContainer", "IIndividualHumanInterventionsContext" );
        }

        return itbivc->GetTxFailedStatus();
    }

    int IndividualHumanCoInfection::GetTime() const
    {
        return parent->GetTime().time;
    }

    bool IndividualHumanCoInfection::IsFastProgressor() const
    {
        for (auto infection : infections)
        {
            IInfectionTB * pointerITB = NULL;

            if (infection->QueryInterface(GET_IID(IInfectionTB), (void**)& pointerITB) == s_OK )
            {
                if ( pointerITB->IsFastProgressor() )
                {
                    return true;
                }
            }
        }

        return false;

    }

    bool IndividualHumanCoInfection::IsTreatmentNaive() const
    {
        // Query for intervention container, in future cache TB Intervention container when we create it 
        IIndividualHumanInterventionsContext *context = NULL;
        ITBInterventionsContainer * itbivc = NULL;

        context = GetInterventionsContext();
        if (s_OK != context->QueryInterface(GET_IID(ITBInterventionsContainer), (void**)&itbivc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "ITBInterventionsContainer", "IIndividualHumanInterventionsContext" );
        }

        return itbivc->GetTxNaiveStatus();
    }

    bool IndividualHumanCoInfection::IsOnTreatment() const 
    { 
        // Query for intervention container
        IIndividualHumanInterventionsContext *context = NULL;
        ITBInterventionsContainer * itbivc = NULL;

        context = GetInterventionsContext();
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

    bool IndividualHumanCoInfection::IsEvolvedMDR() const
    {
        for (auto infection : infections)
        {
            IInfectionTB * pointerITB = NULL;

            if (infection->QueryInterface(GET_IID(IInfectionTB), (void**) &pointerITB) == s_OK )
            {
                if ( pointerITB->IsMDR() && pointerITB->EvolvedResistance() )
                {
                    return true;
                }
            }
        }
        return false;
    }


    bool IndividualHumanCoInfection::HasHIV() const
    {
        bool ret = false;
        for (auto infection : infections)
        {
            IInfectionHIV* pinfHIV = NULL;
            if (s_OK == infection->QueryInterface(GET_IID( IInfectionHIV ), (void**)&pinfHIV) )
            {
                ret = true;
                break;
            }
        }
        return ret;
    }

    float IndividualHumanCoInfection::GetDurationSinceInitInfection() const
    {
        float ret = -1.0f;
        for(auto infection: infections) //note this functin only works with one infection, it will return the answer for the first infection only
        {
            IInfectionTB* pointerITB = NULL;
            if (infection->QueryInterface(GET_IID(IInfectionTB), (void**) &pointerITB) == s_OK )
            {
                ret = pointerITB->GetDurationSinceInitialInfection();
            }
        }
        return ret;
    }

    float IndividualHumanCoInfection::GetCD4() const
    {// return 1000 if healthy (this could be improved by introducing variability in CD4 among healthy individuals)
        float ret = 1000.00;
        for (auto susceptibility : susceptibilitylist)
        {
            ISusceptibilityHIV* pointer_to_HIV_susceptibility = NULL;
            if (s_OK == susceptibility->QueryInterface(GET_IID( ISusceptibilityHIV ), (void**)&pointer_to_HIV_susceptibility) )
            {
                ret = pointer_to_HIV_susceptibility->GetCD4count();
                break;
            }
        }
        return ret;
    }

    void IndividualHumanCoInfection::InitiateART()
    {
        /* clorton float current_CD4 = */ GetCD4();
        for (auto susceptibility : susceptibilitylist)
        {
            ISusceptibilityHIV* pointer_to_HIV_susceptibility = NULL;
            if (s_OK == susceptibility->QueryInterface(GET_IID( ISusceptibilityHIV ), (void**)&pointer_to_HIV_susceptibility) )
            {
                throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "change CD4 trajectory not supported" ); //pointer_to_HIV_susceptibility->InitiateART(); // change CD4 trajectory
                break;
            }
            m_has_ever_been_onART = 1;
        }
    }

    NaturalNumber IndividualHumanCoInfection::GetViralLoad() const
    {
        NaturalNumber ret = 0;
        for (auto infection : infections)
        {
            IInfectionHIV* pinfHIV = NULL;
            if (s_OK == infection->QueryInterface(GET_IID( IInfectionHIV ), (void**)&pinfHIV) )
            {
                int vl = pinfHIV->GetViralLoad();
                ret += vl;
            }
        }
        return ret;
    }

    bool IndividualHumanCoInfection::IsImmune() const
    {
        //this is ONLY used by ReportTB. 
        //People get IsImmune when they get an infection, then with some period of infection-less time, they may lose their immunity

        return susceptibility_tb->IsImmune();
    }

    Infection* IndividualHumanCoInfection::createInfection( suids::suid _suid )
    {
        InfectionHIV* new_inf = InfectionHIV::CreateInfection(this, _suid);
        return (Infection*) new_inf;
    }

    bool IndividualHumanCoInfection::createInfection( suids::suid _suid, infection_list_t &newInfections )
    {
        InfectionTB* new_inf = InfectionTB::CreateInfection(this, _suid);
        newInfections.push_back( new_inf );
        return true;
    }

    IIndividualHumanInterventionsContext* IndividualHumanCoInfection::GetInterventionsContextbyInfection(Infection* infection) 
    {
        return (IIndividualHumanInterventionsContext*)interventions;
    }

    void IndividualHumanCoInfection::setupInterventionsContainer()
    {
        interventions = _new_ MasterInterventionsContainer();
        //IIndividualHumanContext *indcontext = GetContextPointer();
        interventions->SetContextTo(this); //TODO: fix this when init pattern standardized <ERAD-291>  PE: See comment above
    }

    void IndividualHumanCoInfection::ModActivate()  
    {
        for (auto susceptibility : susceptibilitylist)
        { 
            ISusceptibilityHIV* pHIVsus = NULL;
            if (s_OK == susceptibility->QueryInterface(GET_IID( ISusceptibilityHIV ), (void**)&pHIVsus) )
            {
                if (HasHIV() )
                { 
                    bool isonart = false;
                    if (GetHIVInterventionsContainer()->OnArtQuery() && GetHIVInterventionsContainer()->ShouldReconstituteCD4() )
                    {
                        isonart = true;
                    }

                    vector <float> CD4_future = pHIVsus->Generate_forward_CD4( isonart );
                    std::string cd4_future_string;
                    for( auto cd4: CD4_future  )
                    {
                        cd4_future_string += std::to_string( cd4 );
                        cd4_future_string += ", ";
                    }
                    LOG_VALID_F( "Individual %d, CD4_future = %sisonart = %d.\n", GetSuid().data, cd4_future_string.c_str(), isonart );
                    std::vector <float> v_act(CD4_future.size(), 0.0f); 
                    std::vector<float>::iterator vout;

                    release_assert( IndividualHumanCoInfectionConfig::CD4_act_map.size() > 0 );
                    for (auto vin = CD4_future.begin(), vout = v_act.begin() ; vin != CD4_future.end(); ++vin, ++vout)  
                    {
                        auto temp = IndividualHumanCoInfectionConfig::CD4_act_map.lower_bound(*vin);
                        if (temp != IndividualHumanCoInfectionConfig::CD4_act_map.end())
                        {
                            *vout = temp->second;
                        }
                        else
                        {
                            auto it_to_last = temp;
                            it_to_last--;
                            *vout = it_to_last->second;
                        }

                        if (isonart)
                        {
                            *vout = (*vout ) * IndividualHumanCoInfectionConfig::ART_extra_reactivation_reduction;  //additional ART factor
                        }
                    }

                    SetForwardTBAct(v_act);
                }
                else
                {
                    LOG_WARN("Trying to activate HIV negative as though HIV positive");
                    //SetForwardTBAct( std::vector <float>(CD4_forward_vector.size(), 5.0e-6f) );  //vector of one element equal to one for consistency
                    std::vector <float> v_def( CD4_forward_vector.size(), 5.0e-6f );
                    SetForwardTBAct( v_def );
                    // 5.0e-6 is intended to be default
                }
            }
        }
    }
#if 0
    void IndividualHumanCoInfection::RegisterInfectionIncidenceObserver(
        IInfectionIncidenceObserver * pObserver 
    )
    {
        infectionIncidenceObservers.push_back(pObserver);
    }
#endif
    void IndividualHumanCoInfection::LifeCourseLatencyUpdateAll()
    {
        for (auto infection: infections)
        {
            IInfectionTB* pITB= NULL;
            if (s_OK == infection->QueryInterface(GET_IID( IInfectionTB ), (void**)&pITB) )
            {
                if (!pITB->IsActive() && !(pITB->IsFastProgressor() || pITB->IsPendingRelapse() ))
                {
                    LifeCourseLatencyTimerUpdate(pITB) ; //get sus
                }
            }  
        }
    }

    void IndividualHumanCoInfection::LifeCourseLatencyTimerUpdate( IInfectionTB* infection )
    {
        ModActivate();
        infection->LifeCourseLatencyTimerUpdate();
    }

    void
        IndividualHumanCoInfection::SetTBActivationVector(
        const std::vector<float>& vin
        )
    {
        TB_activation_vector = vin;
    }

    const std::vector<float>&
        IndividualHumanCoInfection::GetTBActivationVector()
        const 
    {
        return TB_activation_vector;
    }

    float
    IndividualHumanCoInfection::GetNextLatentActivation(
        float time_incr
    )
    const
    {
        vector <float> forward_act = GetTBActivationVector();
        int v_size = forward_act.size();

        if (time_incr/ SusceptibilityHIVConfig::cd4_time_step >  (float) (v_size -1) )
        {
            return forward_act.back();
        }
        else
        {
            return  forward_act.at((int) (time_incr/ SusceptibilityHIVConfig::cd4_time_step ) );
        }
    }

    IInfectionHIV*
    IndividualHumanCoInfection::GetHIVInfection()
    const
    {
        for (auto infection : infections)
        {
            IInfectionHIV* pointerIInfHIV = NULL;
            if (s_OK == infection->QueryInterface(GET_IID( IInfectionHIV ), (void**)&pointerIInfHIV) )
            {
                //LOG_DEBUG("This is the HIV infection, get pointer ");
                return pointerIInfHIV;
            }  
        }
        LOG_WARN_F("Individual %lu given ART but not HIV positive \n", suid.data);  //cases where this could arise: eg Giving ART to HIV negative
        return nullptr;
    }

    IHIVInterventionsContainer*  IndividualHumanCoInfection::GetHIVInterventionsContainer() const
    { 
        MasterInterventionsContainer* test = dynamic_cast <MasterInterventionsContainer*> (interventions);
        IHIVInterventionsContainer* pHIVIC = NULL;
        for (auto interventions :test->InterventionsContainerList)
        {
            if ( interventions->QueryInterface(GET_IID(IHIVInterventionsContainer), (void**)&pHIVIC) == s_OK)
            {
                return pHIVIC;
            }
        }
        throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "interventions", "IHIVInterventionsContainer" );
    }

    ITBInterventionsContainer*  IndividualHumanCoInfection::GetTBInterventionsContainer() const
    { 
        MasterInterventionsContainer* test = dynamic_cast <MasterInterventionsContainer*> (interventions);
        ITBInterventionsContainer* pTBIC = NULL;
        for (auto interventions :test->InterventionsContainerList)
        {
            if ( interventions->QueryInterface(GET_IID(ITBInterventionsContainer), (void**)&pTBIC) == s_OK)
            {
                return pTBIC;
            }
        }
        throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "interventions", "IHIVInterventionsContainer" );
    }

    ISusceptibilityHIV* IndividualHumanCoInfection::GetHIVSusceptibility() const
    { 
        for (auto susceptibility : susceptibilitylist)
        {
            ISusceptibilityHIV* pSusHIV = NULL;
            if (s_OK == susceptibility->QueryInterface(GET_IID( ISusceptibilityHIV ), (void**)&pSusHIV) )
            {
                //LOG_DEBUG("This is the HIV infection, get pointer ");
                return pSusHIV;
            }  
        }
        return nullptr;
    }

    IInfectionTB* IndividualHumanCoInfection::GetTBInfection() const 
    {
        for (auto infection : infections)
        {
            IInfectionTB* pointerTB = NULL;
            if (s_OK == infection->QueryInterface(GET_IID( IInfectionTB ), (void**)&pointerTB) )
            {
                //LOG_DEBUG("This is the TB infection, get pointer ");
                return pointerTB;
            }  
        }
        throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "infection", "InfectionHIV" ); //We should never get down here
    }

    std::string IndividualHumanCoInfection::toString() const
    {
        std::ostringstream me;
        me << "id="
            << GetSuid().data
            << ",gender="
            << ( GetGender()==Gender::MALE ? "male" : "female" )
            << ",age="
            << GetAge()/DAYSPERYEAR
            << ",num_infections="
            << infections.size()
            // << ",num_relationships="
            // << relationships.size()
            //<< ",num_relationships_lifetime="
            // << num_lifetime_relationships
            //<< ",num_relationships_last_6_months="
            //<< last_6_month_relationships.size()
            //<< ",promiscuity_flags="
            //<< std::hex << static_cast<unsigned>(promiscuity_flags)
            ;
        return me.str();
    }
#if 0
    void IndividualHumanCoInfection::UnRegisterAllObservers(
        IInfectionIncidenceObserver * pObserver 
    )
    {

        // -----------------------------------------------------------------------------------
        // --- infectionIncidenceObservers used to be a std::set but in TB tests 29, 31, & 32
        // --- switching to a vector was much faster.
        // -----------------------------------------------------------------------------------
        for (int i = 0; i < infectionIncidenceObservers.size(); ++i)
        {
            if (infectionIncidenceObservers[i] == pObserver)
            {
                infectionIncidenceObservers[i] = infectionIncidenceObservers.back();
                infectionIncidenceObservers.pop_back();
                return;
            }
        }
    } 
#endif
    bool IndividualHumanCoInfection::InfectionExistsForThisStrain(IStrainIdentity* check_strain_id)
    {
        IInfectionTB* pTB = nullptr;
        for (auto infection : infections)
        {
            if (s_OK == infection->QueryInterface(GET_IID(IInfectionTB), (void**)&pTB))
            {
                //LOG_DEBUG("This is the TB infection");

                if (infection->StrainMatches(check_strain_id))
                {
                    return true;
                }
                
            }
                
        }

        return false;
    }

    void IndividualHumanCoInfection::SetExogenousInfectionStrain(IStrainIdentity* exog_strain_id)
    {
        IInfectionTB* pTB = nullptr;
        for (auto infection : infections)
        {
            if (s_OK == infection->QueryInterface(GET_IID(IInfectionTB), (void**)&pTB))
            {
                //LOG_DEBUG("This is the TB infection");

                if (!pTB->IsActive() && !infection->StrainMatches(exog_strain_id )  )
                {
                    InfectionTB * inf = dynamic_cast <InfectionTB*> ( infection  );
        
                    inf->ModifyInfectionStrain(exog_strain_id);
                    break;
                } 
            } 
        } 
    } 

    void IndividualHumanCoInfection::Die(HumanStateChange newState)
    {
        StateChange = newState;
        switch (newState)
        {
            case HumanStateChange::DiedFromNaturalCauses:
                {
                    LOG_DEBUG_F("%s: individual %d (%s) died of natural causes at age %f with daily_mortality_rate = %f\n", __FUNCTION__, suid.data, (GetGender() == Gender::FEMALE ? "Female" : "Male"), GetAge() / DAYSPERYEAR, m_daily_mortality_rate);
                    broadcaster->TriggerNodeEventObservers(GetEventContext(), EventTrigger::NonDiseaseDeaths);
                }
            break;

            case HumanStateChange::KilledByInfection:
                {
                    LOG_DEBUG_F("%s: individual %d died from infection\n", __FUNCTION__, suid.data);
                    broadcaster->TriggerNodeEventObservers(GetEventContext(), EventTrigger::DiseaseDeaths);
                }
            break;
            case HumanStateChange::KilledByOpportunisticInfection:
                {
                    LOG_DEBUG_F("%s: individual %d died from non-TB opportunistic infection\n", __FUNCTION__, suid.data);
                    broadcaster->TriggerNodeEventObservers(GetEventContext(), EventTrigger::OpportunisticInfectionDeath);

                }
            break;

            default:
                release_assert(false);
            break;
        }
    }

    bool IndividualHumanCoInfection::IsDead() const
    {
        auto state_change = GetStateChange();
        bool is_dead = (
                ((state_change == HumanStateChange::DiedFromNaturalCauses) ||
                 (state_change == HumanStateChange::KilledByInfection) ||
                 (state_change == HumanStateChange::KilledByOpportunisticInfection)||
                 (state_change == HumanStateChange::KilledByCoinfection)
                )
                || (state_change == HumanStateChange::KilledByMCSampling) );
        return is_dead;
    }

    void IndividualHumanCoInfection::PropagateContextToDependents()
    {
        IIndividualHumanContext *context = GetContextPointer();

        // fix up child pointers

        for (auto suscept : susceptibilitylist)
        {
            suscept->SetContextTo(context);
            ISusceptibilityTB* psustb = nullptr;
            ISusceptibilityHIV* psushiv = nullptr;

            if (s_OK == suscept->QueryInterface(GET_IID(ISusceptibilityTB), (void**)&psustb))
            {
                susceptibility_tb = suscept;
            }
            else if (s_OK == suscept->QueryInterface(GET_IID(ISusceptibilityHIV), (void**)&psushiv))
            {
                susceptibility_hiv = suscept;
            }
        }

        for (auto infection : infections)
        {
            infection->SetContextTo(context);

            IInfectionTB* pinfection = nullptr;
            IInfectionHIV* hinfection = nullptr;

            if (s_OK == infection->QueryInterface(GET_IID(IInfectionTB), (void**)&pinfection))
            {
                infection2susceptibilitymap[infection] = susceptibility_tb;
            }
            else if (s_OK == infection->QueryInterface(GET_IID(IInfectionHIV), (void**)&hinfection))
            {
                infection2susceptibilitymap[infection] = susceptibility_hiv;
            }
        }

        if (interventions)  interventions->SetContextTo(context);
    }

    void IndividualHumanCoInfection::ResetCounters( void )
    {
        infectionMDRIncidenceCounter = 0.0f;
        mdr_evolved_incident_counter = 0.0f;
        new_mdr_fast_active_infection_counter = 0.0f;
    }

    REGISTER_SERIALIZABLE(IndividualHumanCoInfection);

    void IndividualHumanCoInfection::serialize(IArchive& ar, IndividualHumanCoInfection* obj)
    {
        IndividualHumanAirborne::serialize(ar, obj);
        IndividualHumanCoInfection& individual = *obj;
        ar.labelElement("suceptibilitylist") & individual.susceptibilitylist;
        ar.labelElement("infectioncount_tb") & individual.infectioncount_tb;
        ar.labelElement("infectioncount_hiv") & individual.infectioncount_hiv;
        ar.labelElement("m_bool_exogenous") & individual.m_bool_exogenous;
        ar.labelElement("m_has_ever_been_onART") & individual.m_has_ever_been_onART;
        ar.labelElement("m_has_ever_tested_positive_for_HIV")  & individual.m_has_ever_tested_positive_for_HIV;
    }

    float IndividualHumanCoInfection::GetImmunityReducedAcquire()
    {
        return susceptibility_tb->getModAcquire();
    }
}

