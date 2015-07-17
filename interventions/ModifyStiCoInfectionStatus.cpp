/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include "ModifyStiCoInfectionStatus.h"
#include "Exceptions.h"
#include "HIVInterventionsContainer.h"

static const char * _module = "ModifyStiCoInfectionStatus";

// Important: Use the instance method to obtain the intervention factory obj instead of static method to cross the DLL boundary
// NO USAGE like this:  GET_CONFIGURABLE(SimulationConfig)->number_substrains in DLL

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(ModifyStiCoInfectionStatus)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(ModifyStiCoInfectionStatus)


    IMPLEMENT_FACTORY_REGISTERED(ModifyStiCoInfectionStatus)

    QuickBuilder ModifyStiCoInfectionStatus::GetSchema()
    {
        return QuickBuilder( jsonSchemaBase );
    }

    ModifyStiCoInfectionStatus::ModifyStiCoInfectionStatus()
    {
        initConfigTypeMap( "New_STI_CoInfection_Status", &set_flag_to, MSCIS_New_STI_Co_Status_DESC_TEXT, false );
    }

    bool
    ModifyStiCoInfectionStatus::Configure(
        const Configuration * inputJson
    )
    {
        return JsonConfigurable::Configure( inputJson );
    }

    bool ModifyStiCoInfectionStatus::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * pCCO
    )
    {
        bool success = true;
        ISTICoInfectionStatusChangeApply * interventions_container = nullptr;
        if (s_OK != context->QueryInterface(GET_IID(ISTICoInfectionStatusChangeApply), (void**)&interventions_container) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "ISTICoInfectionStatusChangeApply", "IIndividualHumanInterventionsContext" );
        } 
        LOG_DEBUG( "Setting/clearing HIV individual's (non-HIV) STI Co-Infection status (flag).\n" );
        if( set_flag_to == true )
        {
            interventions_container->SpreadStiCoInfection();
        }
        else
        {
            interventions_container->CureStiCoInfection();
        }

        return success;
    }

    void ModifyStiCoInfectionStatus::Update( float dt )
    {
    }
}

#if USE_BOOST_SERIALIZATION
BOOST_CLASS_EXPORT(Kernel::ModifyStiCoInfectionStatus)
namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, ModifyStiCoInfectionStatus &ob, const unsigned int v)
    {
        boost::serialization::void_cast_register<ModifyStiCoInfectionStatus, IDistributableIntervention>();
    }
}
#endif
