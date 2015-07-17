/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "Environment.h"
#include "Debug.h"
#include "Infection.h"
#include "InterventionsContainer.h"
#include "Susceptibility.h"
#include "RANDOM.h"
#include "SimulationConfig.h"
#include "MathFunctions.h"

#include "Serializer.h"

#include "RapidJsonImpl.h"

static const char* _module = "Infection";

namespace Kernel
{
    // static initializers for config base class
    MortalityTimeCourse::Enum  InfectionConfig::mortality_time_course   =  MortalityTimeCourse::DAILY_MORTALITY;
    DistributionFunction::Enum InfectionConfig::incubation_distribution = DistributionFunction::FIXED_DURATION;
    DistributionFunction::Enum InfectionConfig::infectious_distribution = DistributionFunction::FIXED_DURATION;
    float InfectionConfig::incubation_period = 1.0f;
    float InfectionConfig::incubation_period_mean = 1.0f;
    float InfectionConfig::incubation_period_std_dev = 1.0f;
    float InfectionConfig::incubation_period_min = 1.0f;
    float InfectionConfig::incubation_period_max = 1.0f;
    float InfectionConfig::infectious_period = 1.0f;
    float InfectionConfig::infectious_period_mean = 1.0f;
    float InfectionConfig::infectious_period_std_dev = 1.0f;
    float InfectionConfig::infectious_period_min = 1.0f;
    float InfectionConfig::infectious_period_max = 1.0f;
    float InfectionConfig::base_infectivity = 1.0f;
    float InfectionConfig::base_mortality = 1.0f;

    GET_SCHEMA_STATIC_WRAPPER_IMPL(Infection,InfectionConfig)
    BEGIN_QUERY_INTERFACE_BODY(InfectionConfig)
    END_QUERY_INTERFACE_BODY(InfectionConfig)

