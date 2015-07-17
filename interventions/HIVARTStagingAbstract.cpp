/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "HIVARTStagingAbstract.h"

#include "InfectionHIV.h"
#include "IIndividualHumanHIV.h"
#include "InterventionEnums.h"
#include "InterventionFactory.h"
#include "NodeEventContext.h"  // for INodeEventContext (ICampaignCostObserver)
#include "HIVInterventionsContainer.h" // for time-date util function and access into IHIVCascadeOfCare and IHIVMedicalHistory
#include "Relationship.h"   // for discordant checking

static const char * _module = "HIVARTStagingAbstract";

#define DEFAULT_STRING "UNINITIALIZED"

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(HIVARTStagingAbstract, HIVSimpleDiagnostic)
    END_QUERY_INTERFACE_DERIVED(HIVARTStagingAbstract, HIVSimpleDiagnostic)

    HIVARTStagingAbstract::HIVARTStagingAbstract()
    : HIVSimpleDiagnostic()
    , ip_tb_key( DEFAULT_STRING )
    , ip_tb_value_expected( DEFAULT_STRING )
    {
        initConfigTypeMap( "Individual_Property_Active_TB_Key", &ip_tb_key, HIV_Staging_Individual_Property_Active_TB_Key_DESC_TEXT, DEFAULT_STRING );
        initConfigTypeMap( "Individual_Property_Active_TB_Value", &ip_tb_value_expected, HIV_Staging_Individual_Property_Active_TB_Value_DESC_TEXT, DEFAULT_STRING );
    }

    HIVARTStagingAbstract::HIVARTStagingAbstract( const HIVARTStagingAbstract& master )
        : HIVSimpleDiagnostic( master )
        , ip_tb_key( master.ip_tb_key )
        , ip_tb_value_expected( master.ip_tb_value_expected )
    {
    }

    bool HIVARTStagingAbstract::Configure( const Configuration * inputJson )
    {
        bool ret = HIVSimpleDiagnostic::Configure(inputJson);
        if( ret && !JsonConfigurable::_dryrun )
        {
            if( ((ip_tb_key != DEFAULT_STRING) && (ip_tb_value_expected == DEFAULT_STRING)) ||
                ((ip_tb_key == DEFAULT_STRING) && (ip_tb_value_expected != DEFAULT_STRING)) )
            {
                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, 
                                                       "Individual_Property_Active_TB_Key", ip_tb_key.c_str(),
                                                       "Individual_Property_Active_TB_Value", ip_tb_value_expected.c_str(),
                                                       "You must define both parameters." );
            }
        }

        return ret ;
    }

    bool HIVARTStagingAbstract::Distribute( IIndividualHumanInterventionsContext *context,
                                            ICampaignCostObserver * const pICCO )
    {
        if( (ip_tb_key != DEFAULT_STRING) && (ip_tb_value_expected != DEFAULT_STRING) )
        {
            // if the user defines both parameters, then we assume that they also
            // defined the properties in the demographcis
            Node::VerifyPropertyDefinedInDemographics( ip_tb_key, ip_tb_value_expected );
        }
        return HIVSimpleDiagnostic::Distribute( context, pICCO );
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

        // ---------------------------------------------------------
        // --- Get the value of the IndividualProperty if it exists.
        // ---------------------------------------------------------
        std::string ip_tb_value_actual = "<NA>" ;
        const auto* p_props = parent->GetEventContext()->GetProperties();
        if( p_props->find( ip_tb_key ) != p_props->end() )
        {
            ip_tb_value_actual = p_props->at( ip_tb_key );
        }

        float year         = parent->GetEventContext()->GetNodeEventContext()->GetTime().Year();
        float CD4count     = med_parent->LowestRecordedCD4();
        bool has_active_tb = (ip_tb_value_actual == ip_tb_value_expected) ;
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

}


#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::HIVARTStagingAbstract)

namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, HIVARTStagingAbstract& obj, const unsigned int v)
    {
        static const char * _module = "HIVARTStagingAbstract";
        LOG_DEBUG("(De)serializing HIVARTStagingAbstract\n");

        boost::serialization::void_cast_register<HIVARTStagingAbstract, IDistributableIntervention>();
        //ar & obj.abortStates;     // todo: serialize this!
        ar & obj.cascadeState;
        ar & obj.firstUpdate;
        ar & boost::serialization::base_object<Kernel::HIVSimpleDiagnostic>(obj);
    }
    template void serialize( boost::mpi::packed_skeleton_iarchive&, Kernel::HIVARTStagingAbstract&, unsigned int);
}
#endif
