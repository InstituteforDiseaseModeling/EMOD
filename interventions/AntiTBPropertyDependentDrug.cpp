/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#ifdef ENABLE_TB

#include "AntiTBPropertyDependentDrug.h"
#include "Contexts.h"
#include "IndividualEventContext.h"
#include "TBContexts.h"
#include "Debug.h"                        // for release-assert
#include "SimulationConfig.h"
#include "TBParameters.h"
#include "TBDrugTypeParameters.h" //for TBDrugTypes

static const char* _module = "AntiTBPropDepDrug";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(AntiTBPropDepDrug, AntiTBDrug)
    END_QUERY_INTERFACE_DERIVED(AntiTBPropDepDrug, AntiTBDrug)

    IMPLEMENT_FACTORY_REGISTERED(AntiTBPropDepDrug)

    void
    DrugTypeByProperty::ConfigureFromJsonAndKey(
        const Configuration* inputJson, const std::string& key
    )
    {
        // read array of json objects into string-to-string map.
        try {
            json::QuickInterpreter s2sarray = (*inputJson)[key].As<json::Array>();
            for( int idx=0; idx < (*inputJson)[key].As<json::Array>().Size(); idx++ )
            {
                try {
                    auto json_map = s2sarray[idx].As<json::Object>();
                    for( auto data = json_map.Begin();
                            data != json_map.End();
                            ++data )
                    {
                        std::string drug_key = data->name;
                        try {
                            std::string value = (std::string)s2sarray[idx][drug_key].As< json::String >();
                            prop2drugMap.insert( std::make_pair( drug_key, value ) );
                        }
                        catch( const json::Exception & )
                        {
                            throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, drug_key.c_str(), s2sarray[idx], "Expected STRING" );
                        }
                    }
                }
                catch( const json::Exception & )
                {
                    throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, key.c_str(), s2sarray[idx], "Expected OBJECT" );
                }
            }
        }
        catch( const json::Exception & )
        {
            throw Kernel::JsonTypeConfigurationException( __FILE__, __LINE__, __FUNCTION__, key.c_str(), (*inputJson), "Expected ARRAY" );
        }
        
    }

    json::QuickBuilder
    DrugTypeByProperty::GetSchema()
    {
        json::QuickBuilder schema( GetSchemaBase() );
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();
        schema[ tn ] = json::String( "idmType:DrugTypeByProperty" );
        schema[ ts ] = json::Array();
        schema[ ts ][0] = json::Object();
        // Should the value be a constrained string?
        schema[ ts ][0][ "<demographics>::Defaults.Individual_Properties.*.Property.<keys>:<demographics>::Defaults.Individual_Properties.*.Value.<keys>" ] = json::String( "<config>::TB_Drug_Params" );
        return schema;
    }

    AntiTBPropDepDrug::AntiTBPropDepDrug()
    : AntiTBDrug()
    , enable_state_specific_tx(true)
    {
        initSimTypes( 1, "TB_SIM" );
    }

    AntiTBPropDepDrug::~AntiTBPropDepDrug()
    { 
        //delete drug_type_by_property;
    }

    bool
    AntiTBPropDepDrug::Configure(
        const Configuration * inputJson
    )
    {
        //drug_type = TBDrugType::PropDepDrug;

        initConfigTypeMap( "Enable_State_Specific_Treatment", &enable_state_specific_tx, TB_PropDepDrug_Enable_State_Specific_Treatment_DESC_TEXT, true );
        initConfigComplexType( "Drug_Type_by_Property", &drug_type_by_property, TB_PropDepDrug_Drug_Type_by_Property_DESC_TEXT);
        
        return AntiTBDrug::Configure( inputJson );
    }

    void AntiTBPropDepDrug::ConfigureDrugTreatment( IIndividualHumanInterventionsContext * ivc )  
    {
        IIndividualHumanTB2* tb_patient = nullptr;
        if ( ivc->GetParent()->QueryInterface( GET_IID(IIndividualHumanTB2), (void**) &tb_patient ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IIndvidualHumanTB2", "IndividualHuman" );
        }
        
        tProperties* pProp = ivc->GetParent()->GetEventContext()->GetProperties();

        auto tbdtMap = GET_CONFIGURABLE(SimulationConfig)->tb_params->TBDrugMap;
        for (auto& pair : (*pProp))
        {
            const std::string& propkey = pair.first;
            const std::string& propvalue = pair.second;
            std::string drugtypekey = propkey + ":" + propvalue;
            std::string drugtypekeyFORMATLAB = propkey + "_" + propvalue;

            //    "Drug_Type_by_Property": {
            //        "QualityOfCare:CDC":"CDCDrug",
            //        "QualityOfCare:Hospital": "HospitalDrug"
            //    },

            if( drug_type_by_property.prop2drugMap.count(drugtypekey) || drug_type_by_property.prop2drugMap.count(drugtypekeyFORMATLAB) )
            {
                //string-ify the drug that was typed in to the campaign file
                std::string conditional_drug_type = drug_type_by_property.prop2drugMap.at(drugtypekey);

                //the primary_decay_time_constant is a generic drug parameter so it can't just be read in from TB_Drug_Params in the config
                
                current_reducedtransmit = 1.0;

                std::string all_cond_drug_type = conditional_drug_type;
                //Auto-adjust drug parameters ONLY at configure for MDR evolution OR for Retx (drug sensitive only), read in the configured params
                if (enable_state_specific_tx == true)
                {    
                    LOG_DEBUG("Inside enable_state_specific_tx \n");
                    release_assert(tb_patient != nullptr);
                    if (tb_patient->IsMDR())
                    {
                        all_cond_drug_type = all_cond_drug_type + "MDR";

                    }
                    else
                    {
                        if( !tb_patient->IsTreatmentNaive() )
                        {
                            all_cond_drug_type = all_cond_drug_type + "Retx";
                            LOG_DEBUG("Person has failed or relapsed in the past, they get a retx drug \n");
                        }
                    }
                }

                LOG_DEBUG_F("Read in the tbdt map, the drug type is %s \n", all_cond_drug_type.c_str());
                if (tbdtMap[all_cond_drug_type] == nullptr)
                {
                    throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "You used a drug which is not in the TB_Drug_Types_For_This_Sim." );
                }
                release_assert( tbdtMap[all_cond_drug_type] );

                fast_decay_time_constant = tbdtMap[all_cond_drug_type]->TB_drug_primary_decay_time_constant;

                LOG_DEBUG_F( "Using fast_decay_time_constant value of %f\n", fast_decay_time_constant );

                TB_drug_inactivation_rate = tbdtMap[all_cond_drug_type]->TB_drug_inactivation_rate;
                TB_drug_clearance_rate = tbdtMap[all_cond_drug_type]->TB_drug_clearance_rate;
                TB_drug_resistance_rate = tbdtMap[all_cond_drug_type]->TB_drug_resistance_rate;
                TB_drug_relapse_rate = tbdtMap[all_cond_drug_type]->TB_drug_relapse_rate;
                TB_drug_mortality_rate = tbdtMap[all_cond_drug_type]->TB_drug_mortality_rate;
                LOG_DEBUG_F("Finished reading in the map: TB_drug_clearance_rate = %f.\n", TB_drug_clearance_rate );

                //in base class AntiTBDrug, sets current_efficacy to 1 and broadcast that you are starting an intervention
                return AntiTBDrug::ConfigureDrugTreatment( ivc ); 
            }
        }
    }

    REGISTER_SERIALIZABLE(AntiTBPropDepDrug);

    void AntiTBPropDepDrug::serialize(IArchive& ar, AntiTBPropDepDrug* obj)
    {
        AntiTBDrug::serialize(ar, obj);
        AntiTBPropDepDrug& drug = *obj;
        ar.labelElement("drug_type_by_property") & drug.drug_type_by_property.prop2drugMap;
        ar.labelElement("enable_state_specific_tx") & drug.enable_state_specific_tx;
    }
}

#endif
