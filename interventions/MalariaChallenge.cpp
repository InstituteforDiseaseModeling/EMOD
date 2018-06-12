/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "MalariaChallenge.h"

#include "Exceptions.h"
#include "InterventionFactory.h"
#include "NodeMalariaEventContext.h"  // for ISporozoiteChallengeConsumer methods

SETUP_LOGGING( "MalariaChallenge" )

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(MalariaChallenge)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_INTERFACE(INodeDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(INodeDistributableIntervention)
    END_QUERY_INTERFACE_BODY(MalariaChallenge)

    IMPLEMENT_FACTORY_REGISTERED(MalariaChallenge)

    MalariaChallenge::MalariaChallenge()
    : BaseNodeIntervention()
    , challenge_type( MalariaChallengeType::InfectiousBites )
    , n_challenged_objects(1)
    , coverage(1.0)
    {
        initSimTypes( 1, "MALARIA_SIM" );
    }

    MalariaChallenge::MalariaChallenge( const MalariaChallenge& master )
    : BaseNodeIntervention( master )
    , challenge_type( master.challenge_type )
    , n_challenged_objects( master.n_challenged_objects )
    , coverage( master.coverage )
    {
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

        return BaseNodeIntervention::Configure( inputJson );
    }

    bool MalariaChallenge::Distribute(INodeEventContext *context, IEventCoordinator2* pEC)
    {
        parent = context;

        if( AbortDueToDisqualifyingInterventionStatus( context ) )
        {
            return false;
        }

        ISporozoiteChallengeConsumer *iscc;
        if (s_OK != context->QueryInterface(GET_IID(ISporozoiteChallengeConsumer), (void**)&iscc))
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "iscc", "ISporozoiteChallengeConsumer", "INodeEventContext");
        }

        bool wasDistributed = false;
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

        return wasDistributed;
    }

    void MalariaChallenge::Update( float dt )
    {
        // Distribute() doesn't call GiveIntervention() for this intervention, so it isn't added to the NodeEventContext's list of NDI
        throw IllegalOperationException(__FILE__, __LINE__, __FUNCTION__, "MalariaChallenge::Update() should not be called.");
    }
}
