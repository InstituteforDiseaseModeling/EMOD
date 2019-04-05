/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "MultiInterventionDistributor.h"
#include "IndividualEventContext.h"
#include "IIndividualHumanContext.h"
#include "ISimulationContext.h"

SETUP_LOGGING( "MultiInterventionDistributor" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(MultiInterventionDistributor)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(MultiInterventionDistributor)

    IMPLEMENT_FACTORY_REGISTERED(MultiInterventionDistributor)

    bool MultiInterventionDistributor::Configure( const Configuration * inputJson )
    {
        initConfigComplexType("Intervention_List", &intervention_list, MID_Intervention_List_DESC_TEXT);

        bool ret = BaseIntervention::Configure( inputJson );
        if( ret )
        {
            InterventionValidator::ValidateInterventionArray( GetTypeName(),
                                                              InterventionTypeValidation::INDIVIDUAL,
                                                              intervention_list._json, 
                                                              inputJson->GetDataLocation() );
        }
        return ret ;
    }

    MultiInterventionDistributor::MultiInterventionDistributor()
    : BaseIntervention()
    {
    }

    MultiInterventionDistributor::~MultiInterventionDistributor()
    {
    }

    void MultiInterventionDistributor::Update( float dt )
    {
        // nothing to do here.  this intervention just distributes its list of interventions and then expires
    }
    
    bool MultiInterventionDistributor::Distribute(IIndividualHumanInterventionsContext *context, ICampaignCostObserver * const pICCO )
    {
        // ----------------------------------------------------------------------------------
        // --- Putting this here because we don't want anything to happen if we are aborting
        // ----------------------------------------------------------------------------------
        if( AbortDueToDisqualifyingInterventionStatus( context->GetParent() ) )
        {
            return false;
        }

        // Important: Use the instance method to obtain the intervention factory obj instead of static method to cross the DLL boundary
        IGlobalContext *pGC = nullptr;
        const IInterventionFactory* ifobj = nullptr;
        release_assert(context->GetParent());
        if (s_OK == context->GetParent()->QueryInterface(GET_IID(IGlobalContext), (void**)&pGC))
        {
            ifobj = pGC->GetInterventionFactory();
        }
        if (!ifobj)
        {
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "The pointer to IInterventionFactory object is not valid (could be DLL specific)" );
        } 

        try
        {
            // Parse intervention_list
            const json::Array & interventions_array = json::QuickInterpreter(intervention_list._json).As<json::Array>();
            LOG_DEBUG_F("interventions array size = %d\n", interventions_array.Size());
            for( int idx=0; idx<interventions_array.Size(); idx++ )
            {
                const json::Object& actualIntervention = json_cast<const json::Object&>(interventions_array[idx]);
                Configuration * tmpConfig = Configuration::CopyFromElement( actualIntervention, "campaign" );
                assert( tmpConfig );

                // Instantiate and distribute interventions
                LOG_DEBUG_F( "Attempting to instantiate intervention of class %s\n", std::string((*tmpConfig)["class"].As<json::String>()).c_str() );
                IDistributableIntervention *di = const_cast<IInterventionFactory*>(ifobj)->CreateIntervention(tmpConfig);
                release_assert( di != nullptr ); // ValidateInterventionArray should have made sure these are valid individual interventions
                if (!di->Distribute( context, pICCO ) )
                {
                    di->Release();
                }
            }
        }
        catch(json::Exception &e)
        {
            // ERROR: ::cerr << "exception casting intervention_config to array! " << e.what() << std::endl;
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, e.what() ); // ( "Intervention_List json problem: intervention_list is valid json but needs to be an array." );
        }

        // Nothing more for this class to do...
        Expire();

        return true;
    }

    void MultiInterventionDistributor::Expire()
    {
        expired = true;
    }
}

#if 0
namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, MultiInterventionDistributor& obj, const unsigned int v)
    {
        ar & obj.intervention_list;
        ar & boost::serialization::base_object<Kernel::BaseIntervention>(obj);
    }
}
#endif
