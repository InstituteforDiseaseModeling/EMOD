/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/


#include "stdafx.h"

#include "ReportNodeDemographics.h"
#include "DllInterfaceHelper.h"
#include "FactorySupport.h"

#include "NodeEventContext.h"
#include "IIndividualHuman.h"
#include "ReportUtilities.h"
#include "Properties.h"
#include "NodeProperties.h"

SETUP_LOGGING( "ReportNodeDemographics" )

namespace Kernel
{
// ----------------------------------------
// --- ReportNodeDemographics Methods
// ----------------------------------------

    ReportNodeDemographics::ReportNodeDemographics()
        : ReportNodeDemographics( "ReportNodeDemographics.csv" )
    {
    }

    ReportNodeDemographics::ReportNodeDemographics( const std::string& rReportName )
        : BaseTextReport( rReportName )
        , m_StratifyByGender(true)
        , m_StratifyByAge(true)
        , m_AgeYears()
        , m_IPKeyToCollect()
        , m_IPValuesList()
        , m_Data()
    {
        // ------------------------------------------------------------------------------------------------
        // --- Since this report will be listening for events, it needs to increment its reference count
        // --- so that it is 1.  Other objects will be AddRef'ing and Release'ing this report/observer
        // --- so it needs to start with a refcount of 1.
        // ------------------------------------------------------------------------------------------------
        AddRef();
    }

    ReportNodeDemographics::~ReportNodeDemographics()
    {
        for( int i = 0; i < m_Data.size(); ++i )
        {
            for( int j = 0; j < m_Data[ i ].size(); ++j )
            {
                for( int k = 0; k < m_Data[ i ][ j ].size(); ++k )
                {
                    delete m_Data[ i ][ j ][ k ];
                    m_Data[ i ][ j ][ k ] = nullptr;
                }
            }
        }
    }

    bool ReportNodeDemographics::Configure( const Configuration * inputJson )
    {
        initConfigTypeMap( "IP_Key_To_Collect", &m_IPKeyToCollect, "Name of the key to add a column for.", "" );
        if( inputJson->Exist("Age_Bins") )
        {
            initConfigTypeMap("Age_Bins", &m_AgeYears, "Age Bins (in years) to aggregate within and report, empty array implies do not stratify by age");
        }
        else
        {
            m_AgeYears.push_back( 40.0 );
            m_AgeYears.push_back( 80.0 );
            m_AgeYears.push_back( 125.0 );
        }
        initConfigTypeMap( "Stratify_By_Gender", &m_StratifyByGender, "1 (default) implies stratify by gender, 0 implies do not", true );
        bool ret = JsonConfigurable::Configure( inputJson );
        
        if( ret )
        {
        }
        return ret;
    }

    NodeData* ReportNodeDemographics::CreateNodeData()
    {
        return new NodeData();
    }

    void ReportNodeDemographics::Initialize( unsigned int nrmSize )
    {
        if( !m_IPKeyToCollect.empty() )
        {
            std::list<std::string> ip_value_list = IPFactory::GetInstance()->GetIP( m_IPKeyToCollect )->GetValues<IPKeyValueContainer>().GetValuesToList();
            for( auto val : ip_value_list )
            {
                m_IPValuesList.push_back( val );
            }
        }
        else
        {
            m_IPValuesList.push_back( "<no ip>" ); // need at least one in the list
        }

        if( m_AgeYears.size() == 0 ) // implies don't stratify by years
        {
            m_AgeYears.push_back( 125.0 );
            m_StratifyByAge = false;
        }

        int num_genders = m_StratifyByGender ? 2 : 1; // 1 implies not stratifying by gender

        // initialize the counters so that they can be indexed by gender and age
        for( int g = 0 ; g < num_genders ; g++ )
        {
            m_Data.push_back( std::vector<std::vector<NodeData*>>() );
            for( int a = 0 ; a < m_AgeYears.size() ; a++ )
            {
                m_Data[ g ].push_back( std::vector<NodeData*>() );
                for( int i = 0 ; i < m_IPValuesList.size() ; ++i )
                {
                    NodeData* pnd = CreateNodeData();
                    m_Data[ g ][ a ].push_back( pnd );
                }
            }
        }
        BaseTextReport::Initialize( nrmSize );
    }

