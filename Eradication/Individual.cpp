/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "Individual.h"

#include <list>
#include <map>
#include <string>

#include "Common.h"
#include "IContagionPopulation.h"
#include "Debug.h"
#include "Exceptions.h"
#include "IndividualEventContext.h"
#include "Infection.h"
#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "IMigrationInfo.h"
#include "INodeContext.h"
#include "NodeDemographics.h"
#include "NodeEventContext.h"
#include "SimulationConfig.h"
#include "suids.hpp"
#include "Susceptibility.h"

#include "RapidJsonImpl.h" // Once JSON lib wrapper is completely done, this underlying JSON library specific include can be taken out
#include <IArchive.h>

static const char * _module = "Individual";

namespace Kernel
{
    bool IndividualHumanConfig::aging = true;
    float IndividualHumanConfig::local_roundtrip_prob = 0.0f;
    float IndividualHumanConfig::air_roundtrip_prob = 0.0f;
    float IndividualHumanConfig::region_roundtrip_prob = 0.0f;
    float IndividualHumanConfig::sea_roundtrip_prob = 0.0f;
    float IndividualHumanConfig::family_roundtrip_prob = 0.0f;
    float IndividualHumanConfig::local_roundtrip_duration_rate = 0.0f;
    float IndividualHumanConfig::air_roundtrip_duration_rate = 0.0f;
    float IndividualHumanConfig::region_roundtrip_duration_rate = 0.0f;
    float IndividualHumanConfig::sea_roundtrip_duration_rate = 0.0f;
    float IndividualHumanConfig::family_roundtrip_duration_rate = 0.0f;
    int IndividualHumanConfig::infection_updates_per_tstep = 0.0f;
    MigrationPattern::Enum IndividualHumanConfig::migration_pattern = MigrationPattern::RANDOM_WALK_DIFFUSION;
    bool IndividualHumanConfig::immunity = false;
    int IndividualHumanConfig::roundtrip_waypoints = 0;
    int IndividualHumanConfig::max_ind_inf = 0;
    bool IndividualHumanConfig::superinfection = 0;
    float IndividualHumanConfig::x_othermortality = 0.0f;
    float IndividualHumanConfig::min_adult_age_years = 15.0f;

    // QI stuff in case we want to use it more extensively outside of campaigns
    GET_SCHEMA_STATIC_WRAPPER_IMPL(Individual,IndividualHumanConfig)
    BEGIN_QUERY_INTERFACE_BODY(IndividualHumanConfig)
    END_QUERY_INTERFACE_BODY(IndividualHumanConfig)

    bool IndividualHumanConfig::IsAdultAge( float years )
    {
        return (min_adult_age_years <= years);
    }

