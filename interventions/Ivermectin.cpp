/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "Ivermectin.h"

#include <typeinfo>

#include "Contexts.h"
#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "VectorInterventionsContainer.h"

static const char* _module = "Ivermectin";

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED(Ivermectin)

    Ivermectin::Ivermectin( const Ivermectin& master )
    : BaseIntervention( master )
    {
        killing_config = master.killing_config;
        killing_effect = WaningEffectFactory::CreateInstance( Configuration::CopyFromElement( killing_config._json ) );
    }

    bool Ivermectin::Configure( const Configuration * inputJson )
    {
        initConfigComplexType("Killing_Config",  &killing_config, IVM_Killing_Config_DESC_TEXT );
        bool configured = JsonConfigurable::Configure( inputJson );
        if( !JsonConfigurable::_dryrun )
        {
            killing_effect = WaningEffectFactory::CreateInstance( Configuration::CopyFromElement( killing_config._json ) );
        }
        return configured;
    }

    Ivermectin::Ivermectin() :
        killing_effect(nullptr)
    {
        initSimTypes( 2, "VECTOR_SIM", "MALARIA_SIM" );
        initConfigTypeMap("Cost_To_Consumer", &cost_per_unit, IVM_Cost_To_Consumer_DESC_TEXT, 0, 999999, 8.0);
    }

    bool Ivermectin::Distribute( IIndividualHumanInterventionsContext *context,
                                 ICampaignCostObserver * const pCCO )
    {
        if (s_OK != context->QueryInterface(GET_IID(IVectorInterventionEffectsSetter), (void**)&ivies) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IVectorInterventionEffectsSetter", "IIndividualHumanInterventionsContext" );
        }
        context->PurgeExisting( typeid(*this).name() );
        return BaseIntervention::Distribute( context, pCCO );
    }

    void Ivermectin::SetContextTo( IIndividualHumanContext *context )
    {
        LOG_DEBUG("Ivermectin::SetContextTo (probably deserializing)\n");
        if (s_OK != context->GetInterventionsContext()->QueryInterface(GET_IID(IVectorInterventionEffectsSetter), (void**)&ivies) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IVectorInterventionEffectsSetter", "IIndividualHumanContext" );
        }
    }

    void Ivermectin::Update( float dt )
    {
        killing_effect->Update(dt);
        float current_killingrate = killing_effect->Current();
        ivies->UpdateInsecticidalDrugKillingProbability( current_killingrate );
        // Discard if efficacy is sufficiently low
        if (current_killingrate < 1e-5)
        {
            expired=true;
        }
    }

    BEGIN_QUERY_INTERFACE_BODY(Ivermectin)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
    END_QUERY_INTERFACE_BODY(Ivermectin)
}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::Ivermectin)
namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, Ivermectin& ivm, const unsigned int v)
    {
        static const char * _module = "Ivermectin";
        LOG_DEBUG("(De)serializing Ivermectin\n");

        ar & ivm.killing_effect;
        ar & boost::serialization::base_object<BaseIntervention>(ivm);
    }
}
#endif

