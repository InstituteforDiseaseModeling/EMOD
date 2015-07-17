/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "DelayedIntervention.h"
#include "MathFunctions.h"
#include "IndividualEventContext.h"
#include "NodeEventContext.h"
#include "RANDOM.h"

static const char * _module = "DelayedIntervention";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(DelayedIntervention)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(DelayedIntervention)

    IMPLEMENT_FACTORY_REGISTERED(DelayedIntervention)

    bool DelayedIntervention::PreConfigure( const Configuration * inputJson )
    {
        initConfigTypeMap("Coverage", &coverage, DI_Coverage_DESC_TEXT, 0.0f, 1.0f, 1.0f);
        return initConfig( "Delay_Distribution", delay_distribution, inputJson, MetadataDescriptor::Enum("Delay_Distribution", DI_Delay_Distribution_DESC_TEXT, MDD_ENUM_ARGS(DistributionFunction)));
    }

    void DelayedIntervention::DistributionConfigure( const Configuration * inputJson )
    {
        // DJK: Should pass inputJson to factor that creates instance of IDelayDistribution <ERAD-1852>
        if( delay_distribution == DistributionFunction::FIXED_DURATION || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Delay_Period", &delay_period, DI_Delay_Period_DESC_TEXT, 0.0f, FLT_MAX, 6.0f ); // should default change depending on disease?
        }

        if( delay_distribution == DistributionFunction::UNIFORM_DURATION || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Delay_Period_Min", &delay_period_min, DI_Delay_Period_Min_DESC_TEXT, 0.0f, FLT_MAX, 6.0f, "Delay_Distribution", "UNIFORM_DISTRIBUTION" );
            initConfigTypeMap( "Delay_Period_Max", &delay_period_max, DI_Delay_Period_Max_DESC_TEXT, 0.0f, FLT_MAX, 6.0f, "Delay_Distribution", "UNIFORM_DISTRIBUTION" );
        }

        if( delay_distribution == DistributionFunction::EXPONENTIAL_DURATION || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Delay_Period", &delay_period, DI_Delay_Period_DESC_TEXT, 0.0f, FLT_MAX, 6.0f );
        }

        if( delay_distribution == DistributionFunction::GAUSSIAN_DURATION || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap( "Delay_Period_Mean", &delay_period_mean, DI_Delay_Period_Mean_DESC_TEXT, 0.0f, FLT_MAX, 6.0f, "Delay_Distribution", "GAUSSIAN_DURATION" );
            initConfigTypeMap( "Delay_Period_Std_Dev", &delay_period_std_dev, DI_Delay_Period_Std_Dev_DESC_TEXT, 0.0f, FLT_MAX, 1.0f, "Delay_Distribution", "GAUSSIAN_DURATION" );
        }
    }

    void DelayedIntervention::InterventionConfigure( const Configuration * inputJson )
    {
        initConfigComplexType("Actual_IndividualIntervention_Configs", &actual_intervention_config, DI_Actual_IndividualIntervention_Configs_DESC_TEXT);
    }

    void DelayedIntervention::InterventionValidate()
    {
        InterventionValidator::ValidateInterventionArray( actual_intervention_config._json );
    }

    void DelayedIntervention::DelayValidate()
    {
        if( !JsonConfigurable::_dryrun )
        {
            if( (delay_distribution == DistributionFunction::UNIFORM_DURATION) &&
                (delay_period_min >= delay_period_max) )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Delay_Distribution", "UNIFORM_DURATION", "Delay_Period_Min >= Delay_Period_Max", "(min > max)" );
            }
            else if( (delay_distribution == DistributionFunction::EXPONENTIAL_DURATION) &&
                        (delay_period == 0.0) )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Delay_Distribution", "EXPONENTIAL_DURATION", "Delay_Period", "0" );
            }
        }
    }

    bool DelayedIntervention::Configure( const Configuration * inputJson )
    {
        PreConfigure(inputJson);
        DistributionConfigure(inputJson);
        InterventionConfigure(inputJson);

        bool retValue = JsonConfigurable::Configure( inputJson );
        if( retValue )
        {
            InterventionValidate();
            DelayValidate();
        }
        return retValue ;
    }

    bool DelayedIntervention::Distribute(
        IIndividualHumanInterventionsContext *context, // interventions container usually
        ICampaignCostObserver * const pICCO
    )
    {
        // Distribute this intervention, which contains the callback to set the parent context
        if( !BaseIntervention::Distribute(context, pICCO) )
            return false;

        // If individual isn't covered, immediately set DelayedIntervention to expired without distributing "actual" IVs
        if( coverage < 1.0f && parent->GetRng()->e() > coverage )
        {
            LOG_DEBUG_F("Random draw outside of %0.2f covered fraction in DelayedIntervention.\n", coverage);
            expired = true;
            return false;
        }

        CalculateDelay();

        LOG_DEBUG_F("Drew %0.2f remaining delay days in %s.\n", remaining_delay_days, DistributionFunction::pairs::lookup_key(delay_distribution));
        return true;
    }

    void
    DelayedIntervention::CalculateDelay()
    {
        switch (delay_distribution)
        {
        case DistributionFunction::FIXED_DURATION:
            remaining_delay_days = delay_period;
            break;

        case DistributionFunction::UNIFORM_DURATION:
            remaining_delay_days = Probability::getInstance()->fromDistribution( delay_distribution, delay_period_min, delay_period_max );
            break;

        case DistributionFunction::EXPONENTIAL_DURATION:
            remaining_delay_days = Probability::getInstance()->fromDistribution( delay_distribution, 1.0/delay_period );
            break;

        case DistributionFunction::GAUSSIAN_DURATION:
            remaining_delay_days = Probability::getInstance()->fromDistribution( delay_distribution, delay_period_mean, delay_period_std_dev );
            break;

        default:
            throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "Only fixed/uniform/gaussian/exponential supported currently." );
        }
        LOG_DEBUG_F("Drew %0.2f remaining delay days in %s.\n", remaining_delay_days, DistributionFunction::pairs::lookup_key(delay_distribution));
    }

    DelayedIntervention::DelayedIntervention()
    : BaseIntervention()
    , parent(nullptr)
    , remaining_delay_days(0.0)
    , coverage(1.0)
    , delay_distribution(DistributionFunction::EXPONENTIAL_DURATION)
    , delay_period(1.0)
    , delay_period_min(0.0)
    , delay_period_max(0.0)
    , delay_period_mean(0.0)
    , delay_period_std_dev(0.0)
    , actual_intervention_config()
    {
    }

    DelayedIntervention::DelayedIntervention( const DelayedIntervention& master )
        :BaseIntervention( master )
    {
    	remaining_delay_days = master.remaining_delay_days;
    	coverage = master.coverage;
    	delay_distribution = master.delay_distribution;
    	delay_period = master.delay_period;
    	delay_period_min = master.delay_period_min;
    	delay_period_max = master.delay_period_max;
    	delay_period_mean = master.delay_period_mean;
    	delay_period_std_dev = master.delay_period_std_dev;
    	actual_intervention_config = master.actual_intervention_config;
    }

    void DelayedIntervention::SetContextTo(IIndividualHumanContext *context) 
    { 
        parent = context; // for rng
    }

    void DelayedIntervention::Update( float dt )
    {

        if( remaining_delay_days > 0 )
        {
            remaining_delay_days -= dt;
            return;
        }

        try
        {
            // TODO: factorize this bit out so it isn't repeating base class behavior here
            bool wasDistributed = false;

            // Important: Use the instance method to obtain the intervention factory obj instead of static method to cross the DLL boundary
            IGlobalContext *pGC = NULL;
            const IInterventionFactory* ifobj = NULL;
            if (s_OK == parent->QueryInterface(GET_IID(IGlobalContext), (void**)&pGC))
            {
                ifobj = pGC->GetInterventionFactory();
            }
            if (!ifobj)
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "The pointer to IInterventionFactory object is not valid (could be DLL specific)" );
            }

            // don't give expired intervention.  should be cleaned up elsewhere anyways, though.
            if(expired) 
                return;

            const json::Array & interventions_array = json::QuickInterpreter( actual_intervention_config._json ).As<json::Array>();
            LOG_DEBUG_F("interventions array size = %d\n", interventions_array.Size());
            for( int idx=0; idx<interventions_array.Size(); idx++ )
            {
                const json::Object& actualIntervention = json_cast<const json::Object&>(interventions_array[idx]);
                Configuration * tmpConfig = Configuration::CopyFromElement(actualIntervention);
                release_assert( tmpConfig );
                LOG_DEBUG_F("DelayedIntervention distributed intervention #%d\n", idx);
                IDistributableIntervention *di = const_cast<IInterventionFactory*>(ifobj)->CreateIntervention(tmpConfig); 
                delete tmpConfig;
                expired = true;

                // Now make sure cost gets reported back to node
                ICampaignCostObserver* pICCO;
                if (s_OK == parent->GetEventContext()->GetNodeEventContext()->QueryInterface(GET_IID(ICampaignCostObserver), (void**)&pICCO) )
                {
                    di->Distribute( parent->GetInterventionsContext(), pICCO );
                }
                else
                {
                    throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetEventContext()->GetNodeEventContext()", "ICampaignCostObserver", "INodeEventContext");
                }
            }
        }
        catch(json::Exception &e)
        {
            // ERROR: ::cerr << "exception casting actual_intervention_config to array! " << e.what() << std::endl;
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, e.what() ); // ( "DelayedIntervention intervention json problem: actual_intervention_config is valid json but needs to be an array." );
        }

    }
    
    DelayedIntervention::~DelayedIntervention()
    { LOG_DEBUG("Destructing DelayedIntervention\n");
    }
}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::DelayedIntervention)

namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, DelayedIntervention& obj, const unsigned int v)
    {
        boost::serialization::void_cast_register<DelayedIntervention, IDistributableIntervention>();
        ar & obj.remaining_delay_days;

        // ERAD-1235: Note that an unregistered class exception is thrown when serializing the 
        //            base class, SimpleHealthSeekingBehavior, if we don't call the following two
        //            lines but instead do serialization::base_object<Kernel::SimpleHealthSeekingBehavior>(obj)
        ar & obj.actual_intervention_config;
        ar & boost::serialization::base_object<Kernel::BaseIntervention>(obj);
    }
}
#endif
