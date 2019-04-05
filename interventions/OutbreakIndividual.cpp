/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "OutbreakIndividual.h"
#include "IIndividualHuman.h"
#include "IIndividualHumanContext.h"
#include "ISimulationContext.h"
#include "InterventionFactory.h"
#include "ConfigurationImpl.h"
#include "NodeEventContext.h"
#include "Exceptions.h"
#include "SimulationConfig.h"
#include "StrainIdentity.h"
#include "IdmString.h"
#include "Node.h" // for number_substrains. Move to StrainIdentity
#include "RANDOM.h"

SETUP_LOGGING( "OutbreakIndividual" )

// Important: Use the instance method to obtain the intervention factory obj instead of static method to cross the DLL boundary
// NO USAGE like this:  GET_CONFIGURABLE(SimulationConfig)->number_substrains in DLL

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(OutbreakIndividual)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_INTERFACE(IOutbreakIndividual)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(OutbreakIndividual)


    IMPLEMENT_FACTORY_REGISTERED(OutbreakIndividual)

    OutbreakIndividual::OutbreakIndividual()
        : antigen(0)
        , genome(0)
        , ignoreImmunity( true )
        , incubation_period_override(-1)
    {
        initConfigTypeMap( "Ignore_Immunity", &ignoreImmunity, OB_Ignore_Immunity_DESC_TEXT, true );
        initConfigTypeMap( "Incubation_Period_Override", &incubation_period_override, Incubation_Period_Override_DESC_TEXT, -1, INT_MAX, -1);
        initSimTypes( 11, "GENERIC_SIM" , "VECTOR_SIM" , "MALARIA_SIM", "AIRBORNE_SIM", "POLIO_SIM", "TBHIV_SIM", "STI_SIM", "HIV_SIM", "PY_SIM", "TYPHOID_SIM", "ENVIRONMENTAL_SIM" );
    }

    bool
    OutbreakIndividual::Configure(
        const Configuration * inputJson
    )
    {
        ConfigureAntigen( inputJson );
        ConfigureGenome( inputJson );

        // --------------------------------------------------------------
        // --- Don't call BaseIntervention::Configure() because we don't
        // --- want to inherit those parameters.
        // --------------------------------------------------------------
        return JsonConfigurable::Configure( inputJson );
    }

    void OutbreakIndividual::ConfigureAntigen( const Configuration * inputJson )
    {
        initConfigTypeMap( "Antigen", &antigen, Antigen_DESC_TEXT, 0, 10, 0 );
    }

    void OutbreakIndividual::ConfigureGenome( const Configuration * inputJson )
    {
        initConfigTypeMap( "Genome", &genome, Genome_DESC_TEXT, -1, 16777216, 0 );
    }

    bool OutbreakIndividual::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * pCCO
    )
    {
        bool distributed = false;
        // TBD: Get individual from context, and infect
        IIndividualHuman* individual = dynamic_cast<IIndividualHuman*>(context->GetParent()); // QI in new code
        INodeEventContext * pContext = individual->GetParent()->GetEventContext();
        LOG_DEBUG( "Infecting individual from Outbreak.\n" );
        if( ignoreImmunity || // if we're ignoring immunity, just infect
            context->GetParent()->GetRng()->SmartDraw( individual->GetAcquisitionImmunity() ) )
        {
            individual->AcquireNewInfection( GetNewStrainIdentity( pContext, context->GetParent() ), incubation_period_override );
            distributed = true;
        }
        else
        {
            LOG_DEBUG_F( "We didn't infect individual %d with immunity %f (ignore=%d).\n", individual->GetSuid().data, individual->GetAcquisitionImmunity(), ignoreImmunity );
        }
        return distributed;
    }

    void OutbreakIndividual::Update( float dt )
    {
        LOG_WARN("updating outbreakIndividual (?!?)\n");
        // Distribute() doesn't call GiveIntervention() for this intervention, so it isn't added to the NodeEventContext's list of NDI
    }

    const Kernel::StrainIdentity* OutbreakIndividual::GetNewStrainIdentity( INodeEventContext *context, IIndividualHumanContext* pIndiv )
    {
        StrainIdentity *outbreakIndividual_strainID = nullptr;

        // Important: Use the instance method to obtain the intervention factory obj instead of static method to cross the DLL boundary
        // NO usage of GET_CONFIGURABLE(SimulationConfig)->number_substrains in DLL
        IGlobalContext *pGC = nullptr;
        const SimulationConfig* simConfigObj = nullptr;
        if (s_OK == context->QueryInterface(GET_IID(IGlobalContext), (void**)&pGC))
        {
            simConfigObj = pGC->GetSimulationConfigObj();
        }
        if (!simConfigObj)
        {
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "The pointer to IInterventionFactory object is not valid (could be DLL specific)" );
        }

        //if (( antigen < 0 ) || ( antigen >= simConfigObj->number_basestrains ))
        if( antigen < 0 )
        {
            //throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "antigen", antigen, "number_basestrains", simConfigObj->number_basestrains );
            throw ConfigurationRangeException( __FILE__, __LINE__, __FUNCTION__, "antigen", antigen, 0 );
        }

        outbreakIndividual_strainID = _new_ StrainIdentity( antigen, genome, pIndiv->GetRng() );

        return outbreakIndividual_strainID;
    }
}