    bool 
    InfectionConfig::Configure(
        const Configuration* config
    )
    {
        initConfig( "Mortality_Time_Course", mortality_time_course, config, MetadataDescriptor::Enum("mortality_time_course", Mortality_Time_Course_DESC_TEXT, MDD_ENUM_ARGS(MortalityTimeCourse)) ); // infection only (move)
        initConfig( "Incubation_Period_Distribution", incubation_distribution, config, MetadataDescriptor::Enum("incubation_distribution", Incubation_Period_Distribution_DESC_TEXT, MDD_ENUM_ARGS(DistributionFunction)) ); // infection-only (move)
        LOG_DEBUG_F( "incubation_distribution = %s\n", DistributionFunction::pairs::lookup_key(incubation_distribution) );
        if( incubation_distribution == DistributionFunction::FIXED_DURATION || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Base_Incubation_Period", &incubation_period, Base_Incubation_Period_DESC_TEXT, 0.0f, FLT_MAX, 6.0f ); // should default change depending on disease?
        }

        if( incubation_distribution == DistributionFunction::UNIFORM_DURATION || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Incubation_Period_Min", &incubation_period_min, Incubation_Period_Min_DESC_TEXT, 0.0f, FLT_MAX, 6.0f, "Incubation_Period_Distribution", "UNIFORM_DISTRIBUTION" );
            initConfigTypeMap( "Incubation_Period_Max", &incubation_period_max, Incubation_Period_Max_DESC_TEXT, 0.0f, FLT_MAX, 6.0f, "Incubation_Period_Distribution", "UNIFORM_DISTRIBUTION" );
        }

        if( incubation_distribution == DistributionFunction::EXPONENTIAL_DURATION || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Base_Incubation_Period", &incubation_period, Base_Incubation_Period_DESC_TEXT, 0.0f, FLT_MAX, 6.0f );
        }

        if( incubation_distribution == DistributionFunction::GAUSSIAN_DURATION || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Incubation_Period_Mean", &incubation_period_mean, Incubation_Period_Mean_DESC_TEXT, 0.0f, FLT_MAX, 6.0f );
            initConfigTypeMap( "Incubation_Period_Std_Dev", &incubation_period_std_dev, Incubation_Period_Std_Dev_DESC_TEXT, 0.0f, FLT_MAX, 1.0f );
        }

        if( incubation_distribution == DistributionFunction::POISSON_DURATION || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Incubation_Period_Mean", &incubation_period_mean, Incubation_Period_Mean_DESC_TEXT, 0.0f, FLT_MAX, 6.0f );
        }

        // Infectious_Duration...
        initConfig( "Infectious_Period_Distribution", infectious_distribution, config, MetadataDescriptor::Enum("infectious_distribution", Infectious_Period_Distribution_DESC_TEXT, MDD_ENUM_ARGS(DistributionFunction)) ); 
        LOG_DEBUG_F( "infectious_distribution = %s\n", DistributionFunction::pairs::lookup_key(infectious_distribution) );

        if( infectious_distribution == DistributionFunction::FIXED_DURATION || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Base_Infectious_Period", &infectious_period, Base_Infectious_Period_DESC_TEXT, 0.0f, FLT_MAX, 6.0f );
        }

        if( infectious_distribution == DistributionFunction::UNIFORM_DURATION || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Infectious_Period_Min", &infectious_period_min, Infectious_Period_Min_DESC_TEXT, 0.0f, FLT_MAX, 6.0f, "Infectious_Period_Distribution", "UNIFORM_DISTRIBUTION" );
            initConfigTypeMap( "Infectious_Period_Max", &infectious_period_max, Infectious_Period_Max_DESC_TEXT, 0.0f, FLT_MAX, 6.0f, "Infectious_Period_Distribution", "UNIFORM_DISTRIBUTION" );
        }

        if( infectious_distribution == DistributionFunction::EXPONENTIAL_DURATION || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Base_Infectious_Period", &infectious_period, Base_Infectious_Period_DESC_TEXT, 0.0f, FLT_MAX, 6.0f );
        }

        if( infectious_distribution == DistributionFunction::GAUSSIAN_DURATION || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Infectious_Period_Mean", &infectious_period_mean, Infectious_Period_Mean_DESC_TEXT, 0.0f, FLT_MAX, 6.0f );
            initConfigTypeMap( "Infectious_Period_Std_Dev", &infectious_period_std_dev, Infectious_Period_Std_Dev_DESC_TEXT, 0.0f, FLT_MAX, 1.0f );
        }
            
        if( infectious_distribution == DistributionFunction::POISSON_DURATION || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Infectious_Period_Mean", &infectious_period_mean, Infectious_Period_Mean_DESC_TEXT, 0.0f, FLT_MAX, 6.0f );
        }

        if( JsonConfigurable::_dryrun == false )
        {
            if( incubation_distribution == DistributionFunction::LOG_NORMAL_DURATION ||
                incubation_distribution == DistributionFunction::BIMODAL_DURATION ||
                infectious_distribution == DistributionFunction::LOG_NORMAL_DURATION ||
                infectious_distribution == DistributionFunction::BIMODAL_DURATION
              )
            {
                throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "LOG_NORMAL_DURATION and BIMODAL_DURATION are not yet supported for Incubation_Period_Distribution and Infectious_Period_Distribution." );
            }
        }

        initConfigTypeMap( "Base_Infectivity", &base_infectivity, Base_Infectivity_DESC_TEXT, 0.0f, 1000.0f, 0.3f ); // should default change depending on disease?
        initConfigTypeMap( "Base_Mortality", &base_mortality, Base_Mortality_DESC_TEXT, 0.0f, 1000.0f, 0.001f ); // should default change depending on disease?

        bool bRet = JsonConfigurable::Configure( config );

