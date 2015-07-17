/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include "OutbreakIndividual.h"
#include "Individual.h"
#include "InterventionFactory.h"
#include "ConfigurationImpl.h"
#include "NodeEventContext.h"
#include "Exceptions.h"
#include "SimulationConfig.h"

static const char * _module = "OutbreakIndividual";

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

    QuickBuilder OutbreakIndividual::GetSchema()
    {
        return QuickBuilder( jsonSchemaBase );
    }

    OutbreakIndividual::OutbreakIndividual()
    {
        initConfigTypeMap( "Antigen", &antigen, Antigen_DESC_TEXT, 0, 10, 0 );
        initConfigTypeMap( "Genome",  &genome,  Genome_DESC_TEXT, -1, 16777216, 0 );
        initConfigTypeMap( "Incubation_Period_Override", &incubation_period_override, Incubation_Period_Override_DESC_TEXT, false);
    }

    bool
    OutbreakIndividual::Configure(
        const Configuration * inputJson
    )
    {
        return JsonConfigurable::Configure( inputJson );
    }

    bool OutbreakIndividual::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * pCCO
    )
    {
        bool success = true;
        // TBD: Get individual from context, and infect
        IndividualHuman* individual = dynamic_cast<IndividualHuman*>(context->GetParent()); // QI in new code
        INodeEventContext * pContext = individual->GetParent()->GetEventContext();
        StrainIdentity* strain_identity = GetNewStrainIdentity(pContext);   // JPS: no need to create strain before we call this if we're calling into node...?
        LOG_DEBUG( "Infecting individual from Outbreak.\n" );
        individual->AcquireNewInfection( strain_identity, incubation_period_override );
        delete strain_identity;
        return success;
    }

    void OutbreakIndividual::Update( float dt )
    {
        LOG_WARN("updating outbreakIndividual (?!?)\n");
        // Distribute() doesn't call GiveIntervention() for this intervention, so it isn't added to the NodeEventContext's list of NDI
    }

    Kernel::StrainIdentity* OutbreakIndividual::GetNewStrainIdentity(INodeEventContext *context)
    {
        StrainIdentity *outbreakIndividual_strainID = NULL;

        // Important: Use the instance method to obtain the intervention factory obj instead of static method to cross the DLL boundary
        // NO usage of GET_CONFIGURABLE(SimulationConfig)->number_substrains in DLL
        IGlobalContext *pGC = NULL;
        const SimulationConfig* simConfigObj = NULL;
        if (s_OK == context->QueryInterface(GET_IID(IGlobalContext), (void**)&pGC))
        {
            simConfigObj = pGC->GetSimulationConfigObj();
        }
        if (!simConfigObj)
        {
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "The pointer to IInterventionFactory object is not valid (could be DLL specific)" );
        }

        if ( genome < 0 )
        {
            int ss = simConfigObj->number_substrains;
            if (ss & (ss-1))
            {
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Only supporting random genome generation for Number_Substrains as factor of two." );
            }
            unsigned int BARCODE_BITS = 0;
            while(ss >>= 1) ++BARCODE_BITS;
            uint32_t genome = context->GetRng()->ul() & ((1 << BARCODE_BITS)-1);
            //genome = context->GetRng()->i(simConfigObj->number_substrains);
            outbreakIndividual_strainID = _new_ StrainIdentity(antigen, genome);
            LOG_DEBUG_F("random genome generation... antigen: %d\t genome: %d\n", antigen, genome);
        }
        else if (genome >= 0 && genome < simConfigObj->number_substrains )
        {
            outbreakIndividual_strainID = _new_ StrainIdentity(antigen, genome);
        }
        else
        {
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "genome", genome, "number_substrains", simConfigObj->number_substrains );
        }

        return outbreakIndividual_strainID;
    }
}

#if USE_BOOST_SERIALIZATION
BOOST_CLASS_EXPORT(Kernel::OutbreakIndividual)
namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, OutbreakIndividual &ob, const unsigned int v)
    {
        boost::serialization::void_cast_register<OutbreakIndividual, IDistributableIntervention>();
        ar & ob.antigen;
        ar & ob.genome;
        ar & ob.incubation_period_override;
    }
}
#endif
