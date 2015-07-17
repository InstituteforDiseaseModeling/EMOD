/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "NodeLevelHealthTriggeredIVScaleUpSwitch.h"

#include "InterventionFactory.h"
#include "Log.h"
#include "Debug.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)

static const char * _module = "NodeLevelHealthTriggeredIVScaleUpSwitch";

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED(NodeLevelHealthTriggeredIVScaleUpSwitch)

    NodeLevelHealthTriggeredIVScaleUpSwitch::NodeLevelHealthTriggeredIVScaleUpSwitch() : NodeLevelHealthTriggeredIV()
    , demographic_coverage_time_profile(ScaleUpProfile::Immediate)
    , initial_demographic_coverage(0)
    , primary_time_constant(0)
    {
    }

    NodeLevelHealthTriggeredIVScaleUpSwitch::~NodeLevelHealthTriggeredIVScaleUpSwitch() { }
 
    bool
    NodeLevelHealthTriggeredIVScaleUpSwitch::Configure(
        const Configuration * inputJson
    )
    {
        initConfig("Demographic_Coverage_Time_Profile",  demographic_coverage_time_profile, inputJson, MetadataDescriptor::Enum("Demographic_Coverage_Time_Profile", NodeHTI_Demographic_Coverage_Time_Profile_DESC_TEXT, MDD_ENUM_ARGS(ScaleUpProfile) ) );
        if ( demographic_coverage_time_profile == ScaleUpProfile::Linear || JsonConfigurable::_dryrun )
        {
            initConfigTypeMap("Initial_Demographic_Coverage", &initial_demographic_coverage, NodeHTI_Initial_Demographic_Coverage_DESC_TEXT, 0.0f, 1.0f, 0.0f);
            initConfigTypeMap("Primary_Time_Constant", &primary_time_constant, NodeHTI_Primary_Time_Constant_DESC_TEXT, 0, INFINITE_TIME); //this is actually the time to reach the final demographic coverage, input as absolute days, not number of dts
        }
        //this has to be an array rather than a single intervention json so that it can be empty if you don't want to phase anything out
        initConfigComplexType("Not_Covered_IndividualIntervention_Configs", &not_covered_intervention_configs, NodeHTI_Not_Covered_IndividualIntervention_Configs_DESC_TEXT);


        bool ret= NodeLevelHealthTriggeredIV::Configure(inputJson);
        return ret;

    }

    float NodeLevelHealthTriggeredIVScaleUpSwitch::getDemographicCoverage( ) const
    {
        
        float current_demographic_coverage;  

        switch (demographic_coverage_time_profile)
        {
        case ScaleUpProfile::Immediate:
            LOG_DEBUG("ScaleUpProfile is Immediate, don't need to update demographic coverage by time \n");
            current_demographic_coverage = demographic_coverage;
            break;

        case ScaleUpProfile::Linear:
            if (duration <= primary_time_constant) 
            {
                //the increment amount is ((demographic_coverage - initial_demog_coverage)/primary_time_constant) per day
                current_demographic_coverage = initial_demographic_coverage + ( (demographic_coverage - initial_demographic_coverage)/primary_time_constant) * duration;
                LOG_DEBUG_F("ScaleUpProfile is Linear, duration is %f, rate of %f, coverage is %f \n", duration, ( (demographic_coverage - initial_demographic_coverage)/primary_time_constant), current_demographic_coverage); 
            }
            else
            {
                current_demographic_coverage = demographic_coverage;
            }
            break;

        default:
            throw NotYetImplementedException( __FILE__, __LINE__, __FUNCTION__, "Only Immediate and Linear supported currently. \n" );
            break;
        }
        return current_demographic_coverage;
    }

    void NodeLevelHealthTriggeredIVScaleUpSwitch::onDisqualifiedByCoverage(
                IIndividualHumanEventContext *pIndiv
    )
    {
        //if qualify by everything except demographic coverage, give the not_covered_individualintervention_config 
        // this intervention is the one phased out as the actual_individualintervention_config is phased in
        // if the not_covered_individualintervention_config is NULL, then there is no intervention to phase out
        LOG_DEBUG("The person qualified by everything except demographic coverage, give the not_covered_individualintervention_config \n");


        // Query for campaign cost observer interface from INodeEventContext *parent
        ICampaignCostObserver *iCCO;
        if (s_OK != parent->QueryInterface(GET_IID(ICampaignCostObserver), (void**)&iCCO))
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "ICampaignCostObserver", "INodeEventContext" );
        }

        // Important: Use the instance method to obtain the intervention factory obj instead of static method to cross the DLL boundary
        //const IInterventionFactory* ifobj = dynamic_cast<NodeEventContextHost *>(parent)->GetInterventionFactoryObj();
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

		const json::Array & interventions_array = json::QuickInterpreter(not_covered_intervention_configs._json).As<json::Array>();
        LOG_DEBUG_F("not_covered_intervention_configs array size = %d\n", interventions_array.Size());
       
        if (interventions_array.Size() == 0 )
        {
            LOG_DEBUG("nothing to phase out \n"); 
        }
        else
        {
            for( int idx=0; idx<interventions_array.Size(); idx++ )
            {
                const json::Object& notcoveredIntervention = json_cast<const json::Object&>(interventions_array[idx]);
                Configuration * tmpConfig = Configuration::CopyFromElement(notcoveredIntervention);
                assert( tmpConfig );
                LOG_DEBUG_F("NodeHTIScaleUpSwitch will distribute notcoveredintervention #%d\n", idx);

                IDistributableIntervention *di = const_cast<IInterventionFactory*>(ifobj)->CreateIntervention(tmpConfig); 

                if( di )
                {
                    di->Distribute( pIndiv->GetInterventionsContext(), iCCO );
                    LOG_DEBUG("A Node level health-triggered intervention was successfully distributed, gave the not_covered_intervention_configs\n");
                    // It's not at all clear to me that we would incur cost at this point, but we could.
                    //iCCO->notifyCampaignExpenseIncurred( interventionCost, pIndiv );
                }
            }
         }       
    }
}

#if USE_BOOST_SERIALIZATION
BOOST_CLASS_EXPORT(Kernel::NodeLevelHealthTriggeredIVScaleUpSwitch)

namespace Kernel {
    REGISTER_SERIALIZATION_VOID_CAST(NodeLevelHealthTriggeredIVScaleUpSwitch, INodeDistributableIntervention)
    REGISTER_SERIALIZATION_VOID_CAST(NodeLevelHealthTriggeredIVScaleUpSwitch, IIndividualEventObserver)
    template<class Archive>
    void serialize(Archive &ar, NodeLevelHealthTriggeredIVScaleUpSwitch& iv, const unsigned int v)
    {
    }
}
#endif