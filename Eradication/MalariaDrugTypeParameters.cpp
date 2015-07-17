/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "MalariaDrugTypeParameters.h"

#include "Common.h"
#include "Exceptions.h"
#include <algorithm> // for transform
#include <cctype> // for tolower

static const char * _module = "DTP";

Kernel::MalariaDrugTypeParameters::tMDTPMap Kernel::MalariaDrugTypeParameters::_mdtMap;

namespace Kernel
{
    void
    DoseMap::ConfigureFromJsonAndKey(
        const Configuration * inputJson,
        const std::string& key
    )
    {
        const Array dose_by_age = (*inputJson)[key].As<Array>();
        std::ostringstream oss;
        oss << "<drugType>" << ": fraction of adult dose\n";
        for( int i=0; i<dose_by_age.Size(); i++)
        {
            QuickInterpreter dosing(dose_by_age[i]);
            float upper_age_in_years = dosing["Upper_Age_In_Years"].As<Number>();
            float fractional_dose = dosing["Fraction_Of_Adult_Dose"].As<Number>();
            fractional_dose_by_upper_age[upper_age_in_years] = fractional_dose;
            oss << "under " << int(upper_age_in_years) << ", " << fractional_dose << "\n";
        }
        LOG_DEBUG_F(oss.str().c_str()); 
    }

    json::QuickBuilder
    DoseMap::GetSchema()
    {
        // maybe put this type into central "complex-types" section and define variable as this type?
        json::QuickBuilder schema( jsonSchemaBase );
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();
        schema[ tn ] = json::String( "idmType:DoseMap" );
        schema[ ts ] = json::Array();
        schema[ ts ][0] = json::Object();
        schema[ ts ][0][ "Upper_Age_In_Years" ] = json::Object();
        schema[ ts ][0][ "Upper_Age_In_Years" ][ "type" ] = json::String( "float" );
        schema[ ts ][0][ "Upper_Age_In_Years" ][ "min" ] = json::Number( 0 );
        schema[ ts ][0][ "Upper_Age_In_Years" ][ "max" ] = json::Number( MAX_HUMAN_AGE );
        schema[ ts ][0][ "Upper_Age_In_Years" ][ "description" ] = json::String( DM_Upper_Age_In_Years_DESC_TEXT );
        schema[ ts ][0][ "Fraction_Of_Adult_Dose"] = json::Object();
        schema[ ts ][0][ "Fraction_Of_Adult_Dose" ][ "type" ] = json::String( "float" );
        schema[ ts ][0][ "Fraction_Of_Adult_Dose" ][ "min" ] = json::Number( 0 );
        schema[ ts ][0][ "Fraction_Of_Adult_Dose" ][ "max" ] = json::Number( 1.0 );
        schema[ ts ][0][ "Fraction_Of_Adult_Dose" ][ "description" ] = json::String( DM_Fraction_Of_Adult_Dose_DESC_TEXT );
        return schema;
    }

    MalariaDrugTypeParameters::MalariaDrugTypeParameters( const std::string &drugType )
        : bodyweight_exponent(0.0f)
    { 
        LOG_DEBUG_F( "ctor: drugType = %s\n", drugType.c_str() );
        initConfigTypeMap( "Max_Drug_IRBC_Kill", &max_drug_IRBC_kill, Max_Drug_IRBC_Kill_DESC_TEXT, 0.0f, 100000.0f, 5.0f );
        initConfigTypeMap( "Drug_Hepatocyte_Killrate", &drug_hepatocyte_killrate, Drug_Hepatocyte_Killrate_DESC_TEXT, 0.0f, 100000.0f, 0.0f );
        initConfigTypeMap( "Drug_Gametocyte02_Killrate", &drug_gametocyte02_killrate, Drug_Gametocyte02_Killrate_DESC_TEXT, 0.0f, 100000.0f, 0.0f );
        initConfigTypeMap( "Drug_Gametocyte34_Killrate", &drug_gametocyte34_killrate, Drug_Gametocyte34_Killrate_DESC_TEXT, 0.0f, 100000.0f, 0.0f );
        initConfigTypeMap( "Drug_GametocyteM_Killrate", &drug_gametocyteM_killrate, Drug_GametocyteM_Killrate_DESC_TEXT, 0.0f, 100000.0f, 0.0f );
        initConfigTypeMap( "Drug_PKPD_C50", &drug_pkpd_c50, Drug_PKPD_C50_DESC_TEXT, 0.0f, 100000.0f, 100.0f );
        initConfigTypeMap( "Drug_Cmax", &drug_Cmax, Drug_Cmax_DESC_TEXT, 0.0f, 100000.0f, 1000.0f );
        initConfigTypeMap( "Drug_Vd", &drug_Vd, Drug_Vd_DESC_TEXT, 0.0f, 100000.0f, 10.0f );
        initConfigTypeMap( "Drug_Decay_T1", &drug_decay_T1, Drug_Decay_T1_DESC_TEXT, 0.0f, 100000.0f, 1.0f );
        initConfigTypeMap( "Drug_Decay_T2", &drug_decay_T2, Drug_Decay_T2_DESC_TEXT, 0.0f, 100000.0f, 1.0f );
        initConfigTypeMap( "Drug_Fulltreatment_Doses", &drug_fulltreatment_doses, Drug_Fulltreatment_Doses_DESC_TEXT, 1, 100000, 3 );
        initConfigTypeMap( "Drug_Dose_Interval", &drug_dose_interval, Drug_Dose_Interval_DESC_TEXT, 0.0f, 100000.0f, 1.0f );

        // was optionally parsed, but that was a little heterodox: age-dependent dosing specifications
        initConfigTypeMap( "Bodyweight_Exponent", &bodyweight_exponent, DRUG_Bodyweight_Exponent_DEST_TEXT, 0.0f, 100000.0f, 0.0f );
    }

