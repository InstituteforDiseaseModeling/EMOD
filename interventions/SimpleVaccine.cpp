/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "Vaccine.h"

#include "Common.h"                  // for INFINITE_TIME
#include "Contexts.h"                // for IIndividualHumanContext, IIndividualHumanInterventionsContext
#include "InterventionsContainer.h"  // for IVaccineConsumer methods
#include "RANDOM.h"                  // for ApplyVaccineTake random draw

// TBD: currently included for JDeserialize only. Once we figure out how to wrap the deserialize
// into rapidjsonimpl class, then this is not needed
#include "RapidJsonImpl.h"

static const char* _module = "SimpleVaccine";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY(SimpleVaccine)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(IDistributableIntervention)
        HANDLE_INTERFACE(IVaccine)
        HANDLE_INTERFACE(IBaseIntervention)
        HANDLE_ISUPPORTS_VIA(IDistributableIntervention)
        HANDLE_INTERFACE(IBaseIntervention)
    END_QUERY_INTERFACE_BODY(SimpleVaccine)

    IMPLEMENT_FACTORY_REGISTERED(SimpleVaccine)

    bool
    SimpleVaccine::Configure(
        const Configuration * inputJson
    )
    {
        initConfigTypeMap("Vaccine_Take", &vaccine_take, SV_Vaccine_Take_DESC_TEXT, 0.0, 1.0, 1.0 ); 

        initConfig( "Vaccine_Type", vaccine_type, inputJson, MetadataDescriptor::Enum("Vaccine_Type", SV_Vaccine_Type_DESC_TEXT, MDD_ENUM_ARGS(SimpleVaccineType)));

        initConfigComplexType("Waning_Config",  &waning_config, IVM_Killing_Config_DESC_TEXT );
        bool configured = JsonConfigurable::Configure( inputJson );
        if( !JsonConfigurable::_dryrun )
        {
            auto tmp_waning = Configuration::CopyFromElement( waning_config._json );
            waning_effect = WaningEffectFactory::CreateInstance( tmp_waning );
            delete tmp_waning;
            tmp_waning = nullptr;
        }
        //release_assert( vaccine_type );
        LOG_DEBUG_F( "Vaccine configured with type %d and take %f.\n", vaccine_type, vaccine_take );
        return configured;
    }

    SimpleVaccine::SimpleVaccine() 
    : BaseIntervention()
    , parent(nullptr) 
    , vaccine_type(SimpleVaccineType::Generic)
    , vaccine_take(0.0)
    , current_reducedacquire(0.0)
    , current_reducedtransmit(0.0)
    , current_reducedmortality(0.0) 
    , waning_effect( nullptr )
    {
        initConfigTypeMap("Cost_To_Consumer", &cost_per_unit, SV_Cost_To_Consumer_DESC_TEXT, 0, 999999, 10.0);
    }

    SimpleVaccine::SimpleVaccine( const SimpleVaccine& master )
    : BaseIntervention( master )
    {
        vaccine_type             = master.vaccine_type;
        vaccine_take             = master.vaccine_take;
        current_reducedacquire   = master.current_reducedacquire;
        current_reducedtransmit  = master.current_reducedtransmit;
        current_reducedmortality = master.current_reducedmortality;
        waning_config            = master.waning_config;

        auto tmp_waning = Configuration::CopyFromElement( waning_config._json );
        waning_effect = WaningEffectFactory::CreateInstance( tmp_waning );
        delete tmp_waning;
        tmp_waning = nullptr;
    }

    SimpleVaccine::~SimpleVaccine()
    {
        delete waning_effect;
    }

    /////////////////////////////////////////////////////////////////////////////////////

    // context is nothing more than ISupports really, and it's a pointer to the individual's
    // intervention container, not the individual itself. It was gotten by a call to
    // pIndividual->GetInterventionsContext().

    bool
    SimpleVaccine::Distribute(
        IIndividualHumanInterventionsContext *context,
        ICampaignCostObserver * pCCO
    )
    {
        // store ivc for apply
        LOG_DEBUG("Distributing SimpleVaccine.\n");
        if (s_OK != context->QueryInterface(GET_IID(IVaccineConsumer), (void**)&ivc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IVaccineConsumer", "IIndividualHumanInterventionsContext" );
        }
        bool success = BaseIntervention::Distribute( context, pCCO );
        ApplyVaccineTake();
        LOG_DEBUG_F( "Vaccine distributed with type %d and take %f for individual %d\n", vaccine_type, vaccine_take, parent->GetSuid().data );
        return success;
    }

    void SimpleVaccine::Update( float dt )
    {
        waning_effect->Update(dt);
        release_assert(ivc);

        switch( vaccine_type )
        {
            case SimpleVaccineType::AcquisitionBlocking:
                current_reducedacquire = waning_effect->Current();
                ivc->UpdateVaccineAcquireRate( current_reducedacquire );
                break;

            case SimpleVaccineType::TransmissionBlocking:
                current_reducedtransmit  = waning_effect->Current();
                ivc->UpdateVaccineTransmitRate( current_reducedtransmit );
                break;

            case SimpleVaccineType::MortalityBlocking:
                current_reducedmortality  = waning_effect->Current(); 
                ivc->UpdateVaccineMortalityRate( current_reducedmortality );
                break;

            case SimpleVaccineType::Generic:
                current_reducedacquire = waning_effect->Current();
                ivc->UpdateVaccineAcquireRate( current_reducedacquire );
                current_reducedtransmit  = waning_effect->Current();
                ivc->UpdateVaccineTransmitRate( current_reducedtransmit );
                current_reducedmortality  = waning_effect->Current(); 
                ivc->UpdateVaccineMortalityRate( current_reducedmortality );
                break;

            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "vaccine_type", vaccine_type, SimpleVaccineType::pairs::lookup_key( vaccine_type ) );
                break;
        }
    }

