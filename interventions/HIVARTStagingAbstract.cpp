
#include "stdafx.h"
#include "HIVARTStagingAbstract.h"

#include "IIndividualHumanHIV.h"
#include "InterventionEnums.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "IHIVInterventionsContainer.h" // for time-date util function and access into IHIVMedicalHistory
#include "IIndividualHumanContext.h"
#include "IdmDateTime.h"

SETUP_LOGGING( "HIVARTStagingAbstract" )

#define DEFAULT_STRING "UNINITIALIZED"

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(HIVARTStagingAbstract, AbstractDecision )
    END_QUERY_INTERFACE_DERIVED(HIVARTStagingAbstract, AbstractDecision )

    HIVARTStagingAbstract::HIVARTStagingAbstract()
    : AbstractDecision( true )
    , ip_tb_value_expected()
    {
        initSimTypes( 1, "HIV_SIM" );
    }

    HIVARTStagingAbstract::HIVARTStagingAbstract( const HIVARTStagingAbstract& master )
        : AbstractDecision( master )
        , ip_tb_value_expected( master.ip_tb_value_expected )
    {
    }

    bool HIVARTStagingAbstract::Configure( const Configuration * inputJson )
    {
        std::string ip_key_str = DEFAULT_STRING ;
        std::string ip_value_str = DEFAULT_STRING ;
        initConfigTypeMap( "Individual_Property_Active_TB_Key", &ip_key_str, HIV_Staging_Individual_Property_Active_TB_Key_DESC_TEXT, DEFAULT_STRING );
        initConfigTypeMap( "Individual_Property_Active_TB_Value", &ip_value_str, HIV_Staging_Individual_Property_Active_TB_Value_DESC_TEXT, DEFAULT_STRING );

        bool ret = AbstractDecision::Configure(inputJson);
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
    bool HIVARTStagingAbstract::MakeDecision( float dt )
    {
        IIndividualHumanHIV * hiv_parent = nullptr;
        if (parent->QueryInterface(GET_IID(IIndividualHumanHIV), (void**)&hiv_parent) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IIndividualHumanHIV", "IIndividualHumanContext" );
        }
        
        IHIVMedicalHistory * med_parent = hiv_parent->GetMedicalHistory();;

        bool has_active_tb = false;
        if( ip_tb_value_expected.IsValid() )
        {
            has_active_tb = parent->GetEventContext()->GetProperties()->Contains( ip_tb_value_expected );
        }

        float year         = parent->GetEventContext()->GetNodeEventContext()->GetTime().Year();
        float CD4count     = med_parent->LowestRecordedCD4();
        bool is_pregnant   = parent->GetEventContext()->IsPregnant() ;

        bool is_positive_decision = MakeDecision( hiv_parent, year, CD4count, has_active_tb, is_pregnant );
        return is_positive_decision;
    }

    // runs on a positive test when in positive treatment fraction
    void HIVARTStagingAbstract::DistributeResultPositive()
    {
        IHIVMedicalHistory * hiv_parent = nullptr;
        if( parent->GetInterventionsContext()->QueryInterface( GET_IID(IHIVMedicalHistory), (void**)&hiv_parent ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IHIVInterventionsContainer", "IIndividualHumanContext" );
        }

        UpdateMedicalHistory( hiv_parent, true );

        // distribute the intervention
        AbstractDecision::DistributeResultPositive();
    }

    // runs on a negative test when in negative treatment fraction
    void HIVARTStagingAbstract::DistributeResultNegative()
    {
        IHIVMedicalHistory * hiv_parent = nullptr;
        if( parent->GetInterventionsContext()->QueryInterface( GET_IID(IHIVMedicalHistory), (void**)&hiv_parent ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "parent", "IHIVInterventionsContainer", "IIndividualHumanContext" );
        }

        UpdateMedicalHistory( hiv_parent, false );

        // distribute the intervention
        AbstractDecision::DistributeResultNegative();
    }

    void HIVARTStagingAbstract::UpdateMedicalHistory( IHIVMedicalHistory *pMedHistory, bool isPositiveTestResult )
    {
        // update the patient's medical chart with testing history
        pMedHistory->OnStageForART(isPositiveTestResult);
    }

    void HIVARTStagingAbstract::serialize(IArchive& ar, HIVARTStagingAbstract* obj)
    {
        AbstractDecision::serialize( ar, obj );
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