    MalariaDrugTypeParameters::~MalariaDrugTypeParameters()
    { LOG_DEBUG( "dtor\n" );
    }

    MalariaDrugTypeParameters* MalariaDrugTypeParameters::CreateMalariaDrugTypeParameters(
        const std::string &drugType
    )
    { 
        LOG_DEBUG( "Create\n" );

        // The MDTP construct exists to cache once-per-simulation values of one or more drugs.
        // Until there can be different parameters for the same drug in a single simulation,
        // at which point we would probably just make those configurable properties of AntimalarialDrug itself,
        // there is no need to keep leaking memory here.

        MalariaDrugTypeParameters* params = NULL;
        tMDTPMap::const_iterator itMap = _mdtMap.find(drugType);

        if ( itMap == _mdtMap.end() )
        {
            params = _new_ MalariaDrugTypeParameters( drugType );
            params->Initialize(drugType);
            if( !JsonConfigurable::_dryrun )
            {
                try
                {
                    Configuration* drug_config = Configuration::CopyFromElement( (*EnvPtr->Config)["Malaria_Drug_Params"][drugType] );
                    params->Configure( drug_config );

                    // Check validity of dosing regimen
                    float sim_tstep = (*EnvPtr->Config)["Simulation_Timestep"].As<Number>();
                    float updates_per_tstep = (*EnvPtr->Config)["Infection_Updates_Per_Timestep"].As<Number>();
                    if ( params->drug_dose_interval < sim_tstep/updates_per_tstep )
                    {
                        std::ostringstream oss;
                        oss << "time_between_doses (" << params->drug_dose_interval << ") is less than dt (" << sim_tstep/updates_per_tstep << ")" << std::endl;
                        throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, oss.str().c_str() );
                    }

                }
                catch(json::Exception &e)
                {
                    // Exception getting parameter block for drug of type "drugType" from config.json
                    throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, e.what() ); 
                }
            }
            _mdtMap[ drugType ] = params;
            return params;
        }
        else
        {
            return itMap->second;
        }
    }

    bool
    MalariaDrugTypeParameters::Configure(
        const ::Configuration *config
    )
    {
        LOG_DEBUG( "Configure\n" );

        if( config->Exist( "Fractional_Dose_By_Upper_Age" ) || JsonConfigurable::_dryrun ) // :(
        {
            initConfigComplexType( "Fractional_Dose_By_Upper_Age", &dose_map, DM_Fraction_Of_Adult_Dose_By_Year_Array_DESC_TEXT );
        }

        return JsonConfigurable::Configure( config );
    }

    QueryResult
    MalariaDrugTypeParameters::QueryInterface(
        iid_t iid, void **ppvObject
    )
    {
        throw NotYetImplementedException(  __FILE__, __LINE__, __FUNCTION__ );
    }

    void MalariaDrugTypeParameters::Initialize(const std::string &drugType)
    {
        LOG_DEBUG_F( "MalariaDrugTypeParameters::Initialize: drug type = %s\n", drugType.c_str() );

        int eDrug = MalariaDrugType::pairs::lookup_value(drugType.c_str());

        if ( eDrug < 0 )
        {
            LOG_WARN_F("Anti-malarial drug name in Malaria_Drug_Params block (%s) is not one of the standard cases.\n", drugType.c_str());
        }

        _drugType = drugType;
    }

}

