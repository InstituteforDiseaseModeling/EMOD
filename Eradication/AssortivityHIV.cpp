/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "AssortivityHIV.h"
#include "Exceptions.h"
#include "IIndividualHumanHIV.h"
#include "IHIVInterventionsContainer.h"
#include "IndividualEventContext.h"
#include "SimulationConfig.h"

SETUP_LOGGING( "AssortivityHIV" )

namespace Kernel 
{
    BEGIN_QUERY_INTERFACE_DERIVED(AssortivityHIV,Assortivity)
    END_QUERY_INTERFACE_DERIVED(AssortivityHIV,Assortivity)

    AssortivityHIV::AssortivityHIV( RelationshipType::Enum relType, RANDOMBASE* prng )
        : Assortivity( relType, prng )
    {
    }

    AssortivityHIV::~AssortivityHIV()
    {
    }

    void AssortivityHIV::AddConfigurationParameters( AssortivityGroup::Enum group, const Configuration *config )
    {
    }

    void AssortivityHIV::CheckDerivedValues()
    {
        if( (GetGroup() == AssortivityGroup::STI_COINFECTION_STATUS     ) ||
            (GetGroup() == AssortivityGroup::HIV_INFECTION_STATUS       ) ||
            (GetGroup() == AssortivityGroup::HIV_TESTED_POSITIVE_STATUS ) ||
            (GetGroup() == AssortivityGroup::HIV_RECEIVED_RESULTS_STATUS) )
        {
            if( GET_CONFIGURABLE( SimulationConfig )->sim_type != SimType::HIV_SIM )
            {
                std::stringstream detail ;
                detail << AssortivityGroup::pairs::lookup_key( GetGroup() ) << " is only valid with HIV_SIM." ;

                const char* sim_type_str = SimType::pairs::lookup_key( GET_CONFIGURABLE( SimulationConfig )->sim_type );

                std::stringstream ss ;
                ss << RelationshipType::pairs::lookup_key( GetRelationshipType() ) << ":Group"; 

                throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, 
                                                        ss.str().c_str(), AssortivityGroup::pairs::lookup_key( GetGroup() ), 
                                                        "Simulation_Type", sim_type_str,
                                                        detail.str().c_str() );
            }
            if (GetGroup() == AssortivityGroup::HIV_RECEIVED_RESULTS_STATUS)
            {
                CheckAxesForReceivedResults();
                SortMatrixReceivedResults();
            }
            else
            {
                CheckAxesForTrueFalse();
                SortMatrixFalseTrue();
            }
        }
    }

    void AssortivityHIV::CheckAxesForReceivedResults()
    {
        std::vector<std::string>& r_axes = GetAxes();
        for( int i = 0 ; i < r_axes.size() ; i++ )
        {
            std::transform( r_axes[i].begin(), r_axes[i].end(), r_axes[i].begin(), ::toupper );
        }

        bool invalid_axes = r_axes.size() != ReceivedTestResultsType::pairs::count() ;
        for( int i = 0 ; !invalid_axes && (i < r_axes.size()) ; i++ )
        {
            int val = ReceivedTestResultsType::pairs::lookup_value( r_axes[i].c_str() );
            invalid_axes = val == -1 ; // -1 implies that the axes name was not found as an enum
        }

        if( invalid_axes )
        {
            std::vector<std::string> result_names ;
            for( int i = 0 ; i < ReceivedTestResultsType::pairs::count() ; i++ )
            {
                std::string name = ReceivedTestResultsType::pairs::get_keys()[i] ;
                result_names.push_back( name );
            }
            std::stringstream ss ;
            ss << "The " << RelationshipType::pairs::lookup_key( GetRelationshipType() ) << ":Group (" 
               << AssortivityGroup::pairs::lookup_key( GetGroup() ) <<") requires that the Axes names(="
               << ValuesToString( r_axes )
               <<") are "
               << ValuesToString( result_names ) << ".  Order is up to the user." ;
            throw GeneralConfigurationException(__FILE__, __LINE__, __FUNCTION__,  ss.str().c_str() );
        }
    }

    void SwapRows( std::vector<std::vector<float>>& rWeightingMatrix, int aRowIndex, int bRowIndex )
    {
        for( int col_index = 0 ; col_index < rWeightingMatrix[aRowIndex].size() ; ++col_index )
        {
            float tmp = rWeightingMatrix[ aRowIndex ][ col_index ];
            rWeightingMatrix[ aRowIndex ][ col_index ] = rWeightingMatrix[ bRowIndex ][ col_index ];
            rWeightingMatrix[ bRowIndex ][ col_index ] = tmp;
        }
    }

    void SwapColumns( std::vector<std::vector<float>>& rWeightingMatrix, int aColIndex, int bColIndex )
    {
        for( int row_index = 0 ; row_index < rWeightingMatrix.size() ; ++row_index )
        {
            float tmp = rWeightingMatrix[ row_index ][ aColIndex ];
            rWeightingMatrix[ row_index ][ aColIndex ] = rWeightingMatrix[ row_index ][ bColIndex ];
            rWeightingMatrix[ row_index ][ bColIndex ] = tmp;
        }
    }

    void AssortivityHIV::SortMatrixReceivedResults()
    {
        for( int result_index = 0 ; result_index < ReceivedTestResultsType::pairs::count() ; ++result_index )
        {
            std::string type_name = ReceivedTestResultsType::pairs::get_keys()[ result_index ];
            int actual_index = GetIndex( type_name );
            if( result_index != actual_index )
            {
                SwapRows( m_WeightingMatrix, result_index, actual_index );
                SwapColumns( m_WeightingMatrix, result_index, actual_index );

                std::string tmp = m_Axes[ actual_index ];
                m_Axes[ actual_index ] = m_Axes[ result_index ];
                m_Axes[ result_index ] = tmp;
            }
        }
    }

    int GetIndexHIV( const Assortivity* pAssortivity, const IIndividualHumanSTI* pIndividual )
    {
        IIndividualHumanHIV * p_partner_hiv = nullptr;
        if (s_OK != (const_cast<IIndividualHumanSTI*>(pIndividual))->QueryInterface(GET_IID(IIndividualHumanHIV), (void**)&p_partner_hiv) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "p_partner_hiv", "IIndividualHumanHIV", "IIndividualHumanSTI");
        }
        return (p_partner_hiv->HasHIV() ? 1 : 0) ;
    }

    int GetIndexHIVTestedPositive( const Assortivity* pAssortivity, const IIndividualHumanSTI* pIndividual )
    {
        IIndividualHumanHIV * p_partner_hiv = nullptr;
        if (s_OK != (const_cast<IIndividualHumanSTI*>(pIndividual))->QueryInterface(GET_IID(IIndividualHumanHIV), (void**)&p_partner_hiv) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "p_partner_hiv", "IIndividualHumanHIV", "IIndividualHumanSTI");
        }
        IHIVMedicalHistory * p_med_history = nullptr;
        if (p_partner_hiv->GetHIVInterventionsContainer()->QueryInterface(GET_IID(IHIVMedicalHistory), (void**)&p_med_history) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "pIndividual", "IHIVMedicalHistory", "IHIVInterventionsContainer" );
        }
        return (p_med_history->EverTestedHIVPositive() ? 1 : 0) ;
    }

    int GetIndexHIVReceivedResults( const Assortivity* pAssortivity, const IIndividualHumanSTI* pIndividual )
    {
        IIndividualHumanHIV * p_partner_hiv = nullptr;
        if (s_OK != (const_cast<IIndividualHumanSTI*>(pIndividual))->QueryInterface(GET_IID(IIndividualHumanHIV), (void**)&p_partner_hiv) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "p_partner_hiv", "IIndividualHumanHIV", "IIndividualHumanSTI");
        }
        IHIVMedicalHistory * p_med_history = nullptr;
        if (p_partner_hiv->GetHIVInterventionsContainer()->QueryInterface(GET_IID(IHIVMedicalHistory), (void**)&p_med_history) != s_OK)
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "p_partner_hiv->GetHIVInterventionsContainer()", "IHIVMedicalHistory", "IHIVInterventionsContainer" );
        }
        ReceivedTestResultsType::Enum results_enum = p_med_history->ReceivedTestResultForHIV();
        return int( results_enum );
    }

    IIndividualHumanSTI* AssortivityHIV::SelectPartnerForExtendedGroups( AssortivityGroup::Enum group,
                                                                         IIndividualHumanSTI* pPartnerA,
                                                                         const list<IIndividualHumanSTI*>& potentialPartnerList )
    {
        IIndividualHumanSTI* p_partner_B = nullptr ;
        switch( group )
        {
            case AssortivityGroup::HIV_INFECTION_STATUS:
                p_partner_B = FindPartner( pPartnerA, potentialPartnerList, GetIndexHIV );
                break;

            case AssortivityGroup::HIV_TESTED_POSITIVE_STATUS:
                p_partner_B = FindPartner( pPartnerA, potentialPartnerList, GetIndexHIVTestedPositive );
                break;

            case AssortivityGroup::HIV_RECEIVED_RESULTS_STATUS:
                p_partner_B = FindPartner( pPartnerA, potentialPartnerList, GetIndexHIVReceivedResults );
                break;

            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "group", group, AssortivityGroup::pairs::lookup_key( group ) );
                break;
        }

        return p_partner_B ;
    }

    REGISTER_SERIALIZABLE(AssortivityHIV);

    void AssortivityHIV::serialize(IArchive& ar, AssortivityHIV* obj)
    {
        Assortivity::serialize( ar, obj );
        AssortivityHIV& hiv = *obj;
    }
}