    std::string ReportNodeDemographics::GetHeader() const
    {
        std::stringstream header ;
        header <<         "Time" 
               << ", " << "NodeID" ;
        if( m_StratifyByGender )
        {
            header << ", " << "Gender" ;
        }
        if( m_StratifyByAge )
        {
            header << ", " << "AgeYears" ;
        }
        if( !m_IPKeyToCollect.empty() )
        {
            header << ", IndividualProp=" << m_IPKeyToCollect;
        }

        header << ", " << "NumIndividuals" 
               << ", " << "NumInfected"
               ;

        for( auto pnp : NPFactory::GetInstance()->GetNPList() )
        {
            header << ", NodeProp=" << pnp->GetKey<NPKey>().ToString();
        }

        return header.str();
    }

    bool ReportNodeDemographics::IsCollectingIndividualData( float currentTime, float dt ) const
    {
        return true ;
    }

    void ReportNodeDemographics::LogIndividualData( IIndividualHuman* individual ) 
    {
        int gender_index  = m_StratifyByGender ? (int)(individual->GetGender()) : 0;
        int age_bin_index = ReportUtilities::GetAgeBin( individual->GetAge(), m_AgeYears );
        int ip_index      = GetIPIndex( individual->GetProperties() );

        NodeData* p_nd = m_Data[ gender_index ][ age_bin_index ][ ip_index ];
        p_nd->num_people += 1;

        if( individual->IsInfected() )
        {
            p_nd->num_infected += 1;
        }
        LogIndividualData( individual, p_nd );
    }

    void ReportNodeDemographics::LogNodeData( INodeContext* pNC )
    {
        float time = pNC->GetTime().time;
        auto node_id = pNC->GetExternalID();
        NPKeyValueContainer& np_values = pNC->GetNodeProperties();

        int num_genders = m_StratifyByGender ? 2 : 1; // 1 implies not stratifying by gender
        for( int g = 0 ; g < num_genders ; g++ )
        {
            char* gender = (g == 0) ? "M" : "F"  ;

            for( int a = 0 ; a < m_AgeYears.size() ; a++ )
            {
                for( int i = 0 ; i < m_IPValuesList.size() ; ++i )
                {
                    GetOutputStream() << time
                               << "," << node_id;
                    if( m_StratifyByGender )
                    {
                        GetOutputStream() << "," << gender;
                    }
                    if( m_StratifyByAge )
                    {
                        GetOutputStream() << "," << m_AgeYears[ a ];
                    }
                    if( !m_IPKeyToCollect.empty() )
                    {
                        GetOutputStream() << ", " << m_IPValuesList[ i ] ;
                    }

                    WriteNodeData( m_Data[ g ][ a ][ i ] );

                    for( auto pnp : NPFactory::GetInstance()->GetNPList() )
                    {
                        NPKeyValue kv = np_values.Get( pnp->GetKey<NPKey>() );
                        GetOutputStream() << "," << kv.GetValueAsString();
                    }
                    GetOutputStream() << std::endl;
                }
            }
        }

        // Reset the counters for the next node
        for( int g = 0 ; g < num_genders ; g++ )
        {
            for( int a = 0 ; a < m_AgeYears.size() ; a++ )
            {
                for( int i = 0 ; i < m_IPValuesList.size() ; ++i )
                {
                    m_Data[ g ][ a ][ i ]->Reset();
                }
            }
        }
    }

    void ReportNodeDemographics::WriteNodeData( const NodeData* pData )
    {
        GetOutputStream() << "," << pData->num_people
                          << "," << pData->num_infected;
    }

    int ReportNodeDemographics::GetIPIndex( IPKeyValueContainer* pProps ) const
    {
        int index = 0;
        if( !m_IPKeyToCollect.empty() )
        {
            std::string value = pProps->Get( IPKey(m_IPKeyToCollect) ).GetValueAsString();

            index = std::find( m_IPValuesList.cbegin(), m_IPValuesList.cend(), value ) - m_IPValuesList.cbegin();
        }
        return index;
    }

}