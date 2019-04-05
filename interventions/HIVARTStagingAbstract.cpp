/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "HIVARTStagingAbstract.h"

#include "InfectionHIV.h"
#include "IIndividualHumanHIV.h"
#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "IHIVInterventionsContainer.h" // for time-date util function and access into IHIVMedicalHistory
#include "Relationship.h"   // for discordant checking
#include "IIndividualHumanContext.h"
#include "IdmDateTime.h"

SETUP_LOGGING( "HIVARTStagingAbstract" )

#define DEFAULT_STRING "UNINITIALIZED"

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(HIVARTStagingAbstract, HIVSimpleDiagnostic)
    END_QUERY_INTERFACE_DERIVED(HIVARTStagingAbstract, HIVSimpleDiagnostic)

    HIVARTStagingAbstract::HIVARTStagingAbstract()
    : HIVSimpleDiagnostic()
    , ip_tb_value_expected()
    {
    }

    HIVARTStagingAbstract::HIVARTStagingAbstract( const HIVARTStagingAbstract& master )
        : HIVSimpleDiagnostic( master )
        , ip_tb_value_expected( master.ip_tb_value_expected )
    {
    }

    bool HIVARTStagingAbstract::Configure( const Configuration * inputJson )
    {
        std::string ip_key_str = DEFAULT_STRING ;
        std::string ip_value_str = DEFAULT_STRING ;
        initConfigTypeMap( "Individual_Property_Active_TB_Key", &ip_key_str, HIV_Staging_Individual_Property_Active_TB_Key_DESC_TEXT, DEFAULT_STRING );
        initConfigTypeMap( "Individual_Property_Active_TB_Value", &ip_value_str, HIV_Staging_Individual_Property_Active_TB_Value_DESC_TEXT, DEFAULT_STRING );
        bool ret = HIVSimpleDiagnostic::Configure(inputJson);
        if( ret && !JsonConfigurable::_dryrun )
        {
            if( ((ip_key_str != DEFAULT_STRING) && (ip_value_str == DEFAULT_STRING)) ||
                ((ip_key_str == DEFAULT_STRING) && (ip_value_str != DEFAULT_STRING)) )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, 
                                                       "Individual_Property_Active_TB_Key", ip_key_str.c_str(),
                                                       "Individual_Property_Active_TB_Value", ip_value_str.c_str(),
                                                       "You must define both parameters." );
            }
            else if( (ip_key_str != DEFAULT_STRING) && (ip_value_str != DEFAULT_STRING) )
            {
                ip_tb_value_expected = IPKeyValue( ip_key_str, ip_value_str ) ;
            }
        }

        return ret ;
    }

    // staged for ART via CD4 agnostic testing?
    bool HIVARTStagingAbstract::positiveTestResult()
    {
        IIndividualHumanHIV * hiv_parent = nullptr;
        if (parent->QueryInterface(GET_IID(IIndividualHumanHIV), (void**)&hiv_parent) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanHIV", "IIndividualHumanContext" );
        }
        
        IHIVMedicalHistory * med_parent = nullptr;
        if (parent->GetInterventionsContext()->QueryInterface(GET_IID(IHIVMedicalHistory), (void**)&med_parent) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IHIVMedicalChart", "IIndividualHumanContext" );
        }

        bool has_active_tb = false;
        if( ip_tb_value_expected.IsValid() )
        {
            has_active_tb = parent->GetEventContext()->GetProperties()->Contains( ip_tb_value_expected );
        }

        float year         = parent->GetEventContext()->GetNodeEventContext()->GetTime().Year();
        float CD4count     = med_parent->LowestRecordedCD4();
        bool is_pregnant   = parent->GetEventContext()->IsPregnant() ;

        bool result = positiveTestResult( hiv_parent, year, CD4count, has_active_tb, is_pregnant );
        return result;
    }

    // runs on a positive test when in positive treatment fraction
    void HIVARTStagingAbstract::positiveTestDistribute()
    {
        IHIVMedicalHistory * hiv_parent = nullptr;
        if( parent->GetInterventionsContext()->QueryInterface( GET_IID(IHIVMedicalHistory), (void**)&hiv_parent ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IHIVInterventionsContainer", "IIndividualHumanContext" );
        }

        UpdateMedicalHistory( hiv_parent, true );

        // distribute the intervention
        HIVSimpleDiagnostic::positiveTestDistribute();
    }

    // runs on a negative test when in negative treatment fraction
    void HIVARTStagingAbstract::onNegativeTestResult()
    {
        IHIVMedicalHistory * hiv_parent = nullptr;
        if( parent->GetInterventionsContext()->QueryInterface( GET_IID(IHIVMedicalHistory), (void**)&hiv_parent ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IHIVInterventionsContainer", "IIndividualHumanContext" );
        }

        UpdateMedicalHistory( hiv_parent, false );

        // distribute the intervention
        HIVSimpleDiagnostic::onNegativeTestResult();
    }

    void HIVARTStagingAbstract::UpdateMedicalHistory( IHIVMedicalHistory *pMedHistory, bool isPositiveTestResult )
    {
        // update the patient's medical chart with testing history
        pMedHistory->OnStageForART(isPositiveTestResult);
    }

    void HIVARTStagingAbstract::serialize(IArchive& ar, HIVARTStagingAbstract* obj)
    {
        HIVSimpleDiagnostic::serialize( ar, obj );
        HIVARTStagingAbstract& art = *obj;

        std::string key_value;
        if( ar.IsWriter() )
        {
            key_value = art.ip_tb_value_expected.ToString();
        }

        //ar.labelElement("ip_tb_value_expected") & art.ip_tb_value_expected;
        ar.labelElement("ip_tb_value_expected") & key_value;

        if( ar.IsReader() )
        {
            art.ip_tb_value_expected = IPKeyValue( key_value );
        }
    }
}
