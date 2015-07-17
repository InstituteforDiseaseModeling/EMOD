/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"
#include "AssortivityHIV.h"
#include "Exceptions.h"
#include "IIndividualHumanHIV.h"
#include "IHIVInterventionsContainer.h"
#include "IndividualEventContext.h"
#include "SimulationConfig.h"

namespace Kernel 
{
    static const char * _module = "AssortivityHIV";

    AssortivityHIV::AssortivityHIV( RelationshipType::Enum relType, RANDOMBASE* prng )
        : Assortivity( relType, prng )
        , m_StartYear( 0.0 )
        , m_StartUsing(false)
    {
    }

    AssortivityHIV::~AssortivityHIV()
    {
    }

    void AssortivityHIV::AddConfigurationParameters( AssortivityGroup::Enum group, const Configuration *config )
    {
        if( JsonConfigurable::_dryrun || (group == AssortivityGroup::STI_COINFECTION_STATUS     ) 
                                      || (group == AssortivityGroup::HIV_INFECTION_STATUS       )
                                      || (group == AssortivityGroup::HIV_TESTED_POSITIVE_STATUS )
                                      || (group == AssortivityGroup::HIV_RECEIVED_RESULTS_STATUS) )
        {
            initConfigTypeMap( "Start_Year", &m_StartYear, "TBD - The year to start using the assortivity preference.", 0.0f, 3000.0f, 0.0f );
        }
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
            if( GetGroup() == AssortivityGroup::HIV_RECEIVED_RESULTS_STATUS )
                CheckAxesForReceivedResults();
            else
                CheckAxesForTrueFalse();
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

    void AssortivityHIV::Update( const IdmDateTime& rCurrentTime, float dt )
    {
        float current_year = rCurrentTime.Year() ;
        m_StartUsing = m_StartYear < current_year ;
    }

    AssortivityGroup::Enum AssortivityHIV::GetGroupToUse() const
    {
        AssortivityGroup::Enum group = GetGroup() ;
        if( !m_StartUsing )
        {
            group = AssortivityGroup::NO_GROUP ;
        }
        return group ;
    }

    std::string GetStringValueHIV( const Assortivity* pAssortivity, const IIndividualHumanSTI* pIndividual )
    {
        IIndividualHumanHIV * p_partner_hiv = nullptr;
        if (s_OK != (const_cast<IIndividualHumanSTI*>(pIndividual))->QueryInterface(GET_IID(IIndividualHumanHIV), (void**)&p_partner_hiv) )
        {
            throw QueryInterfaceException(__FILE__, __LINE__, __FUNCTION__, "p_partner_hiv", "IIndividualHumanHIV", "IIndividualHumanSTI");
        }
        return (p_partner_hiv->HasHIV() ? std::string("TRUE") : std::string("FALSE")) ;
    }

    std::string GetStringValueHIVTestedPositive( const Assortivity* pAssortivity, const IIndividualHumanSTI* pIndividual )
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
        return (p_med_history->EverTestedHIVPositive() ? std::string("TRUE") : std::string("FALSE")) ;
    }

    std::string GetStringValueHIVReceivedResults( const Assortivity* pAssortivity, const IIndividualHumanSTI* pIndividual )
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
        std::string results_str = ReceivedTestResultsType::pairs::lookup_key( results_enum );
        return results_str ;
    }

    IIndividualHumanSTI* AssortivityHIV::SelectPartnerForExtendedGroups( AssortivityGroup::Enum group,
                                                                         const IIndividualHumanSTI* pPartnerA,
                                                                         const list<IIndividualHumanSTI*>& potentialPartnerList )
    {
        IIndividualHumanSTI* p_partner_B = nullptr ;
        switch( group )
        {
            case AssortivityGroup::HIV_INFECTION_STATUS:
                p_partner_B = FindPartner( pPartnerA, potentialPartnerList, GetStringValueHIV );
                break;

            case AssortivityGroup::HIV_TESTED_POSITIVE_STATUS:
                p_partner_B = FindPartner( pPartnerA, potentialPartnerList, GetStringValueHIVTestedPositive );
                break;

            case AssortivityGroup::HIV_RECEIVED_RESULTS_STATUS:
                p_partner_B = FindPartner( pPartnerA, potentialPartnerList, GetStringValueHIVReceivedResults );
                break;

            default:
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "group", group, AssortivityGroup::pairs::lookup_key( group ) );
                break;
        }

        return p_partner_B ;
    }
}