        return bRet;
    }

    Infection::Infection() :
        parent(NULL),
        suid(suids::nil_suid()),
        duration(0.0f),
        total_duration(0.0f),
        infectious_timer(0.0f),
        infectiousness(0.0f),
        StateChange(InfectionStateChange::None),
        infection_strain(NULL)
    {
    }

    Infection::Infection(IIndividualHumanContext *context) :
        parent(context),
        suid(suids::nil_suid()),
        duration(0.0f),
        total_duration(0.0f),
        infectious_timer(0.0f),
        infectiousness(0.0f),
        StateChange(InfectionStateChange::None),
        infection_strain(NULL)
    {
    }

    void Infection::Initialize(suids::suid _suid)
    {
        suid = _suid;
    }

    Infection *Infection::CreateInfection(IIndividualHumanContext *context, suids::suid _suid)
    {
        Infection *newinfection = _new_ Infection(context);
        newinfection->Initialize(_suid);

        return newinfection;
    }

    Infection::~Infection()
    {
        delete infection_strain;
    }

    void Infection::SetParameters(Kernel::StrainIdentity* infstrain, int incubation_period_override ) // or something
    {
        // Set up infection strain
        CreateInfectionStrain(infstrain);

        if( incubation_period_override != -1 )
        {
            incubation_timer = (float)incubation_period_override;
        }
        else
        {
            // have to do this again
            switch( incubation_distribution ) 
            {
                case DistributionFunction::FIXED_DURATION:
                    incubation_timer = incubation_period;
                    break;

                case DistributionFunction::UNIFORM_DURATION:
                    incubation_timer = Probability::getInstance()->fromDistribution( incubation_distribution, incubation_period_min, incubation_period_max );
                    break;

                case DistributionFunction::EXPONENTIAL_DURATION:
                    incubation_timer = Probability::getInstance()->fromDistribution( incubation_distribution, 1.0/incubation_period );
                    break;

                case DistributionFunction::GAUSSIAN_DURATION:
                    incubation_timer = Probability::getInstance()->fromDistribution( incubation_distribution, incubation_period_mean, incubation_period_std_dev );
                    break;

                case DistributionFunction::POISSON_DURATION:
                    incubation_timer = Probability::getInstance()->fromDistribution( incubation_distribution, incubation_period_mean );
                    break;

                default:
                    break;
            }
            LOG_DEBUG_F( "incubation_timer = %f\n", incubation_timer );
        }
        
        switch( infectious_distribution ) 
        {
            case DistributionFunction::FIXED_DURATION:
                infectious_timer = infectious_period;
                break;

            case DistributionFunction::UNIFORM_DURATION:
                infectious_timer = Probability::getInstance()->fromDistribution( infectious_distribution, infectious_period_min, infectious_period_max );
                break;

            case DistributionFunction::EXPONENTIAL_DURATION:
                infectious_timer = Probability::getInstance()->fromDistribution( infectious_distribution, 1.0/infectious_period );
                break;

            case DistributionFunction::GAUSSIAN_DURATION:
                infectious_timer = Probability::getInstance()->fromDistribution( infectious_distribution, infectious_period_mean, infectious_period_std_dev );
                break;

            case DistributionFunction::POISSON_DURATION:
                infectious_timer = Probability::getInstance()->fromDistribution( infectious_distribution, infectious_period_mean );
                break;

            default:
                break;
        }
        LOG_DEBUG_F( "infectious_timer = %f\n", infectious_timer );

        total_duration = incubation_timer + infectious_timer;
        infectiousness = 0;
        StateChange    = InfectionStateChange::None;

        if (incubation_timer <= 0)
        {
            infectiousness = base_infectivity;
        }
    }

    void Infection::InitInfectionImmunology(Susceptibility* _immunity)
    {
    }

    // TODO future : grant access to the susceptibility object by way of the host context and keep the update call neutral
    void Infection::Update(float dt, Susceptibility* immunity)
    {
        StateChange = InfectionStateChange::None;
        duration += dt;

// TODO        if (duration >= incubation_timer)
        if (duration > incubation_timer)
        {
            infectiousness = base_infectivity;

            // Used to have a release_assert( infectiousness ) here to make sure infectiousness was not zero, 
            // but setting infectiousness to zero can be a valid use case (e.g., while applying external incidence only)
        }

        // To query for mortality-reducing effects of drugs or vaccines
        IDrugVaccineInterventionEffects* idvie = NULL;

        // if disease has a daily mortality rate, and disease mortality is on, then check for death
        if (params()->vital_disease_mortality
            && (mortality_time_course == MortalityTimeCourse::DAILY_MORTALITY)
            && (duration > incubation_timer))
        {
            if ( s_OK != parent->GetInterventionsContext()->QueryInterface(GET_IID(IDrugVaccineInterventionEffects), (void**)&idvie) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetInterventionsContext()", "IDrugVaccineInterventionEffects", "IIndividualHumanInterventionsContext" );
            }

            if ( randgen->e() < base_mortality * dt * immunity->getModMortality() * idvie->GetInterventionReducedMortality() )
            { 
                StateChange = InfectionStateChange::Fatal; 
            }
        }

        if (duration > total_duration)
        {
            // disease mortality active and is accounted for at end of infectious period
            if (params()->vital_disease_mortality && (mortality_time_course == MortalityTimeCourse::MORTALITY_AFTER_INFECTIOUS))
            {
                if ( s_OK != parent->GetInterventionsContext()->QueryInterface(GET_IID(IDrugVaccineInterventionEffects), (void**)&idvie) )
                {
                    throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetInterventionsContext()", "IDrugVaccineInterventionEffects", "IIndividualHumanInterventionsContext" );
                }

                if ( randgen->e() < base_mortality * immunity->getModMortality() * idvie->GetInterventionReducedMortality() )
                {
                    StateChange = InfectionStateChange::Fatal;
                }
                else
                {
                    StateChange = InfectionStateChange::Cleared;
                }//  For debugging only  (1-base_mortality) recover, rest chance die at end of infection, modified by mortality immunity
            }
            else
            {
                StateChange = InfectionStateChange::Cleared;
            }
        }

        EvolveStrain(immunity, dt); // genomic modifications
    }

    void Infection::CreateInfectionStrain(StrainIdentity* infstrain)
    {
        if (infection_strain == NULL)
        {
            // this infection is new, not passed from another processor, so need to initialize the strain object
            infection_strain = _new_ Kernel::StrainIdentity;
        }

        if (infstrain != NULL)
        {
            *infection_strain = *infstrain;
            // otherwise, using the default antigenID and substrainID from the StrainIdentity constructor
        }
    }

    void Infection::EvolveStrain(Kernel::Susceptibility* immunity, float dt)
    {
        // genetic evolution happens here.
        // infection_strain
    }

    void Infection::GetInfectiousStrainID(Kernel::StrainIdentity* infstrain) 
    {
        *infstrain = *infection_strain;
    }

    void Infection::SetContextTo(IIndividualHumanContext* context) { parent = context; }

    IIndividualHumanContext* Infection::GetParent() { return parent; }

    suids::suid Infection::GetSuid() const { return suid; }

    const SimulationConfig* Infection::params() { return GET_CONFIGURABLE(SimulationConfig); } // overridden in derived classes but with different return types to hide the casting operation

    InfectionStateChange::_enum Infection::GetStateChange() const { return StateChange; }

    float Infection::GetInfectiousness() const { return infectiousness; }

    float Infection::GetInfectiousnessByRoute(string route) const {
        if( infectiousnessByRoute.find( route ) == infectiousnessByRoute.end() )
        {
            throw BadMapKeyException( __FILE__, __LINE__, __FUNCTION__, "infectiousnesssByRoute", route.c_str() );
        }
        return infectiousnessByRoute.at(route); 
    }

    float Infection::GetInfectiousPeriod() const { return infectious_period; }

    // Created for TB, but makes sense to be in base class, but no-one else is using yet, placeholder functionality
    bool Infection::IsActive() const
    {
        return false;
    }

    NonNegativeFloat
    Infection::GetDuration()
    const
    {
        return duration;
    }
}