    bool IndividualHumanConfig::CanSupportFamilyTrips( IMigrationInfoFactory* pmif )
    {
        bool not_supported = (migration_pattern != MigrationPattern::SINGLE_ROUND_TRIPS)
                          || (pmif->IsEnabled( MigrationType::LOCAL_MIGRATION    ) && (local_roundtrip_prob  != 1.0))
                          || (pmif->IsEnabled( MigrationType::AIR_MIGRATION      ) && (air_roundtrip_prob    != 1.0))
                          || (pmif->IsEnabled( MigrationType::REGIONAL_MIGRATION ) && (region_roundtrip_prob != 1.0))
                          || (pmif->IsEnabled( MigrationType::SEA_MIGRATION      ) && (sea_roundtrip_prob    != 1.0));

        if( not_supported && pmif->IsEnabled( MigrationType::FAMILY_MIGRATION ) )
        {
            std::stringstream msg;
            msg << "Invalid Configuration for Family Trips." << std::endl;
            msg << "Migration_Pattern must be SINGLE_ROUND_TRIPS and the 'XXX_Migration_Roundtrip_Probability' must equal 1.0 if that Migration Type is enabled." << std::endl;
            msg << "Migration_Pattern = " << MigrationPattern::pairs::lookup_key( migration_pattern ) << std::endl;
            msg << "Enable_Local_Migration = "    << pmif->IsEnabled( MigrationType::LOCAL_MIGRATION    ) << " and Local_Migration_Roundtrip_Probability = "    << local_roundtrip_prob  << std::endl;
            msg << "Enable_Air_Migration = "      << pmif->IsEnabled( MigrationType::AIR_MIGRATION      ) << " and Air_Migration_Roundtrip_Probability = "      << air_roundtrip_prob    << std::endl;
            msg << "Enable_Regional_Migration = " << pmif->IsEnabled( MigrationType::REGIONAL_MIGRATION ) << " and Regional_Migration_Roundtrip_Probability = " << region_roundtrip_prob << std::endl;
            msg << "Enable_Sea_Migration = "      << pmif->IsEnabled( MigrationType::SEA_MIGRATION      ) << " and Sea_Migration_Roundtrip_Probability = "      << sea_roundtrip_prob    << std::endl;
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
        return !not_supported;
    }

    //------------------------------------------------------------------
    //   Initialization methods
    //------------------------------------------------------------------

    bool
    IndividualHumanConfig::Configure(
        const Configuration* config
    )
    {
        LOG_DEBUG( "Configure\n" );
        initConfigTypeMap( "Enable_Aging", &aging, Enable_Aging_DESC_TEXT, true );
        initConfigTypeMap( "Infection_Updates_Per_Timestep", &infection_updates_per_tstep, Infection_Updates_Per_Timestep_DESC_TEXT, 0, 144, 1 );
        initConfigTypeMap( "Enable_Immunity", &immunity, Enable_Immunity_DESC_TEXT, true );
        initConfigTypeMap( "Max_Individual_Infections", &max_ind_inf, Max_Individual_Infections_DESC_TEXT, 0, 1000, 1 );
        initConfigTypeMap( "Enable_Superinfection", &superinfection, Enable_Superinfection_DESC_TEXT, false );
        initConfigTypeMap( "x_Other_Mortality", &x_othermortality, x_Other_Mortality_DESC_TEXT, 0.0f, FLT_MAX, 1.0f );
        initConfigTypeMap( "Minimum_Adult_Age_Years", &min_adult_age_years, Minimum_Adult_Age_Years_DESC_TEXT, 0.0f, FLT_MAX, 15.0f );

        MigrationStructure::Enum migration_structure; // TBD: Would be nice to get from SimulationConfig, but fakeHuman is configured first
        initConfig( "Migration_Model", migration_structure, config, MetadataDescriptor::Enum("migration_structure", Migration_Model_DESC_TEXT, MDD_ENUM_ARGS(MigrationStructure)) );

        if( migration_structure != MigrationStructure::NO_MIGRATION || JsonConfigurable::_dryrun)
        {
            initConfig( "Migration_Pattern", migration_pattern, config, MetadataDescriptor::Enum("migration_pattern", Migration_Pattern_DESC_TEXT, MDD_ENUM_ARGS(MigrationPattern)), "Migration_Structure", "Not NO_MIGRATION" );

            if ( JsonConfigurable::_dryrun )
            {
                RegisterRandomWalkDiffusionParameters();
                RegisterSingleRoundTripsParameters();
                RegisterWaypointsHomeParameters();
            }
            else if(migration_pattern == MigrationPattern::RANDOM_WALK_DIFFUSION)
            {
                LOG_DEBUG( "Following migration = RANDOM_WALK_DIFFUSION path\n" );
                local_roundtrip_prob      = 0.0f;
                air_roundtrip_prob        = 0.0f;
                region_roundtrip_prob     = 0.0f;
                sea_roundtrip_prob        = 0.0f;

                RegisterRandomWalkDiffusionParameters();
            }
            else if( migration_pattern == MigrationPattern::SINGLE_ROUND_TRIPS )
            {
                LOG_DEBUG( "Following migration = single_round_trip path\n" );
                RegisterSingleRoundTripsParameters();

                roundtrip_waypoints = 1;
            }
            else if( migration_pattern == MigrationPattern::WAYPOINTS_HOME )
            {
                LOG_DEBUG( "Following migration = WAYPOINTS_HOME path\n" );
                local_roundtrip_prob      = 1.0f;
                air_roundtrip_prob        = 1.0f;
                region_roundtrip_prob     = 1.0f;
                sea_roundtrip_prob        = 1.0f;
                family_roundtrip_prob     = 1.0f;

                local_roundtrip_duration_rate  = 0.0f;
                air_roundtrip_duration_rate    = 0.0f;
                region_roundtrip_duration_rate = 0.0f;
                sea_roundtrip_duration_rate    = 0.0f;
                family_roundtrip_duration_rate = 0.0f;

                RegisterWaypointsHomeParameters();
            }
        }

        bool bRet = JsonConfigurable::Configure( config );
        if( local_roundtrip_duration_rate != 0 )
        {
             local_roundtrip_duration_rate = 1.0f/local_roundtrip_duration_rate;
        }
        if( region_roundtrip_duration_rate != 0 )
        {
             region_roundtrip_duration_rate = 1.0f/region_roundtrip_duration_rate;
        }
        if( air_roundtrip_duration_rate != 0 )
        {
             air_roundtrip_duration_rate = 1.0f/air_roundtrip_duration_rate;
        }
        if( sea_roundtrip_duration_rate != 0 )
        {
             sea_roundtrip_duration_rate = 1.0f/sea_roundtrip_duration_rate;
        }
        if( family_roundtrip_duration_rate != 0 )
        {
             family_roundtrip_duration_rate = 1.0f/family_roundtrip_duration_rate;
        }

        if (superinfection && (max_ind_inf < 2))
        {
            throw IncoherentConfigurationException(__FILE__, __LINE__, __FUNCTION__, "Max_Individual_Infections", max_ind_inf, "Enable_Superinfection", superinfection);
        }

        InfectionConfig fakeInfection;
        fakeInfection.Configure( config );
        SusceptibilityConfig fakeImmunity;
        fakeImmunity.Configure( config );

        return bRet;
    }

    void IndividualHumanConfig::RegisterRandomWalkDiffusionParameters()
    {
        // Placeholder for now.
    }

    void IndividualHumanConfig::RegisterSingleRoundTripsParameters()
    {
        initConfigTypeMap( "Local_Migration_Roundtrip_Probability", &local_roundtrip_prob, Local_Migration_Roundtrip_Probability_DESC_TEXT, 0.0f, 1.0f, 0.95f, "Migration_Pattern", "SINGLE_ROUND_TRIPS" );
        initConfigTypeMap( "Air_Migration_Roundtrip_Probability", &air_roundtrip_prob, Air_Migration_Roundtrip_Probability_DESC_TEXT, 0.0f, 1.0f, 0.8f, "Migration_Pattern", "SINGLE_ROUND_TRIPS"  );
        initConfigTypeMap( "Regional_Migration_Roundtrip_Probability", &region_roundtrip_prob, Regional_Migration_Roundtrip_Probability_DESC_TEXT, 0.0f, 1.0f, 0.1f, "Migration_Pattern", "SINGLE_ROUND_TRIPS"  );
        initConfigTypeMap( "Sea_Migration_Roundtrip_Probability", &sea_roundtrip_prob, Sea_Migration_Roundtrip_Probability_DESC_TEXT, 0.0f, 1.0f, 0.25f, "Migration_Pattern", "SINGLE_ROUND_TRIPS"  );
        //initConfigTypeMap( "Family_Migration_Roundtrip_Probability", &family_roundtrip_prob, Family_Migration_Roundtrip_Probability_DESC_TEXT, 0.0f, 1.0f, 0.25f, "Migration_Pattern", "SINGLE_ROUND_TRIPS"  );
        family_roundtrip_prob = 1.0;

        initConfigTypeMap( "Local_Migration_Roundtrip_Duration", &local_roundtrip_duration_rate, Local_Migration_Roundtrip_Duration_DESC_TEXT, 0.0f, 10000.0f, 1.0f, "Migration_Pattern", "SINGLE_ROUND_TRIPS"  );
        initConfigTypeMap( "Air_Migration_Roundtrip_Duration", &air_roundtrip_duration_rate, Air_Migration_Roundtrip_Duration_DESC_TEXT, 0.0f, 10000.0f, 1.0f, "Migration_Pattern", "SINGLE_ROUND_TRIPS"  );
        initConfigTypeMap( "Regional_Migration_Roundtrip_Duration", &region_roundtrip_duration_rate, Regional_Migration_Roundtrip_Duration_DESC_TEXT, 0.0f, 10000.0f, 1.0f, "Migration_Pattern", "SINGLE_ROUND_TRIPS"  );
        initConfigTypeMap( "Sea_Migration_Roundtrip_Duration", &sea_roundtrip_duration_rate, Sea_Migration_Roundtrip_Duration_DESC_TEXT, 0.0f, 10000.0f, 1.0f, "Migration_Pattern", "SINGLE_ROUND_TRIPS"  );
        initConfigTypeMap( "Family_Migration_Roundtrip_Duration", &family_roundtrip_duration_rate, Family_Migration_Roundtrip_Duration_DESC_TEXT, 0.0f, 10000.0f, 1.0f, "Migration_Pattern", "SINGLE_ROUND_TRIPS"  );
    }

    void IndividualHumanConfig::RegisterWaypointsHomeParameters()
    {
        initConfigTypeMap( "Roundtrip_Waypoints", &roundtrip_waypoints, Roundtrip_Waypoints_DESC_TEXT, 0, 1000, 10 , "Migration_Pattern", "WAYPOINTS_HOME");
    }

    IndividualHuman::IndividualHuman(suids::suid _suid, float mc_weight, float initial_age, int gender, float poverty_factor)
        : suid(_suid)
        , m_age(initial_age)
        , m_gender(gender)
        , m_mc_weight(mc_weight)
        , m_daily_mortality_rate(0)
        , above_poverty(poverty_factor > 0.0f)
        , is_pregnant(false)
        , pregnancy_timer(FLT_MAX)
        , susceptibility(nullptr)
        , infections()
        , interventions(nullptr)
        , transmissionGroupMembership()
        , transmissionGroupMembershipByRoute()
        , m_is_infected(false)
        , infectiousness(0.0f)
        , Inf_Sample_Rate(0)
        , cumulativeInfs(0)
        , m_new_infection_state(NewInfectionState::Invalid)
        , StateChange(HumanStateChange::None)
        , migration_mod(0)
        , migration_type(MigrationType::NO_MIGRATION)
        , migration_destination(suids::nil_suid())
        , migration_time_until_trip(0.0)
        , migration_time_at_destination(0.0)
        , migration_is_destination_new_home(false)
        , migration_will_return(false)
        , migration_outbound(false)
        , max_waypoints(0)
        , waypoints()
        , waypoints_trip_type()
        , waiting_for_family_trip(false)
        , leave_on_family_trip(false)
        , is_on_family_trip(false)
        , family_migration_destination(suids::nil_suid())
        , family_migration_type(MigrationType::NO_MIGRATION)
        , family_migration_time_until_trip(0.0)
        , family_migration_time_at_destination(0.0)
        , family_migration_is_destination_new_home(false)
        , home_node_id(suids::nil_suid())
        , Properties()
        , m_PropertyReportString()
        , parent(nullptr)
        , broadcaster(nullptr)
    {
    }

    IndividualHuman::IndividualHuman(INodeContext *context)
        : suid(suids::nil_suid())
        , m_age(0)
        , m_gender(0)
        , m_mc_weight(0)
        , m_daily_mortality_rate(0)
        , above_poverty(0)
        , is_pregnant(false)
        , pregnancy_timer(FLT_MAX)
        , susceptibility(nullptr)
        , infections()
        , interventions(nullptr)
        , transmissionGroupMembership()
        , transmissionGroupMembershipByRoute()
        , m_is_infected(false)
        , infectiousness(0.0f)
        , Inf_Sample_Rate(0)
        , cumulativeInfs(0)
        , m_new_infection_state(NewInfectionState::Invalid)
        , StateChange(HumanStateChange::None)
        , migration_mod(0)
        , migration_type(MigrationType::NO_MIGRATION)
        , migration_destination(suids::nil_suid())
        , migration_time_until_trip(0.0)
        , migration_time_at_destination(0.0)
        , migration_is_destination_new_home(false)
        , migration_will_return(false)
        , migration_outbound(false)
        , max_waypoints(0)
        , waypoints()
        , waypoints_trip_type()
        , waiting_for_family_trip(false)
        , leave_on_family_trip(false)
        , is_on_family_trip(false)
        , family_migration_destination(suids::nil_suid())
        , family_migration_type(MigrationType::NO_MIGRATION)
        , family_migration_time_until_trip(0.0)
        , family_migration_time_at_destination(0.0)
        , family_migration_is_destination_new_home(false)
        , home_node_id(suids::nil_suid())
        , Properties()
        , m_PropertyReportString()
        , parent(nullptr)
        , broadcaster(nullptr)
    {
    }

    IndividualHuman::~IndividualHuman()
    {
        for (auto infection : infections)
        {
            delete infection;
        }

        delete susceptibility;
        delete interventions;
    }


    QueryResult IndividualHuman::QueryInterface( iid_t iid, void** ppinstance )
    {
        release_assert(ppinstance); // todo: add a real message: "QueryInterface requires a non-NULL destination!");

        ISupports* foundInterface;
        if ( iid == GET_IID(IIndividualHumanEventContext))
            foundInterface = static_cast<IIndividualHumanEventContext*>(this);
        else if ( iid == GET_IID(IIndividualHumanContext))
            foundInterface = static_cast<IIndividualHumanContext*>(this);
        else if ( iid == GET_IID(IIndividualHuman)) 
            foundInterface = static_cast<IIndividualHuman*>(this);
        else if ( iid == GET_IID(IInfectable))
            foundInterface = static_cast<IInfectable*>(this);
        else if ( iid == GET_IID(IInfectionAcquirable))
            foundInterface = static_cast<IInfectionAcquirable*>(this);
        else if ( iid == GET_IID(IMigrate))
            foundInterface = static_cast<IMigrate*>(this);
        else if ( iid == GET_IID(ISupports) )
            foundInterface = static_cast<ISupports*>(static_cast<IIndividualHumanContext*>(this));
        else if (iid == GET_IID(IGlobalContext))
            parent->QueryInterface(iid, reinterpret_cast<void**>(&foundInterface));
        else
            foundInterface = nullptr;

        QueryResult status = e_NOINTERFACE;
        if ( foundInterface )
        {
            foundInterface->AddRef();
            status = s_OK;
        }

        *ppinstance = foundInterface;
        return status;
    }

    IndividualHuman *IndividualHuman::CreateHuman()
    {
        IndividualHuman *newindividual = _new_ IndividualHuman();

        return newindividual;
    }

    IndividualHuman *IndividualHuman::CreateHuman(INodeContext *context, suids::suid id, float MCweight, float init_age, int gender, float init_poverty)
    {
        IndividualHuman *newhuman = _new_ IndividualHuman(id, MCweight, init_age, gender, init_poverty);

        newhuman->SetContextTo(context);
        LOG_DEBUG_F( "Created human with age=%f\n", newhuman->m_age );

        return newhuman;
    }

    void IndividualHuman::InitializeHuman()
    {
        release_assert( parent );
        home_node_id = parent->GetSuid() ;
    }

    bool IndividualHuman::IsAdult() const
    {
        float age_years = GetAge() / DAYSPERYEAR ;
        return age_years >= min_adult_age_years ;
    }

    bool IndividualHuman::IsDead() const
    {
        auto state_change = GetStateChange();
        bool is_dead = (GET_CONFIGURABLE(SimulationConfig)->vital_dynamics &&
                       ( (state_change == HumanStateChange::DiedFromNaturalCauses) || 
                         (state_change == HumanStateChange::KilledByInfection    ) ) ) 
                    || (state_change == HumanStateChange::KilledByMCSampling) ;    //Killed by MC sampling should not rely on vital_dynamics being true.  
        return is_dead ;
    }

    void IndividualHuman::SetContextTo(INodeContext* context)
    {
        INodeContext *old_context = parent;
        parent = context;

        if (parent)
        {
            // clear migration_destination whenever we've migrated to a new node
            // (as opposed to initial simulation initialization, new births, or outbreak import cases)
            if(parent->GetSuid() == migration_destination)
            {
                if(waypoints.size() > 0 && waypoints[0] == migration_destination)
                {
                    waypoints.clear();
                    migration_outbound    = true;
                    migration_will_return = true;
                }

                migration_destination = suids::nil_suid();
            }

            if( (parent->GetSuid() == home_node_id) && is_on_family_trip )
            {
                is_on_family_trip = false ;
            }

            // need to do this *after* (potentially) clearing waypoints above, so that AtHome() can return true
            PropagateContextToDependents();
        }
        else if(old_context)
        {
            if(migration_outbound)
            {
                if(migration_will_return)
                {
                    waypoints.push_back(old_context->GetSuid());
                    waypoints_trip_type.push_back(migration_type);
                }
            }
            else if( waypoints.size() > 0 )
            {
                waypoints.pop_back();
                waypoints_trip_type.pop_back();
            }
        }


        if( parent )
        {
            if (s_OK != parent->GetEventContext()->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), reinterpret_cast<void**>(&broadcaster)))
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()", "INodeTriggeredInterventionConsumer", "INodeEventContext" );
            }
        }
    }

    void IndividualHuman::setupInterventionsContainer()
    {
        // set up a default interventions context
        // derived classes may override this
        interventions = _new_ InterventionsContainer();
    }

    void IndividualHuman::CreateSusceptibility(float immunity_modifier, float risk_modifier)
    {
        susceptibility = Susceptibility::CreateSusceptibility(this, m_age, immunity_modifier, risk_modifier);
    }

    void IndividualHuman::SetParameters(float infsample, float immunity_modifier, float risk_modifier, float migration_modifier)
    {
        StateChange       = HumanStateChange::None;

        // migration stuff
        max_waypoints         = 10;
        migration_outbound    = true;
        migration_will_return = true;

        // set to 0
        is_pregnant     = false;
        pregnancy_timer = 0;

        ClearNewInfectionState();
        susceptibility = nullptr;

        setupInterventionsContainer();
        // The original version caused a lot of issues when querying IIndividualHumanContext functions, initializing it
        // with the correct context works.  Serialization did it ok, which means when serializing and deserializing, it fixed the problem
        // making it harder to originally find
        IIndividualHumanContext *indcontext = GetContextPointer();
        interventions->SetContextTo(indcontext); //TODO: fix this when init pattern standardized <ERAD-291>  PE: See comment above

        Inf_Sample_Rate = infsample; // EAW: currently set to 1 by Node::addNewIndividual
        migration_mod   = migration_modifier;

        // set maximum number of waypoints for this individual--how far will they wander?
        max_waypoints = roundtrip_waypoints;

        CreateSusceptibility(immunity_modifier, risk_modifier);

        // iterate over all IndividualProperty categories in Node, and get one for each
        auto& distribs = parent->GetIndividualPropertyDistributions();
        for (const auto& distribution : distribs)
        {
            const std::string& propertyKey = distribution.first;
            //std::cout << "propertyKey = " << propertyKey << std::endl;
            float rand = randgen->e();
            //auto prop = distribs.find( propertyKey )->second;
            auto& prop = distribution.second;
            for (auto& entry : prop)
            {
                float maxedge = entry.first;
                if( rand < maxedge )
                {
                    Properties[ propertyKey ] = entry.second;
                    if(Environment::getInstance()->Log->CheckLogLevel(Logger::DEBUG, "Individual"))
                    {
                        std::ostringstream msg;
                        msg << "Selected property value " << entry.second << " for key " << propertyKey << " for individual " << GetSuid().data << std::endl;
                        LOG_DEBUG_F( msg.str().c_str() );
                    }
                    break;
                }
            }
        }
    }

    void IndividualHuman::SetInitialInfections(int init_infs)
    {
        if (init_infs)
        {
            for (int i = 0; i < init_infs; i++)
            {
                AcquireNewInfection();
            }
        }
    }

    void IndividualHuman::UpdateMCSamplingRate(float desired_mc_weight)
    {
        if (desired_mc_weight > m_mc_weight)
        {
            LOG_DEBUG_F("ratio = %f\n", m_mc_weight / desired_mc_weight);
            if (randgen->e() < m_mc_weight / desired_mc_weight)
            {
                UpdateGroupPopulation(-1.0f);
                m_mc_weight = desired_mc_weight;
                UpdateGroupPopulation(1.0f);
                LOG_DEBUG_F( "Changed mc_weight to %f for individual %d\n.", m_mc_weight, GetSuid().data );
            }
            else // TODO: It's possible to downsample the immune subpopulation (which could be entire node or hint group population) to 0. Add safeguards to prevent this.
            {
                StateChange = HumanStateChange::KilledByMCSampling;
                LOG_DEBUG_F( "Individual %d will be deleted as part of downsampling.\n", GetSuid().data );
            }
        }
    }

    void IndividualHuman::setupMaternalAntibodies(IIndividualHumanContext* mother, INodeContext* node)
    {
        // derived classes can determine the specific initialization of maternal antibodies related to mother or possible mothers
    }

    void IndividualHuman::PropagateContextToDependents()
    {
        IIndividualHumanContext *context = GetContextPointer();

        // fix up child pointers
        for (auto infection : infections)
        {
            infection->SetContextTo(context);
        }

        if (susceptibility) susceptibility->SetContextTo(context);
        if (interventions)  interventions->SetContextTo(context);
    }

    //------------------------------------------------------------------
    //   Update methods
    //------------------------------------------------------------------

    void IndividualHuman::Update(float currenttime, float dt)
    {
        LOG_VALID_F( "%s\n", __FUNCTION__ );
        float infection_timestep = dt;
        int numsteps = 1;

        // eventually need to correct for time step in case of individuals moving among communities with different adapted time steps

        StateChange = HumanStateChange::None;

        //  Aging
        if (aging)
        {
            m_age += dt;
        }

        // Adjust time step for infections as specified by infection_updates_per_tstep.  A value of 0 reverts to a single update per time step for backward compatibility.
        // There is no special meaning of 1 being hourly.  For hourly infection updates with a tstep of one day, one must now specify 24.
        if ( infection_updates_per_tstep > 1 )
        {
            // infection_updates_per_tstep is now an integer > 1, so set numsteps equal to it,
            // allowing the subdivision dt into smaller infection_timestep
            numsteps = infection_updates_per_tstep;
            infection_timestep = dt / numsteps;
        }

        // Process list of infections
        if (infections.size() == 0) // don't need to process infections or go hour by hour
        {
            susceptibility->Update(dt);
            interventions->Update(dt);
        }
        else
        {
            for (int i = 0; i < numsteps; i++)
            {
                for (auto it = infections.begin(); it != infections.end();)
                {
                    // Update infection
                    (*it)->Update(infection_timestep, susceptibility);

                    // Check for a new infection/human state (e.g. Fatal, Cleared)
                    InfectionStateChange::_enum inf_state_change = (*it)->GetStateChange();
                    if (inf_state_change != InfectionStateChange::None)
                    {
                        SetNewInfectionState(inf_state_change);

                        // Notify susceptibility of cleared infection and remove from list
                        if ( inf_state_change == InfectionStateChange::Cleared )
                        {
                            LOG_DEBUG_F( "Individual %d's infection cleared.\n", GetSuid().data );
                            // --------------------------------------------
                            // --- the following makes sense but it changes
                            // --- results unexpectedly.
                            // --------------------------------------------
                            //ClearNewInfectionState(); // Seems to break things
                            // --------------------------------------------
                            if (immunity)
                            {
                                susceptibility->UpdateInfectionCleared();
                            } //Immunity update: survived infection
                            delete *it;
                            it = infections.erase(it);
                            continue;
                        }

                        // Set human state change and stop updating infections if the person has died
                        if ( inf_state_change == InfectionStateChange::Fatal )
                        {
                            Die( HumanStateChange::KilledByInfection );
                            break;
                        }
                    }

                    ++it;
                }

                if (immunity)
                {
                    susceptibility->Update(infection_timestep);      // Immunity update: mainly decay of immunity
                }
                if (StateChange == HumanStateChange::KilledByInfection)
                {
                    break; // If individual died, no need to keep simulating infections.
                }
                interventions->Update(infection_timestep);
            }
        }

        applyNewInterventionEffects(dt);

        // Trigger "every-update" event observers
        broadcaster->TriggerNodeEventObservers(GetEventContext(), IndividualEventTriggerType::EveryUpdate);

        //  Get new infections
        ExposeToInfectivity(dt, &transmissionGroupMembership); // Need to do it even if infectivity==0, because of diseases in which immunity of acquisition depends on challenge (eg malaria)

        //  Is there an active infection for statistical purposes?
        m_is_infected = (infections.size() > 0);

        if (StateChange == HumanStateChange::None && GET_CONFIGURABLE(SimulationConfig)->vital_dynamics) // Individual can't die if they're already dead
            CheckVitalDynamics(currenttime, dt);

        if (StateChange == HumanStateChange::None && GET_CONFIGURABLE(SimulationConfig)->migration_structure) // Individual can't migrate if they're already dead
            CheckForMigration(currenttime, dt);
    }

    void IndividualHuman::CheckVitalDynamics(float currenttime, float dt)
    {
        VitalDeathDependence::Enum vital_death_dependence = GET_CONFIGURABLE(SimulationConfig)->vital_death_dependence;
        if (  vital_death_dependence == VitalDeathDependence::NONDISEASE_MORTALITY_BY_AGE_AND_GENDER ||
                vital_death_dependence == VitalDeathDependence::NONDISEASE_MORTALITY_BY_YEAR_AND_AGE_FOR_EACH_GENDER )
        {
            // DJK TODO: Compute natural death at initiation and use timer <ERAD-1857>
            // for performance, cache and recalculate mortality rate only every month
            if( m_daily_mortality_rate == 0 || fmodf(m_age, IDEALDAYSPERMONTH) < dt )
            {
                if( vital_death_dependence == VitalDeathDependence::NONDISEASE_MORTALITY_BY_AGE_AND_GENDER ) {
                    // "MortalityDistribution" is added to map in Node::SetParameters if Death_Rate_Dependence is NONDISEASE_MORTALITY_BY_AGE_AND_GENDER
                    m_daily_mortality_rate = GetDemographicsDistribution(NodeDemographicsDistribution::MortalityDistribution)->DrawResultValue(GetGender() == Gender::FEMALE, GetAge());
                }
                else
                {
                    float year = parent->GetTime().Year();

                    if( GetGender() == Gender::MALE )
                        m_daily_mortality_rate = GetDemographicsDistribution(NodeDemographicsDistribution::MortalityDistributionMale)->DrawResultValue(GetAge(), year);
                    else
                        m_daily_mortality_rate = GetDemographicsDistribution(NodeDemographicsDistribution::MortalityDistributionFemale)->DrawResultValue(GetAge(), year);
                }
            }

            if(randgen->e() < x_othermortality * m_daily_mortality_rate * dt)
            {
                LOG_DEBUG_F("%s died of natural causes at age %f with daily_mortality_rate = %f\n", (GetGender() == Gender::FEMALE ? "Female" : "Male"), GetAge() / DAYSPERYEAR, m_daily_mortality_rate);
                Die( HumanStateChange::DiedFromNaturalCauses );
            }
        }
    }

    void IndividualHuman::applyNewInterventionEffects(float dt)
    {
    }

    float IndividualHuman::GetImmunityReducedAcquire() const
    {
        return susceptibility->getModAcquire();
    }

    float IndividualHuman::GetInterventionReducedAcquire() const
    {
        return interventions->GetInterventionReducedAcquire();
    }

    bool IndividualHuman::UpdatePregnancy(float dt)
    {
        bool birth_this_timestep = false;
        if (is_pregnant)
        {
            pregnancy_timer -= dt;
            if (pregnancy_timer <= 0)
            {
                is_pregnant         = false;
                pregnancy_timer     = 0;
                birth_this_timestep = true;

                // Broadcast GaveBirth
                INodeTriggeredInterventionConsumer* broadcaster = nullptr;
                if (GetNodeEventContext()->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), reinterpret_cast<void**>(&broadcaster)) != s_OK)
                {
                    throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()", "INodeTriggeredInterventionConsumer", "IIIndividualHumanEventContext" );
                }
                broadcaster->TriggerNodeEventObservers( GetEventContext(), IndividualEventTriggerType::GaveBirth );
            }
        }

        return birth_this_timestep;
    }

    void IndividualHuman::InitiatePregnancy(float duration)
    {
        if (!is_pregnant)
        {
            is_pregnant = true;
            pregnancy_timer = duration; 

            // Broadcast Pregnant
            INodeTriggeredInterventionConsumer* broadcaster = nullptr;
            if (GetNodeEventContext()->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), reinterpret_cast<void**>(&broadcaster)) != s_OK)
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()", "INodeTriggeredInterventionConsumer", "IIIndividualHumanEventContext" );
            }
            broadcaster->TriggerNodeEventObservers( GetEventContext(), IndividualEventTriggerType::Pregnant );
        }
    }

    //------------------------------------------------------------------
    //   Migration methods
    //------------------------------------------------------------------

    void IndividualHuman::ImmigrateTo(INodeContext* destination_node)
    {
        if( destination_node == nullptr )
        {
            throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "destination_node" );
        }

        destination_node->processImmigratingIndividual(this);
        if( migration_is_destination_new_home )
        {
            home_node_id = destination_node->GetSuid();
            migration_is_destination_new_home = false;
            migration_outbound = false ;
            migration_will_return = false ;
            waypoints.clear();
            waypoints_trip_type.clear();
        }
    }

    void IndividualHuman::CheckForMigration(float currenttime, float dt)
    {
        //  Determine if individual moves during this time step

        switch (GET_CONFIGURABLE(SimulationConfig)->migration_structure)
        {
        case MigrationStructure::FIXED_RATE_MIGRATION:
            if( leave_on_family_trip )
            {
                migration_outbound                = true;
                migration_will_return             = true;
                migration_destination             = family_migration_destination;
                migration_type                    = family_migration_type;
                migration_time_until_trip         = family_migration_time_until_trip;
                migration_time_at_destination     = family_migration_time_at_destination;
                migration_is_destination_new_home = family_migration_is_destination_new_home;
                is_on_family_trip                 = true;

                leave_on_family_trip             = false;
                family_migration_destination     = suids::nil_suid();
                family_migration_type            = MigrationType::NO_MIGRATION;
                family_migration_time_until_trip = 0.0 ;
            }
            else if( !waiting_for_family_trip )
            {
                if( migration_destination.is_nil() )
                    SetNextMigration();
            }

            if( !migration_destination.is_nil() )
            {
                migration_time_until_trip -= dt;

                // --------------------------------------------------------------------
                // --- This check should really be zero, but epsilon makes the test for
                // --- this pass as expected.  Namely, it helps things work more like
                // --- you'd expect with an intervention where the times might be round numbers.
                // --------------------------------------------------------------------
                if( migration_time_until_trip <= 0.0000001f )
                {
                    LOG_DEBUG_F( "%s: individual %d is migrating.\n", __FUNCTION__, suid.data );
                    StateChange = HumanStateChange::Migrating;
                }
            }
            break;

        case MigrationStructure::VARIABLE_RATE_MIGRATION:   // Variable, but drawn from distributions
            throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "MigrationStructure::VARIABLE_RATE_MIGRATION" );

        case MigrationStructure::LEVY_FLIGHTS: //Levy flights
            //  need community to have specific location with easy way to calculate distance
            //  can calculate a matrix among communities based on distances
            //  would be easiest to implement for individual-based
            throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "MigrationStructure::LEVY_FLIGHTS" );

        case MigrationStructure::NO_MIGRATION:
        default:
            std::stringstream msg;
            msg << "Invalid migration_structure=" << GET_CONFIGURABLE(SimulationConfig)->migration_structure;
            throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            break;
        }
    }

    void IndividualHuman::SetNextMigration(void)
    {
        IMigrationInfo *migration_info = parent->GetMigrationInfo();

        // ----------------------------------------------------------------------------------------
        // --- We don't want the check for reachable nodes here because one could travel to a node
        // --- that doesn't let its residents travel and we need the ability to get back.
        // --- That is, I should be able to travel to a node and return from it, even if the
        // --- residents of the node do not migrate.
        // ----------------------------------------------------------------------------------------
        if( GET_CONFIGURABLE(SimulationConfig)->migration_structure != MigrationStructure::NO_MIGRATION )
        {
            if(waypoints.size() == 0)
                migration_outbound = true;
            else if(waypoints.size() == max_waypoints)
                migration_outbound = false;

            if( migration_outbound && (migration_info->GetReachableNodes().size() > 0) )
            {
                migration_info->PickMigrationStep( this, migration_mod, migration_destination, migration_type, migration_time_until_trip );

                if( migration_type == MigrationType::NO_MIGRATION )
                {
                    return ;
                }
                else if( migration_type == MigrationType::FAMILY_MIGRATION )
                {
                    waiting_for_family_trip = true ;

                    float time_at_destination = GetRoundTripDurationRate( migration_type );
                    parent->SetWaitingForFamilyTrip( migration_destination, 
                                                     migration_type,
                                                     migration_time_until_trip,
                                                     time_at_destination,
                                                     false );

                    migration_destination = suids::nil_suid();
                    migration_type = MigrationType::NO_MIGRATION;
                    migration_time_until_trip = 0.0 ;
                    migration_will_return = true; // family trips must return
                }
                else
                {
                    float return_prob = 0.0f;
                    switch(migration_type)
                    {
                        case MigrationType::LOCAL_MIGRATION:    return_prob = local_roundtrip_prob;  break;
                        case MigrationType::AIR_MIGRATION:      return_prob = air_roundtrip_prob;    break;
                        case MigrationType::REGIONAL_MIGRATION: return_prob = region_roundtrip_prob; break;
                        case MigrationType::SEA_MIGRATION:      return_prob = sea_roundtrip_prob;    break;
                        case MigrationType::FAMILY_MIGRATION:   return_prob = family_roundtrip_prob; break;
                        case MigrationType::INTERVENTION_MIGRATION:
                        default:
                            throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "migration_type", migration_type, "MigrationType" );
                    }

                    migration_will_return = (return_prob > 0.0f);
                    if( migration_will_return && (return_prob < 1.0f) )
                    {
                        if(randgen->e() > return_prob)
                        {
                            migration_will_return = false;
                        }
                    }
                }
            }
            else if( waypoints.size() > 0 )
            {
                migration_destination = waypoints.back();
                MigrationType::Enum trip_type = waypoints_trip_type.back();


                if( migration_time_at_destination > 0.0f )
                {
                    migration_time_until_trip = migration_time_at_destination ;
                    migration_time_at_destination = 0.0f ;
                }
                else
                {
                    migration_time_until_trip = GetRoundTripDurationRate( trip_type );
                }
            }
        }
    }

    float IndividualHuman::GetRoundTripDurationRate( MigrationType::Enum trip_type )
    {
        float return_duration_rate = 0.0f;
        switch(trip_type)
        {
            case MigrationType::LOCAL_MIGRATION:    return_duration_rate = local_roundtrip_duration_rate;  break;
            case MigrationType::AIR_MIGRATION:      return_duration_rate = air_roundtrip_duration_rate;    break;
            case MigrationType::REGIONAL_MIGRATION: return_duration_rate = region_roundtrip_duration_rate; break;
            case MigrationType::SEA_MIGRATION:      return_duration_rate = sea_roundtrip_duration_rate;    break;
            case MigrationType::FAMILY_MIGRATION:   return_duration_rate = family_roundtrip_duration_rate; break;
            case MigrationType::INTERVENTION_MIGRATION:
            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "trip_type", trip_type, "MigrationType" );
        }

        float duration = 0.0;
        if(return_duration_rate > 0.0f)
        {
            duration = float(randgen->expdist( return_duration_rate ));
        }
        return duration;
    }

    const suids::suid& IndividualHuman::GetMigrationDestination()
    {
        return migration_destination;
    }

    bool IndividualHuman::IsMigrating()
    {
        return StateChange == HumanStateChange::Migrating;
    }

    void IndividualHuman::UpdateGroupMembership()
    {
        tProperties* properties = GetProperties();
        const RouteList_t& routes = parent->GetTransmissionRoutes();
        LOG_DEBUG_F("Updating transmission group membership for individual %d for %d routes (first route is %s).\n", this->GetSuid().data, routes.size(), routes[0].c_str());

        for (auto& route : routes)
        {
            LOG_DEBUG_F("Updating for Route %s.\n", route.c_str());
            RouteList_t single_route;
            single_route.push_back(route);
            parent->GetGroupMembershipForIndividual(single_route, properties, &transmissionGroupMembershipByRoute[route]);
        }
        parent->GetGroupMembershipForIndividual(routes, properties, &transmissionGroupMembership);  // DJK: Why this and the one per-route above?
    }

    void IndividualHuman::UpdateGroupPopulation(float size_changes)
    {
        parent->UpdateTransmissionGroupPopulation(&transmissionGroupMembership, size_changes, this->GetMonteCarloWeight());
    }

    bool IndividualHuman::AtHome() const
    {
        return home_node_id == parent->GetSuid();
    }

    void IndividualHuman::GoHome()
    {
        migration_destination = home_node_id ;
    }

    void IndividualHuman::SetGoingOnFamilyTrip( suids::suid migrationDestination,
                                                MigrationType::Enum migrationType,
                                                float timeUntilTrip,
                                                float timeAtDestination,
                                                bool isDestinationNewHome )
    {
        leave_on_family_trip                     = true;
        family_migration_destination             = migrationDestination;
        family_migration_type                    = migrationType;
        family_migration_time_until_trip         = timeUntilTrip;
        family_migration_time_at_destination     = timeAtDestination;
        family_migration_is_destination_new_home = isDestinationNewHome;
        waiting_for_family_trip                  = false;
    }

    void IndividualHuman::SetWaitingToGoOnFamilyTrip()
    {
        waiting_for_family_trip   = true ;
        migration_destination     = suids::nil_suid();
        migration_time_until_trip = 0.0 ;
    }

    void IndividualHuman::SetMigrating( suids::suid destination, 
                                        MigrationType::Enum type, 
                                        float timeUntilTrip, 
                                        float timeAtDestination,
                                        bool isDestinationNewHome )
    {
        if( parent->GetSuid().data != destination.data )
        {
            migration_destination             = destination;
            migration_type                    = type;
            migration_time_until_trip         = timeUntilTrip;
            migration_time_at_destination     = timeAtDestination;
            migration_is_destination_new_home = isDestinationNewHome;
            migration_outbound                = !isDestinationNewHome;
            migration_will_return             = !isDestinationNewHome;
        }
    }

    //------------------------------------------------------------------
    //   Infection methods
    //------------------------------------------------------------------

    void IndividualHuman::ExposeToInfectivity(float dt, const TransmissionGroupMembership_t* transmissionGroupMembership)
    {
        parent->ExposeIndividual(static_cast<IInfectable*>(this), transmissionGroupMembership, dt);
    }

    // TODO: port normal exposure_to_infectivity logic to this pattern as well <ERAD-328>
    void IndividualHuman::Expose(const IContagionPopulation* cp, float dt, TransmissionRoute::Enum transmission_route)
    {
        StrainIdentity strainId;
        strainId.SetAntigenID(cp->GetAntigenId()); // find antigenID of the strain to get infectivity from, is the individual already infected by the contagion of this antigen type?

        if (!InfectionExistsForThisStrain(&strainId)) // no existing infection of this antigenic type, so determine infection from exposure
        {
            LOG_DEBUG_F( "id = %lu, group_id = %d, total contagion = %f, dt = %f, immunity factor = %f, interventions factor = %f\n",
                         GetSuid().data, transmissionGroupMembership.at(0), cp->GetTotalContagion(), dt, susceptibility->getModAcquire(), interventions->GetInterventionReducedAcquire() );
            if (randgen->e() < EXPCDF(-cp->GetTotalContagion()*dt*susceptibility->getModAcquire()*interventions->GetInterventionReducedAcquire())) // infection results from this strain?
            {
                /*std::ostringstream msg;
                msg << "factor for cdf = " << cp->GetTotalContagion()*dt*susceptibility->getModAcquire()*interventions->GetInterventionReducedAcquire() << std::endl;
                LOG_DEBUG_F( "%s\n", msg.str().c_str() );*/
                cp->ResolveInfectingStrain(&strainId); // get the substrain ID
                AcquireNewInfection(&strainId);
            }
            else
            {
                // immune response without infection
            }
        }
        else
        {
            /*LOG_DEBUG_F( "id = %lu, group_id = %d, total contagion = %f, dt = %f, immunity factor = %f, interventions factor = %f\n",
                         GetSuid().data, transmissionGroupMembership->at(0), cp->GetTotalContagion(), dt, susceptibility->getModAcquire(), interventions->GetInterventionReducedAcquire() );*/
            // multiple infections of the same type happen here
            if (randgen->e() < EXPCDF(-cp->GetTotalContagion()*dt * susceptibility->getModAcquire()*interventions->GetInterventionReducedAcquire())) // infection results from this strain?
            {
                /*std::ostringstream msg;
                msg << "factor for cdf = " << cp->GetTotalContagion()*dt*susceptibility->getModAcquire()*interventions->GetInterventionReducedAcquire() << std::endl;
                LOG_DEBUG_F( "%s\n", msg.str().c_str() );*/
                cp->ResolveInfectingStrain(&strainId);
                AcquireNewInfection(&strainId); // superinfection of this antigenic type
            }
            else
            {
                // immune response without super infection
            }
        }
    }

    void IndividualHuman::AcquireNewInfection(StrainIdentity *infstrain, int incubation_period_override )
    {
        //LOG_DEBUG_F( "AcquireNewInfection: id=%lu, group_id=%d\n", GetSuid().data, ( transmissionGroupMembership.size() ? transmissionGroupMembership.at(0) : nullptr ) );
        int numInfs = int(infections.size());
        if ((superinfection && (numInfs < max_ind_inf)) || numInfs == 0)
        {
            cumulativeInfs++;
            m_is_infected = true;

            IInfection *newinf = createInfection( parent->GetNextInfectionSuid() );
            newinf->SetParameters(infstrain, incubation_period_override);
            newinf->InitInfectionImmunology(susceptibility);

            LOG_DEBUG( "Adding infection to infections list.\n" );
            infections.push_front(newinf);
            infectiousness += newinf->GetInfectiousness();
            ReportInfectionState(); // can specify different reporting in derived classes
#if 0
            // Trigger new infection event observers
            IIndividualTriggeredInterventionConsumer * pITIC = nullptr;
            if (s_OK == GetInterventionsContext()->QueryInterface(GET_IID(IIndividualTriggeredInterventionConsumer), (void**)&pITIC) )
            {
                pITIC->TriggerIndividualEventObservers( GetEventContext(), IndividualEventTriggerType::NewInfectionEvent );
            }
#endif
        }
    }

    void IndividualHuman::UpdateInfectiousness(float dt)
    {
        infectiousness = 0;

        if ( infections.size() == 0 )
            return;

        for (auto infection : infections)
        {
            infectiousness += infection->GetInfectiousness();
            float tmp_infectiousness =  m_mc_weight * infection->GetInfectiousness() * susceptibility->GetModTransmit() * interventions->GetInterventionReducedTransmit();
            StrainIdentity tmp_strainIDs;
            infection->GetInfectiousStrainID(&tmp_strainIDs);
            if( tmp_infectiousness )
            {
                LOG_DEBUG_F( "Individual %d depositing contagion into transmission group.\n", GetSuid().data );
                parent->DepositFromIndividual(&tmp_strainIDs, tmp_infectiousness, &transmissionGroupMembership);
            }
        }

        infectiousness *= susceptibility->GetModTransmit() * interventions->GetInterventionReducedTransmit();
    }

    bool IndividualHuman::InfectionExistsForThisStrain(StrainIdentity* check_strain_id)
    {
        for (auto infection : infections)
        {
            StrainIdentity infection_strain;
            infection->GetInfectiousStrainID(&infection_strain);
            if (infection_strain.GetAntigenID() == check_strain_id->GetAntigenID())
                return true;
        }

        return false;
    }

    IInfection* IndividualHuman::createInfection(suids::suid _suid)
    {
        return Infection::CreateInfection(this, _suid);
    }

    bool IndividualHuman::SetNewInfectionState(InfectionStateChange::_enum inf_state_change)
    {
        // Derived disease classes can report their own NewInfection states
        return false;
    }

    void IndividualHuman::ReportInfectionState()
    {
        LOG_DEBUG( "ReportInfectionState\n" );
        // Is infection reported this turn?
        // Will only implement delayed reporting (for fever response) later
        // 50% reporting immediately
        if (randgen->e() < .5)
            m_new_infection_state = NewInfectionState::NewAndDetected;
        else
        {
            release_assert(parent);
            //parent->notifyOnNewInfection( this );
            m_new_infection_state = NewInfectionState::NewInfection;
        }
    }

    void IndividualHuman::ClearNewInfectionState()
    {
        m_new_infection_state = NewInfectionState::Invalid;
    }

    //------------------------------------------------------------------
    //   Assorted getters and setters
    //------------------------------------------------------------------

    // IIndividualHumanContext methods
    suids::suid IndividualHuman::GetSuid() const         { return suid; }
    suids::suid IndividualHuman::GetNextInfectionSuid()  { return parent->GetNextInfectionSuid(); }
    ::RANDOMBASE* IndividualHuman::GetRng()              { return parent->GetRng(); }

    const NodeDemographics* IndividualHuman::GetDemographics()           const { return parent->GetDemographics(); }
    const NodeDemographicsDistribution* IndividualHuman::GetDemographicsDistribution(std::string key) const { return parent->GetDemographicsDistribution(key); }

    IIndividualHumanInterventionsContext* IndividualHuman::GetInterventionsContext()  const { return static_cast<IIndividualHumanInterventionsContext*>(interventions); }
    IIndividualHumanInterventionsContext* IndividualHuman::GetInterventionsContextbyInfection(Infection* infection) { 
        return static_cast<IIndividualHumanInterventionsContext*>(interventions); 
        }; //Note can also throw exception here since it's not using the infection to find the intervention
    IIndividualHumanEventContext*         IndividualHuman::GetEventContext()                { return static_cast<IIndividualHumanEventContext*>(this); }
    ISusceptibilityContext*               IndividualHuman::GetSusceptibilityContext() const { return static_cast<ISusceptibilityContext*>(susceptibility); }

    bool IndividualHuman::IsPossibleMother() const
    {
        if (m_age > (14 * DAYSPERYEAR) && m_age < (45 * DAYSPERYEAR) && m_gender == Gender::FEMALE)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }

    void IndividualHuman::Die( HumanStateChange newState )
    {
        StateChange = newState;
        INodeTriggeredInterventionConsumer* broadcaster = nullptr;
        if (GetNodeEventContext()->QueryInterface(GET_IID(INodeTriggeredInterventionConsumer), reinterpret_cast<void**>(&broadcaster)) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()", "INodeTriggeredInterventionConsumer", "IIIndividualHumanEventContext" );
        }

        StateChange = newState;
        switch (newState)
        {
            case HumanStateChange::DiedFromNaturalCauses:
            {
                LOG_DEBUG_F( "%s: individual %d (%s) died of natural causes at age %f with daily_mortality_rate = %f\n", __FUNCTION__, suid.data, (GetGender() == Gender::FEMALE ? "Female" : "Male"), GetAge() / DAYSPERYEAR, m_daily_mortality_rate );
                broadcaster->TriggerNodeEventObservers( GetEventContext(), IndividualEventTriggerType::NonDiseaseDeaths );
            }
            break;

            case HumanStateChange::KilledByInfection:
            {
                LOG_DEBUG_F( "%s: individual %d died from infection\n", __FUNCTION__, suid.data );
                broadcaster->TriggerNodeEventObservers( GetEventContext(), IndividualEventTriggerType::DiseaseDeaths );
            }
            break;

            default:
            release_assert( false );
            break;
        }
    }

    float IndividualHuman::GetAcquisitionImmunity() const
    {
        return susceptibility->getModAcquire()*interventions->GetInterventionReducedAcquire();
    }

    INodeEventContext * IndividualHuman::GetNodeEventContext()
    {
        release_assert( GetParent() );
        return GetParent()->GetEventContext();
    }

    IIndividualHumanContext* IndividualHuman::GetContextPointer() { return this; }

    INodeContext* IndividualHuman::GetParent() const
    {
        return parent;
    }

    tProperties* IndividualHuman::GetProperties()
    {
        return &Properties;
    }

    const infection_list_t&
    IndividualHuman::GetInfections() const
    {
        return infections;
    }

    ProbabilityNumber
    IndividualHuman::getProbMaternalTransmission()
    const
    {
        return GET_CONFIGURABLE(SimulationConfig)->prob_maternal_transmission;
    }

