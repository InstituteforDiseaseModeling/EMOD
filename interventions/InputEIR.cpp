/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "InputEIR.h"

#include "Exceptions.h"
#include "InterventionFactory.h"
#include "NodeMalariaEventContext.h"  // for ISporozoiteChallengeConsumer methods
#include "SusceptibilityVector.h"     // for age-dependent biting risk static methods

static const char * _module = "InputEIR";

namespace Kernel
{
    void
    InputEIRConfig::ConfigureFromJsonAndKey(
        const Configuration* inputJson,
        const std::string& key
    )
    {
        // Now's as good a time as any to parse in the calendar schedule.
        json::QuickInterpreter iec_qi( (*inputJson)[key] );
        json::QuickInterpreter scheduleJson( iec_qi.As<json::Array>() );
        release_assert( iec_qi.As<json::Array>().Size() );
        if( iec_qi.As<json::Array>().Size() != MONTHSPERYEAR )
        {
            std::ostringstream msg;
            msg << "InputEIR configuration key " << key << " must be array of size " << MONTHSPERYEAR;
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
        for( unsigned int idx=0; idx<iec_qi.As<json::Array>().Size(); idx++ )
        {
            (*this)[idx] = float(scheduleJson[idx].As<json::Number>());
        }
    }

    json::QuickBuilder
    InputEIRConfig::GetSchema()
    {
        json::QuickBuilder schema( jsonSchemaBase );
        auto tn = JsonConfigurable::_typename_label();
        auto ts = JsonConfigurable::_typeschema_label();
        schema[ tn ] = json::String( "idmType:InputEIRConfig" );
    
        schema[ts] = json::Array();
        schema[ts][0] = json::Object();
        schema[ts][0][ "type" ] = json::String( "float" );
        schema[ts][0][ "min" ] = json::Number( 0 );
        schema[ts][0][ "max" ] = json::Number( 1000.0f );
        schema[ts][0][ "description" ] = json::String( IE_Monthly_EIR_DESC_TEXT );
        return schema;
    }

    BEGIN_QUERY_INTERFACE_BODY(InputEIR)
        HANDLE_INTERFACE(IConfigurable)
        HANDLE_INTERFACE(INodeDistributableIntervention)
        HANDLE_ISUPPORTS_VIA(INodeDistributableIntervention)
    END_QUERY_INTERFACE_BODY(InputEIR)

    IMPLEMENT_FACTORY_REGISTERED(InputEIR)

    QuickBuilder InputEIR::GetSchema()
    {
        return QuickBuilder( jsonSchemaBase );
    }

    InputEIR::InputEIR() 
        : today(0)
        , daily_EIR(0.0f)
        , risk_function(NULL)
    {
        initSimTypes( 1, "MALARIA_SIM" ); // using sporozoite challenge
    }

    bool InputEIR::Configure( const Configuration * inputJson )
    {
        initConfig( "Age_Dependence", age_dependence, inputJson, MetadataDescriptor::Enum("Age_Dependence", IE_Age_Dependence_DESC_TEXT, MDD_ENUM_ARGS(AgeDependentBitingRisk)) );
        initConfigComplexType( "Monthly_EIR", &monthly_EIR, IE_Config_Type_DESC_TEXT);

        switch(age_dependence)
        {
            case AgeDependentBitingRisk::OFF:
                risk_function=NULL;
                break;

            case AgeDependentBitingRisk::LINEAR:
                risk_function = SusceptibilityVector::LinearBitingFunction;
                break;

            case AgeDependentBitingRisk::SURFACE_AREA_DEPENDENT:
                risk_function = SusceptibilityVector::SurfaceAreaBitingFunction;
                break;

            default:
                if( !JsonConfigurable::_dryrun )
                {
                    throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, 
                        "age_dependence", age_dependence, 
                        AgeDependentBitingRisk::pairs::lookup_key(age_dependence) );
                }
        }

        bool configured = JsonConfigurable::Configure( inputJson );

        if(monthly_EIR.size() != MONTHSPERYEAR && JsonConfigurable::_dryrun == false )
        {
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, 
                "'Monthly_EIR' parameterizes the mean number of infectious bites experienced by an individual for each month of the year.  As such, it must be an array of EXACTLY length 12." );
        }

        return configured;
    }

    void InputEIR::SetContextTo(INodeEventContext *context)
    {
        parent = context;
    }

    void InputEIR::Update( float dt )
    {
        daily_EIR = monthly_EIR.at( (today / IDEALDAYSPERMONTH) % MONTHSPERYEAR ) / IDEALDAYSPERMONTH;
        today++;
        LOG_DEBUG_F("Day = %d, annualized EIR = %0.2f\n", today, 365*daily_EIR);

        ISporozoiteChallengeConsumer *iscc;
        if (s_OK == parent->QueryInterface(GET_IID(ISporozoiteChallengeConsumer), (void**)&iscc))
        {
            iscc->ChallengeWithInfectiousBites(1, daily_EIR, risk_function);
        }
        else
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "iscc", "ISporozoiteChallengeConsumer", "INodeEventContext" );
        }    
    }
}