#if USE_JSON_SERIALIZATION || USE_JSON_MPI
namespace Kernel
{

    void Infection::JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const
    {
        root->BeginObject();

        root->Insert("suid");
        suid.JSerialize(root, helper);

        root->Insert("duration", duration);
        root->Insert("total_duration", total_duration);
        root->Insert("incubation_timer", incubation_timer);
        root->Insert("infectious_timer", infectious_timer);
        root->Insert("infectiousness", infectiousness);

        root->Insert("contact_shedding_fraction", contact_shedding_fraction);

        // root->Insert("infectiousnessByRoute", infectiousnessByRoute);
        root->Insert("infectiousnessByRoute");
        helper->JSerialize(infectiousnessByRoute, root);

        root->Insert("StateChange", (int) StateChange);

        if (infection_strain)
        {
            root->Insert("infection_strain");
            infection_strain->JSerialize(root, helper);
        }

        root->EndObject();
    }
        
    void Infection::JDeserialize( IJsonObjectAdapter* root, JSerializer* helper )
    {
        rapidjson::Document * doc = (rapidjson::Document*) root; // total hack to get around build path issues with rapid json and abstraction
        suid.data                 = (*doc)[ "suid" ][ "data" ].GetInt();
        duration                  = (*doc)[ "duration" ].GetDouble();
        total_duration            = (*doc)[ "total_duration" ].GetDouble();
        incubation_timer          = (*doc)[ "incubation_timer" ].GetDouble();
        infectious_timer          = (*doc)[ "infectious_timer" ].GetDouble();
        infectiousness            = (*doc)[ "infectiousness" ].GetDouble();
        contact_shedding_fraction = (*doc)[ "contact_shedding_fraction" ].GetDouble();
        //infectiousnessByRoute   = (*doc)["infectiousnessByRoute"].GetMap();
        std::ostringstream errMsg;
        errMsg << "infectiousnessByRoute map serialization not yet implemented.";
        throw NotYetImplementedException(  __FILE__, __LINE__, __FUNCTION__, errMsg.str().c_str() );
        StateChange = static_cast<InfectionStateChange::_enum>((*doc)[ "StateChange" ].GetInt());

        if ((*doc).HasMember( "infection" ))
        {
            infection_strain = new StrainIdentity();
            infection_strain->JDeserialize((IJsonObjectAdapter*) &(*doc)[ "infection_strain" ], helper);
        }
        else
        {
            infection_strain = nullptr;
        }
    }
} // namespace Kernel

#endif

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
namespace Kernel {
    template<class Archive>
    void serialize(Archive & ar, Infection &inf, const unsigned int file_version )
    {
        static const char * _module = "Infection";
        LOG_DEBUG("(De)serializing Infection\n");

        ar & inf.infection_strain;
        ar & inf.suid; // unique id of this infection within the system

        // Local parameters
        ar & inf.duration;//local timer
        ar & inf.total_duration;
        ar & inf.incubation_timer;
        ar & inf.infectious_timer;
        ar & inf.infectiousness;
        ar & inf.infectiousnessByRoute;
        //  Lets individual know something has happened
        ar & inf.StateChange;
    }
    template void serialize( boost::mpi::packed_skeleton_iarchive&, Kernel::Infection&, unsigned int);
}
BOOST_CLASS_EXPORT(Kernel::Infection)
BOOST_CLASS_IMPLEMENTATION(Kernel::Infection, boost::serialization::object_serializable);
BOOST_CLASS_TRACKING(Kernel::Infection, boost::serialization::track_never);
#endif