/* clorton
    std::string IndividualHuman::toJson()
    {
        IJsonObjectAdapter* adapter = CreateJsonObjAdapter();
        adapter->CreateNewWriter();
        adapter->BeginObject();

        adapter->Insert("suid", suid.data);
        adapter->Insert("age", m_age);
        adapter->Insert("gender", m_gender);
        adapter->Insert("mc_weight", m_mc_weight);
        adapter->Insert("daily_mortality_rate", m_daily_mortality_rate);
        adapter->Insert("above_poverty", above_poverty);
        adapter->Insert("is_pregnant", is_pregnant);
        adapter->Insert("pregnancy_timer", pregnancy_timer);

        // susceptibility
        // infections
        // interventions
        // transmissionGroupMembership
        // transmissionGroupMembershipByRoute

        adapter->Insert("is_infected", m_is_infected);
        adapter->Insert("infectiousness", infectiousness);
        adapter->Insert("Inf_Sample_Rate", Inf_Sample_Rate);
        adapter->Insert("cumulativeInfs", cumulativeInfs);

        adapter->Insert("new_infection_state", int(m_new_infection_state));
        adapter->Insert("StateChange", int(StateChange));

        adapter->Insert("migration_mod", migration_mod);
        adapter->Insert("migration_type", migration_type);
        adapter->Insert("migration_destination", migration_destination.data);
        adapter->Insert("time_to_next_migration", time_to_next_migration);
        adapter->Insert("will_return", will_return);
        adapter->Insert("outbound", outbound);
        adapter->Insert("max_waypoints", max_waypoints);
        // waypoints
        // waypoints_trip_type

        // Properties

        // parent

        adapter->EndObject();

        std::string s(adapter->ToString());
        return s;
    }

    void IndividualHuman::fromJson(std::string& json)
    {
        IJsonObjectAdapter* adapter = CreateJsonObjAdapter();
        adapter->Parse(json.c_str());

        suid.data = adapter->GetUint("suid");
        m_age = adapter->GetFloat("age");
        m_gender = adapter->GetInt("gender");
        m_mc_weight = adapter->GetFloat("mc_weight");
        m_daily_mortality_rate = adapter->GetFloat("daily_mortality_rate");
        is_pregnant = adapter->GetBool("is_pregnant");
        pregnancy_timer = adapter->GetBool("pregnancy_timer");

        // susceptibility
        // infections
        // interventions
        // transmissionGroupMembership
        // transmissionGroupMembershipByRoute

        m_is_infected = adapter->GetBool("is_infected");
        infectiousness = adapter->GetFloat("infectiousness");
        Inf_Sample_Rate = adapter->GetFloat("Inf_Sample_Rate");
        cumulativeInfs = adapter->GetInt("cumulativeInfs");

        m_new_infection_state = NewInfectionState::_enum(adapter->GetInt("new_infection_state"));
        StateChange = HumanStateChange(adapter->GetInt("StateChange"));

        migration_mod = adapter->GetFloat("migration_mod");
        migration_type = adapter->GetInt("migration_type");
        migration_destination.data = adapter->GetUint("migration_destination");
        time_to_next_migration = adapter->GetFloat("time_to_next_migration");
        will_return = adapter->GetBool("will_return");
        outbound = adapter->GetBool("outbound");
        max_waypoints = adapter->GetInt("max_waypoints");
        // waypoints
        // waypoints_trip_type

        // Properties

        // parent

        delete adapter;
    }
*/
    
    void serialize_waypoint_types( IArchive& ar, std::vector<MigrationType::Enum>& waypointTripTypes )
    {
        size_t count = ar.IsWriter() ? waypointTripTypes.size() : -1;

        ar.startArray(count);
        if (ar.IsWriter())
        {
            for (auto& entry : waypointTripTypes)
            {
                ar & (uint32_t&)entry;
            }
        }
        else
        {
            for (size_t i = 0; i < count; ++i)
            {
                MigrationType::Enum value;
                ar & (uint32_t&)value;
                waypointTripTypes.push_back( value );
            }
        }
        ar.endArray();
    }

    void IndividualHuman::serialize(IArchive& ar, IndividualHuman* obj)
    {
        IndividualHuman& individual = *obj;
        ar.labelElement("suid_data") & individual.suid.data;
        ar.labelElement("m_age") & individual.m_age;
        ar.labelElement("m_gender") & individual.m_gender;
        ar.labelElement("m_mc_weight") & individual.m_mc_weight;
        ar.labelElement("m_daily_mortality_rate") & individual.m_daily_mortality_rate;
        ar.labelElement("above_poverty") & individual.above_poverty;
        ar.labelElement("is_pregnant") & individual.is_pregnant;
        ar.labelElement("pregnancy_timer") & individual.pregnancy_timer;
        ar.labelElement("susceptibility") & individual.susceptibility;
        ar.labelElement("infections") & individual.infections;
        ar.labelElement("interventions") & individual.interventions;
        // don't serialize transmissionGroupMembership, it will be reconstituted on deserialization
        // don't serialize transmissionGroupMembershipByRoute, it will be reconstituted on deserialization
        ar.labelElement("m_is_infected") & individual.m_is_infected;
        ar.labelElement("infectiousness") & individual.infectiousness;
        ar.labelElement("Inf_Sample_Rate") & individual.Inf_Sample_Rate;
        ar.labelElement("cumulativeInfs") & individual.cumulativeInfs;
        ar.labelElement("m_new_infection_state") & (uint32_t&)individual.m_new_infection_state;
        ar.labelElement("StateChange") & (uint32_t&)individual.StateChange;
        ar.labelElement("migration_mod") & individual.migration_mod;
        ar.labelElement("migration_type") & (uint32_t&)individual.migration_type;
        ar.labelElement("migration_destination_data") & individual.migration_destination.data;
        ar.labelElement("migration_time_until_trip") & individual.migration_time_until_trip;
        ar.labelElement("migration_time_at_destination") & individual.migration_time_at_destination;
        ar.labelElement("migration_is_destination_new_home") & individual.migration_is_destination_new_home;
        ar.labelElement("migration_will_return") & individual.migration_will_return;
        ar.labelElement("migration_outbound") & individual.migration_outbound;
        ar.labelElement("max_waypoints") & individual.max_waypoints;
        ar.labelElement("waypoints") & individual.waypoints;
        ar.labelElement("waypoints_trip_type"); serialize_waypoint_types( ar, individual.waypoints_trip_type );
        ar.labelElement("home_node_id") & individual.home_node_id.data;
        ar.labelElement("Properties") & individual.Properties;
        ar.labelElement("waiting_for_family_trip") & individual.waiting_for_family_trip;
        ar.labelElement("leave_on_family_trip") & individual.leave_on_family_trip;
        ar.labelElement("is_on_family_trip") & individual.is_on_family_trip;
        ar.labelElement("family_migration_type") & (uint32_t&)individual.family_migration_type;
        ar.labelElement("family_migration_time_until_trip") & individual.family_migration_time_until_trip;
        ar.labelElement("family_migration_time_at_destination") & individual.family_migration_time_at_destination;
        ar.labelElement("family_migration_is_destination_new_home") & individual.family_migration_is_destination_new_home;
        ar.labelElement("family_migration_destination") & individual.family_migration_destination.data;
    }

    REGISTER_SERIALIZABLE(IndividualHuman);
}
