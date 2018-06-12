/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "Contexts.h"
#include "MultiNodeInterventionDistributor.h"
#include "NodeEventContext.h"

SETUP_LOGGING( "MultiNodeInterventionDistributor" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY( MultiNodeInterventionDistributor )
        HANDLE_INTERFACE( IConfigurable )
        HANDLE_INTERFACE( IBaseIntervention )
        HANDLE_INTERFACE( INodeDistributableIntervention )
        HANDLE_ISUPPORTS_VIA( INodeDistributableIntervention )
    END_QUERY_INTERFACE_BODY( MultiNodeInterventionDistributor )

    IMPLEMENT_FACTORY_REGISTERED( MultiNodeInterventionDistributor )

    bool MultiNodeInterventionDistributor::Configure( const Configuration * inputJson )
    {
        initConfigComplexType( "Node_Intervention_List", &intervention_list, MNID_Intervention_List_DESC_TEXT );

        bool ret = BaseNodeIntervention::Configure( inputJson );
        if( ret )
        {
            InterventionValidator::ValidateInterventionArray( GetTypeName(),
                                                              InterventionTypeValidation::NODE,
                                                              intervention_list._json,
                                                              inputJson->GetDataLocation() );
        }
        return ret;
    }

    MultiNodeInterventionDistributor::MultiNodeInterventionDistributor()
        : BaseNodeIntervention()
    {
    }

    MultiNodeInterventionDistributor::~MultiNodeInterventionDistributor()
    {
    }

    void MultiNodeInterventionDistributor::Update( float dt )
    {
        // nothing to do here.  this intervention just distributes its list of interventions and then expires
        release_assert( false );
    }

    bool MultiNodeInterventionDistributor::Distribute( INodeEventContext *context, IEventCoordinator2* pEC )
    {
        // ----------------------------------------------------------------------------------
        // --- Putting this here because we don't want anything to happen if we are aborting
        // ----------------------------------------------------------------------------------
        if( AbortDueToDisqualifyingInterventionStatus( context ) )
        {
            return false;
        }

        // Important: Use the instance method to obtain the intervention factory obj instead of static method to cross the DLL boundary
        IGlobalContext *pGC = nullptr;
        const IInterventionFactory* ifobj = nullptr;
        release_assert( context );
        if( s_OK == context->QueryInterface( GET_IID( IGlobalContext ), (void**)&pGC ) )
        {
            ifobj = pGC->GetInterventionFactory();
        }
        if( !ifobj )
        {
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "The pointer to IInterventionFactory object is not valid (could be DLL specific)" );
        }

        try
        {
            // Parse intervention_list
            const json::Array & interventions_array = json::QuickInterpreter( intervention_list._json ).As<json::Array>();
            LOG_DEBUG_F( "interventions array size = %d\n", interventions_array.Size() );
            for( int idx = 0; idx<interventions_array.Size(); idx++ )
            {
                const json::Object& actualIntervention = json_cast<const json::Object&>(interventions_array[ idx ]);
                Configuration * tmpConfig = Configuration::CopyFromElement( actualIntervention, "campaign" );
                assert( tmpConfig );

                // Instantiate and distribute interventions
                LOG_DEBUG_F( "Attempting to instantiate intervention of class %s\n", std::string( (*tmpConfig)[ "class" ].As<json::String>() ).c_str() );
                INodeDistributableIntervention *ndi = const_cast<IInterventionFactory*>(ifobj)->CreateNDIIntervention( tmpConfig );
                release_assert( ndi != nullptr ); // ValidateInterventionArray should have made sure these are valid individual interventions
                if( !ndi->Distribute( context, pEC ) )
                {
                    ndi->Release();
                }
            }
        }
        catch( json::Exception &e )
        {
            // ERROR: ::cerr << "exception casting intervention_config to array! " << e.what() << std::endl;
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, e.what() ); // ( "Intervention_List json problem: intervention_list is valid json but needs to be an array." );
        }

        // Nothing more for this class to do...
        SetExpired( true );

        return true;
    }
}
