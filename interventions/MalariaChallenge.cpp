/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "MalariaChallenge.h"

#include "Exceptions.h"
#include "InterventionFactory.h"
#include "NodeMalariaEventContext.h"  // for ISporozoiteChallengeConsumer methods

static const char * _module = "MalariaChallenge";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(MalariaChallenge)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(INodeDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(INodeDistributableIntervention)
    END_QUERY_INTERFACE_BODY(MalariaChallenge)

    IMPLEMENT_FACTORY_REGISTERED(MalariaChallenge)

    QuickBuilder MalariaChallenge::GetSchema()
    {
        return QuickBuilder( jsonSchemaBase );
    }

    MalariaChallenge::MalariaChallenge()
    {
        initSimTypes( 1, "MALARIA_SIM" );
    }

    bool MalariaChallenge::Configure( const Configuration * inputJson )
    {
        initConfig( "Challenge_Type", challenge_type, inputJson, MetadataDescriptor::Enum("Challenge_Type", MC_Challenge_Type_DESC_TEXT, MDD_ENUM_ARGS(MalariaChallengeType)) );
        initConfigTypeMap( "Coverage", &coverage, MC_Coverage_DESC_TEXT, 0, 1, 1 );        

        if (!JsonConfigurable::_dryrun)
        {
            switch ( challenge_type )
            {
            case MalariaChallengeType::InfectiousBites:
                initConfigTypeMap( "Infectious_Bite_Count", &n_challenged_objects, MC_Infectious_Bite_Count_DESC_TEXT, 0, 1000, 1 );
                break;

            case MalariaChallengeType::Sporozoites:
                initConfigTypeMap( "Sporozoite_Count", &n_challenged_objects, MC_Sporozoite_Count_DESC_TEXT, 0, 1000, 1 );
                break;

            default:
                if( !JsonConfigurable::_dryrun )
                {
                    throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "challenge_type", challenge_type, MalariaChallengeType::pairs::lookup_key( challenge_type ) );
                }
            }
        }
        else
        {
            initConfigTypeMap( "Infectious_Bite_Count", &n_challenged_objects, MC_Infectious_Bite_Count_DESC_TEXT, 0, 1000, 1 );
            initConfigTypeMap( "Sporozoite_Count", &n_challenged_objects, MC_Sporozoite_Count_DESC_TEXT, 0, 1000, 1 );
        }

        JsonConfigurable::Configure( inputJson );
        return true;
    }

    bool MalariaChallenge::Distribute(INodeEventContext *context, IEventCoordinator2* pEC)
    {
        bool wasDistributed = false;

        ISporozoiteChallengeConsumer *iscc;
        if (s_OK == context->QueryInterface(GET_IID(ISporozoiteChallengeConsumer), (void**)&iscc))
        {
            if(this->challenge_type == MalariaChallengeType::InfectiousBites)
            {
                iscc->ChallengeWithInfectiousBites(n_challenged_objects, coverage);
                wasDistributed = true;
            }
            else if(this->challenge_type == MalariaChallengeType::Sporozoites)
            {
                iscc->ChallengeWithSporozoites(n_challenged_objects, coverage);
                wasDistributed = true;
            }
        }
        else
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "iscc", "ISporozoiteChallengeConsumer", "INodeEventContext" );
        }

        return wasDistributed;
    }

    void MalariaChallenge::Update( float dt )
    {
        LOG_WARN("updating malaria challenge (?!?)\n");
        // Distribute() doesn't call GiveIntervention() for this intervention, so it isn't added to the NodeEventContext's list of NDI
    }
}
