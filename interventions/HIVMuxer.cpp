
#include "stdafx.h"
#include "HIVMuxer.h"
#include "MathFunctions.h"
#include "IndividualEventContext.h"
#include "IIndividualHumanContext.h"
#include "NodeEventContext.h"
#include "RANDOM.h"
#include "IHIVInterventionsContainer.h" // for access to campaign_semaphores

SETUP_LOGGING( "HIVMuxer" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(HIVMuxer, HIVDelayedIntervention)
    END_QUERY_INTERFACE_DERIVED(HIVMuxer, HIVDelayedIntervention)

    IMPLEMENT_FACTORY_REGISTERED(HIVMuxer)

    HIVMuxer::HIVMuxer()
    : HIVDelayedIntervention()
    , max_entries(1)
    , muxer_name("")
    , firstUpdate(true)
    , ihcs( nullptr )
    {
    }

    HIVMuxer::HIVMuxer( const HIVMuxer &master )
    : HIVDelayedIntervention( master )
    , max_entries(master.max_entries)
    , muxer_name(master.muxer_name)
    , firstUpdate(master.firstUpdate)
    , ihcs( nullptr )
    {
    }

    bool HIVMuxer::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap("Muxer_Name", &muxer_name, HIV_Muxer_Name_DESC_TEXT );
        initConfigTypeMap("Max_Entries", &max_entries, HIV_Max_Entries_DESC_TEXT , 0, INT_MAX, 1);

        bool ret = HIVDelayedIntervention::Configure(inputJson);
        return ret ;
    }

    void HIVMuxer::DelayValidate()
    {
        DelayedIntervention::DelayValidate();

        // The HIVMuxer can not work appropriately if the delay can be zero.
        if( !JsonConfigurable::_dryrun )
        {
            if( (delay_distribution->GetType() == DistributionFunction::CONSTANT_DISTRIBUTION) && (delay_distribution->GetParam1() == 0.0) )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Delay_Period_Distribution", "CONSTANT_DISTRIBUTION", "Delay_Period_Constant", "0" );
            }
            else if( delay_distribution->GetType() == DistributionFunction::GAUSSIAN_DISTRIBUTION )
            {
                // 99.7% of all numbers will be with 3 std devs of the mean
                float mean    = delay_distribution->GetParam1();
                float std_dev = delay_distribution->GetParam2();
                float min = mean - 3.0*std_dev ;
                float max = mean + 3.0*std_dev ;
                if( (min <= 0) && (0 <= max) )
                {
                    throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, 
                                                            "Delay_Distribution",
                                                            "GAUSSIAN_DISTRIBUTION",
                                                            "Delay_Period_Mean && Delay_Period_Std_Dev", 
                                                            "(can include zero)" );
                }
            }
        }
    }

    void HIVMuxer::CalculateDelay()
    {
        HIVDelayedIntervention::CalculateDelay();
        if( remaining_delay_days == 0.0 )
        {
            // The HIVMuxer can not work appropriately if the delay is zero.
            // throw OutOfRangeException( __FILE__, __LINE__, __FUNCTION__, "remaining_delay_days", 0.0f, 0.0f );
            LOG_WARN_F("remaining_delay_days == 0.  HIVMuxer can not work appropriately if the delay is zero.\n");
        }
    }

    bool HIVMuxer::Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pICCO )
    {
        bool distributed = HIVDelayedIntervention::Distribute(context, pICCO);
        if( distributed )
        {
            if ( s_OK != context->QueryInterface(GET_IID(IHIVCampaignSemaphores), (void **)&ihcs) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IHIVCampaignSemaphores", "IIndividualHumanInterventionsContext" );
            }

            // initialize the semaphore if not already done
            if (!ihcs->SemaphoreExists(muxer_name))
            {
                ihcs->SemaphoreInit(muxer_name, 0);
            }
        }
        return distributed;
    }

    void HIVMuxer::Update(float dt)
    {
        if( ihcs == nullptr )
        {
            if ( s_OK != parent->GetInterventionsContext()->QueryInterface(GET_IID(IHIVCampaignSemaphores), (void **)&ihcs) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent->GetInterventionsContext()", 
                                                                                 "IHIVCampaignSemaphores", 
                                                                                 "IIndividualHumanInterventionsContext" );
            }
        }

        if (firstUpdate)
        {
            int num_entries = ihcs->SemaphoreIncrement(muxer_name);
            expired = num_entries > max_entries;
            LOG_DEBUG_F("Individual %d now has %d instances of %s (allows %d).  expired == %d.\n", 
                         parent->GetSuid().data, num_entries, muxer_name.c_str(), max_entries, expired);
            firstUpdate = false;
        }

        if (!expired)   // if already expired (from the firstUpdate), skip this step
        {
            HIVDelayedIntervention::Update(dt);
        }

        if (expired)
        {
            if (!ihcs->SemaphoreDecrement(muxer_name))
            {
                LOG_WARN_F("Individual %d has freed more instances of %s than they created.\n", parent->GetSuid().data, muxer_name.c_str());
            }
            return;
        }
    }

    REGISTER_SERIALIZABLE(HIVMuxer);

    void HIVMuxer::serialize(IArchive& ar, HIVMuxer* obj)
    {
        HIVDelayedIntervention::serialize( ar, obj );
        HIVMuxer& muxer = *obj;

        ar.labelElement("max_entries") & muxer.max_entries;
        ar.labelElement("muxer_name" ) & muxer.muxer_name;
        ar.labelElement("firstUpdate") & muxer.firstUpdate;
    }
}
