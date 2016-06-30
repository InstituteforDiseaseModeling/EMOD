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
    , vaccine_took(false)
    , waning_effect( nullptr )
    , ivc( nullptr )
    {
        initConfigTypeMap("Cost_To_Consumer", &cost_per_unit, SV_Cost_To_Consumer_DESC_TEXT, 0, 999999, 10.0);
    }

    SimpleVaccine::SimpleVaccine( const SimpleVaccine& master )
    : BaseIntervention( master )
    , parent(nullptr) 
    , vaccine_type(master.vaccine_type)
    , vaccine_take(master.vaccine_take)
    , vaccine_took(master.vaccine_took)
    , waning_config(master.waning_config)
    , waning_effect( nullptr )
    , ivc( nullptr )
    {
        auto tmp_waning = Configuration::CopyFromElement( waning_config._json );
        waning_effect = WaningEffectFactory::CreateInstance( tmp_waning );
        delete tmp_waning;
        tmp_waning = nullptr;
    }

    SimpleVaccine::~SimpleVaccine()
    {
        delete waning_effect;
        waning_effect = nullptr;
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

        // ---------------------------------------------------------------------------------------------
        // --- One does not know if a vaccine 'took' or not unless they go through a specific test
        // --- to see if they have developed the correct antibodies.  Normally, once they have received
        // --- a vaccine, they are considered vaccinated and are counted in the vaccinated group.
        // --- However, if the vaccine did not take, we need to have the vaccine be ineffective.
        // ---------------------------------------------------------------------------------------------
        vaccine_took = ApplyVaccineTake( context->GetParent() );

        bool distribute =  BaseIntervention::Distribute( context, pCCO );
        return distribute;
    }

    void SimpleVaccine::Update( float dt )
    {
        // -----------------------------------------------------------------
        // --- Still update waning_effect even if the vaccine did not take.
        // --- This allows it to expire on schedule.
        // -----------------------------------------------------------------
        waning_effect->Update(dt);

        // ----------------------------------------------------------------------
        // --- If the vaccine did not take, do not attempt to update the vaccine
        // --- behavior within the individual.  
        // ----------------------------------------------------------------------
        if( vaccine_took )
        {
            release_assert(ivc);
            switch( vaccine_type )
            {
                case SimpleVaccineType::AcquisitionBlocking:
                    ivc->UpdateVaccineAcquireRate( waning_effect->Current() );
                    break;

                case SimpleVaccineType::TransmissionBlocking:
                    ivc->UpdateVaccineTransmitRate( waning_effect->Current() );
                    break;

                case SimpleVaccineType::MortalityBlocking:
                    ivc->UpdateVaccineMortalityRate( waning_effect->Current() );
                    break;

                case SimpleVaccineType::Generic:
                    ivc->UpdateVaccineAcquireRate(   waning_effect->Current() );
                    ivc->UpdateVaccineTransmitRate(  waning_effect->Current() );
                    ivc->UpdateVaccineMortalityRate( waning_effect->Current() );
                    break;

                default:
                    throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "vaccine_type", vaccine_type, SimpleVaccineType::pairs::lookup_key( vaccine_type ) );
                    break;
            }
        }

        if( !expired )
        {
            expired = waning_effect->Expired();
        }
    }

    bool SimpleVaccine::ApplyVaccineTake( IIndividualHumanContext* pihc )
    {
        release_assert( pihc );

        bool did_vaccine_take = true;
        if(vaccine_take<1.0)
        {
            if(pihc->GetRng()->e()>vaccine_take)
            {
                LOG_DEBUG("Vaccine did not take.\n");
                did_vaccine_take = false;
            }
        }
        return did_vaccine_take;
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
        ar.labelElement("vaccine_type")  & vaccine.vaccine_type;
        ar.labelElement("vaccine_take")  & vaccine.vaccine_take;
        ar.labelElement("vaccine_took")  & vaccine.vaccine_took;
        ar.labelElement("waning_effect") & vaccine.waning_effect;
    }
}
