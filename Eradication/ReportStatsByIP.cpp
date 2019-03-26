/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "ReportStatsByIP.h"
#include "Log.h"
#include "Exceptions.h"
#include "NodeEventContext.h"
#include "IndividualEventContext.h"

SETUP_LOGGING( "ReportStatsByIP" )

namespace Kernel
{
    ReportStatsByIP::ReportStatsByIP()
        : m_IpKeys()
        , m_StatsTotal()
        , m_StatsByIP()
    {
    }

    ReportStatsByIP::~ReportStatsByIP()
    {
    }

    void ReportStatsByIP::SetIPKeyNames( const std::string& rParameterName,
                                         const jsonConfigurable::tDynamicStringSet& rIPKeyNames )
    {
        for( auto key_name : rIPKeyNames )
        {
            IndividualProperty* p_ip = IPFactory::GetInstance()->GetIP( key_name, rParameterName.c_str(), false );
            if( p_ip == nullptr )
            {
                std::stringstream ss;
                ss << "The IP Key (" << key_name << ") specified in '" << rParameterName << "' is unknown.\n"
                    << "Valid values are: " << IPFactory::GetInstance()->GetKeysAsString();
                throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
            }
            m_IpKeys.push_back( p_ip->GetKey<IPKey>() );
            for( auto kv : p_ip->GetValues<IPKeyValueContainer>() )
            {
                m_StatsByIP.insert( std::make_pair( kv.ToString(), StatsPerIP() ) );
            }
        }
    }

    std::string ReportStatsByIP::GetHeader() const
    {
        std::stringstream header;
        header <<        "NumIndividuals"
               << "," << "NumInfected";

        for( auto key : m_IpKeys )
        {
            IndividualProperty* p_ip = IPFactory::GetInstance()->GetIP( key.ToString() );
            for( auto kv : p_ip->GetValues<IPKeyValueContainer>() )
            {
                header << "," << kv.ToString() << ":NumIndividuals"
                       << "," << kv.ToString() << ":NumInfected";
            }
        }
        return header.str();
    }

    void ReportStatsByIP::ResetData()
    {
        m_StatsTotal.Clear();
        for( auto& entry : m_StatsByIP )
        {
            entry.second.Clear();
        }
    }

    void ReportStatsByIP::CollectDataFromNode( INodeEventContext* pNEC,
                                               individual_qualified_function_t isQualifiedFunc )
    {
        INodeEventContext::individual_visit_function_t count_func =
            [ this, isQualifiedFunc ]( IIndividualHumanEventContext *ihec )
        {
            if( isQualifiedFunc( ihec ) )
            {
                ++(m_StatsTotal.num_individuals);
                if( ihec->IsInfected() )
                {
                    ++(m_StatsTotal.num_infected);
                }

                // ----------------------------------------------------------------------------------
                // --- For each key of interest to the user, find the value that the individual has.
                // --- With this key-value pair, find the entry in the map so we can update the stats
                // --- for that particular key-value pair.
                // ----------------------------------------------------------------------------------
                for( auto key : m_IpKeys )
                {
                    IPKeyValue kv = ihec->GetProperties()->Get( key );
                    auto& r_entry = m_StatsByIP[ kv.ToString() ];

                    ++(r_entry.num_individuals);
                    if( ihec->IsInfected() )
                    {
                        ++(r_entry.num_infected);
                    }
                }
            }
        };

        pNEC->VisitIndividuals( count_func );
    }

    std::string ReportStatsByIP::GetReportData()
    {
        std::stringstream ss;
        ss <<        m_StatsTotal.num_individuals
           << "," << m_StatsTotal.num_infected;

        for( auto key : m_IpKeys )
        {
            IndividualProperty* p_ip = IPFactory::GetInstance()->GetIP( key.ToString() );
            for( auto kv : p_ip->GetValues<IPKeyValueContainer>() )
            {
                auto& r_entry = m_StatsByIP[ kv.ToString() ];
                ss << "," << r_entry.num_individuals
                   << "," << r_entry.num_infected;
            }
        }
        return ss.str();
    }

    const StatsPerIP& ReportStatsByIP::GetStatsTotal() const
    {
        return m_StatsTotal;
    }

    const StatsPerIP& ReportStatsByIP::GetStats( IPKeyValue kv ) const
    {
        return m_StatsByIP.at( kv.ToString() );
    }
}
