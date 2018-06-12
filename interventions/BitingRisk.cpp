/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "BitingRisk.h"
#include "Contexts.h"
#include "VectorInterventionsContainerContexts.h"

SETUP_LOGGING( "BitingRisk" )

namespace Kernel
{
    IMPLEMENT_FACTORY_REGISTERED( BitingRisk )

    BEGIN_QUERY_INTERFACE_BODY( BitingRisk )
        HANDLE_INTERFACE( IConfigurable )
        HANDLE_INTERFACE( IDistributableIntervention )
        HANDLE_ISUPPORTS_VIA( IDistributableIntervention )
    END_QUERY_INTERFACE_BODY( BitingRisk )

        BitingRisk::BitingRisk()
        : BaseIntervention()
        , m_IBitingRisk( nullptr )
        , m_Distribution( DistributionFunction::FIXED_DURATION )
    {
        // ??????????????
        // ??? DENGUE_SIM
        // ??????????????
        initSimTypes( 2, "VECTOR_SIM", "MALARIA_SIM" );
    }

    BitingRisk::BitingRisk( const BitingRisk& master )
        : BaseIntervention( master )
        , m_IBitingRisk( nullptr )
        , m_Distribution( master.m_Distribution )
    {
    }

    BitingRisk::~BitingRisk()
    {
    }

    bool BitingRisk::Configure( const Configuration * inputJson )
    {
        m_Distribution.SetTypeNameDesc( "Risk_Distribution_Type", BR_Risk_Distribution_Type_DESC_TEXT );
        m_Distribution.AddSupportedType( DistributionFunction::FIXED_DURATION,       "Constant",         BR_Constant_DESC_TEXT,         "", "" );
        m_Distribution.AddSupportedType( DistributionFunction::UNIFORM_DURATION,     "Uniform_Min",      BR_Uniform_Min_DESC_TEXT,      "Uniform_Max",      BR_Uniform_Max_DESC_TEXT );
        m_Distribution.AddSupportedType( DistributionFunction::GAUSSIAN_DURATION,    "Gaussian_Mean",    BR_Gaussian_Mean_DESC_TEXT,    "Gaussian_Std_Dev", BR_Gaussian_Std_Dev_DESC_TEXT );
        m_Distribution.AddSupportedType( DistributionFunction::EXPONENTIAL_DURATION, "Exponential_Mean", BR_Exponential_Mean_DESC_TEXT, "", "" );

        m_Distribution.Configure( this, inputJson );

        bool configured = BaseIntervention::Configure( inputJson );

        if( !JsonConfigurable::_dryrun && configured )
        {
            m_Distribution.CheckConfiguration();
        }
        return configured;
    }

    bool BitingRisk::Distribute( IIndividualHumanInterventionsContext *context,
                                 ICampaignCostObserver * const pCCO )
    {
        if( AbortDueToDisqualifyingInterventionStatus( context->GetParent() ) )
        {
            return false;
        }

        bool distributed = BaseIntervention::Distribute( context, pCCO );
        if( distributed )
        {
            if( s_OK != context->QueryInterface( GET_IID( IBitingRisk ), (void**)&m_IBitingRisk ) )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context", "IBitingRisk", "IIndividualHumanInterventionsContext" );
            }
        }
        return distributed;
    }

    void BitingRisk::SetContextTo( IIndividualHumanContext *context )
    {
        BaseIntervention::SetContextTo( context );

        if( s_OK != context->GetInterventionsContext()->QueryInterface( GET_IID( IBitingRisk ), (void**)&m_IBitingRisk ) )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "context->GetInterventionsContext()", "IBitingRisk", "IIndividualHumanInterventionsContext" );
        }
    }

    void BitingRisk::Update( float dt )
    {
        if( !BaseIntervention::UpdateIndividualsInterventionStatus() ) return;

        float rate = m_Distribution.CalculateDuration();
        m_IBitingRisk->UpdateRelativeBitingRate( rate );
        expired = true;
    }

    REGISTER_SERIALIZABLE( BitingRisk );

    void BitingRisk::serialize( IArchive& ar, BitingRisk* obj )
    {
        BaseIntervention::serialize( ar, obj );
        BitingRisk& biting_risk = *obj;
        ar.labelElement( "m_Distribution" ) & biting_risk.m_Distribution;
    }
}