/*
    Kernel::QueryResult SimpleVaccine::QueryInterface( iid_t iid, void **ppinstance )
    {
        assert(ppinstance);

        if ( !ppinstance )
            return e_NULL_POINTER;

        ISupports* foundInterface;

        if ( iid == GET_IID(IVaccine))
            foundInterface = static_cast<IVaccine*>(this);
        else if ( iid == GET_IID(ISupports))
            foundInterface = static_cast<ISupports*>(static_cast<IVaccine*>(this));
        else
            foundInterface = 0;

        QueryResult status;
        if ( !foundInterface )
            status = e_NOINTERFACE;
        else
        {
            //foundInterface->AddRef();           // not implementing this yet!
            status = s_OK;
        }

        *ppinstance = foundInterface;
        return status;
    }*/

    void SimpleVaccine::ApplyVaccineTake()
    {
        if(parent)
        {
            if(vaccine_take<1.0)
            {
                if(parent->GetRng()->e()>vaccine_take)
                {
                    LOG_DEBUG("Vaccine did not take.\n");
                    expired = true;
                }
            }
        }
    }

    void SimpleVaccine::SetContextTo(
        IIndividualHumanContext *context
    )
    {
        parent = context;
        if (s_OK != parent->GetInterventionsContext()->QueryInterface(GET_IID(IVaccineConsumer), (void**)&ivc) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context->GetInterventionsContext()", "IVaccineConsumer", "IIndividualHumanInterventionsContext" );
        }
        LOG_DEBUG_F( "Vaccine configured with type %d and take %f for individual %d\n", vaccine_type, vaccine_take, parent->GetSuid().data );
    } // needed for VaccineTake

    REGISTER_SERIALIZABLE(SimpleVaccine);

    void SimpleVaccine::serialize(IArchive& ar, SimpleVaccine* obj)
    {
        BaseIntervention::serialize( ar, obj );
        SimpleVaccine& vaccine = *obj;
        ar.labelElement("vaccine_type")                  & vaccine.vaccine_type;
        ar.labelElement("vaccine_take")                  & vaccine.vaccine_take;
        ar.labelElement("current_reducedacquire")        & vaccine.current_reducedacquire;
        ar.labelElement("current_reducedtransmit")       & vaccine.current_reducedtransmit;
        ar.labelElement("current_reducedmortality")      & vaccine.current_reducedmortality;
        ar.labelElement("waning_effect")                 & vaccine.waning_effect;
    }
}
