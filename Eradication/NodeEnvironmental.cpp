/***************************************************************************************************

Copyright (c) 2019 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#if defined(ENABLE_ENVIRONMENTAL)

#include "IdmDateTime.h"
#include "NodeEnvironmental.h"
#include "IndividualEnvironmental.h"
#include "TransmissionGroupsFactory.h"
#include "SimulationConfig.h"
#include "INodeContext.h"
#include "ISimulationContext.h"
#include "SimulationEventContext.h"
#include "EventTriggerNode.h"
#include "Infection.h"

SETUP_LOGGING( "NodeEnvironmental" )

namespace Kernel
{
    GET_SCHEMA_STATIC_WRAPPER_IMPL(NodeEnvironmental,NodeEnvironmental)
    /*BEGIN_QUERY_INTERFACE_DERIVED(NodeTyphoid, Node)
        HANDLE_INTERFACE(INodeTyphoid)
    END_QUERY_INTERFACE_DERIVED(NodeTyphoid, Node)*/

    NodeEnvironmental::NodeEnvironmental()
    : Node()
    , contagion(0)
    , txEnvironment(nullptr)
    {
    }

    NodeEnvironmental::NodeEnvironmental(ISimulationContext *_parent_sim, ExternalNodeId_t externalNodeId, suids::suid node_suid)
    : Node(_parent_sim, externalNodeId, node_suid)
    , contagion(0)
    , txEnvironment(nullptr)
    {
    }

    NodeEnvironmental::~NodeEnvironmental(void)
    {
    }

    NodeEnvironmental *NodeEnvironmental::CreateNode(ISimulationContext *_parent_sim, ExternalNodeId_t externalNodeId, suids::suid node_suid)
    {
        NodeEnvironmental *newnode = _new_ NodeEnvironmental(_parent_sim, externalNodeId, node_suid);
        newnode->Initialize();

        return newnode;
    }

    bool NodeEnvironmental::Configure( const Configuration* config )
    {
        initConfigTypeMap( "Node_Contagion_Decay_Rate", &node_contagion_decay_fraction, Node_Contagion_Decay_Rate_DESC_TEXT, 1.0f ); //this value represents *fraction* of contagion not carried over to the next time step, see EndUpdate() in transmissiongroups
        initConfigTypeMap( "Environmental_Ramp_Up_Duration", &environmental_ramp_up_duration, Environmental_Ramp_Up_Duration_DESC_TEXT, FLT_MIN, 365, 2 );
        initConfigTypeMap( "Environmental_Ramp_Down_Duration", &environmental_ramp_down_duration, Environmental_Ramp_Down_Duration_DESC_TEXT, FLT_MIN, 365, 2 );
        initConfigTypeMap( "Environmental_Cutoff_Days", &environmental_cutoff_days, Environmental_Cutoff_Days_DESC_TEXT, 0, DAYSPERYEAR, 2 );
        initConfigTypeMap( "Environmental_Peak_Start", &environmental_peak_start, Environmental_Peak_Start_DESC_TEXT, 0, 500, 2 );
        bool ret = Node::Configure( config );
        if( environmental_ramp_up_duration + environmental_ramp_down_duration + environmental_cutoff_days >= DAYSPERYEAR )
        {
            std::ostringstream msg;
            msg << "Environmental_Ramp_Up_Duration ("
                << environmental_ramp_up_duration
                << ") + Environmental_Ramp_Down_Duration ("
                << environmental_ramp_down_duration
                << ") + Environmental_Cutoff_Days ("
                << environmental_cutoff_days
                << ") must be < 365. Equals "
                << environmental_ramp_up_duration + environmental_ramp_down_duration + environmental_cutoff_days
                << ".";
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
        return ret;
    }

    IIndividualHuman* NodeEnvironmental::createHuman( suids::suid suid, float monte_carlo_weight, float initial_age, int gender)
    {
        return IndividualHumanEnvironmental::CreateHuman(this, suid, monte_carlo_weight, initial_age, gender);
    }

    void NodeEnvironmental::updateInfectivity(float dt)
    {
        Node::updateInfectivity(dt);
        txEnvironment->EndUpdate(getSeasonalAmplitude());
        GetParent()->GetSimulationEventContext()->GetNodeEventBroadcaster()->TriggerObservers( GetEventContext(), EventTriggerNode::SheddingComplete );
    }

    float NodeEnvironmental::getClimateInfectivityCorrection() const
    {
        // Environmental infectivity depends on rainfall.
        // TODO: make more configurable to accommodate different modalities:
        //       - high rainfall inducing prolonged flooding
        //       - high rainfall causing dilution and flushing vs. low rainfall causing concentration
        //       - etc.

        float correction = 1.0f;

        if ( localWeather == nullptr )
        {
            throw NullPointerException( __FILE__, __LINE__, __FUNCTION__, "localWeather", "Climate");
        }

        float rainfall = localWeather->accumulated_rainfall();

        // The following is a linear increase in infectivity above a threshold of 10mm of rainfall
        if ( rainfall > 0.01 )
        {
            correction += (rainfall - 0.01) / 0.01;
        }
        LOG_DEBUG_F( "Infectivity scale factor = %f at rainfall = %f.\n", correction, rainfall );

        return correction;
    }

    ITransmissionGroups* NodeEnvironmental::CreateTransmissionGroups()
    {
        txEnvironment = TransmissionGroupsFactory::CreateNodeGroups( TransmissionGroupType::StrainAwareGroups, GetRng() );
        txEnvironment->UseGroupPopulationForNormalization();
        txEnvironment->SetTag( "environmental" );
        return TransmissionGroupsFactory::CreateNodeGroups( TransmissionGroupType::StrainAwareGroups, GetRng() );
    }

    void NodeEnvironmental::AddDefaultRoute( void )
    {
        AddRoute( ENVIRONMENTAL );
    }

    void NodeEnvironmental::BuildTransmissionRoutes( float contagionDecayRate )
    {
        LOG_DEBUG_F("Number of basestrains: %d\n", InfectionConfig::number_basestrains);
        transmissionGroups->Build( contagionDecayRate, InfectionConfig::number_basestrains, InfectionConfig::number_substrains );
        txEnvironment->Build( node_contagion_decay_fraction, InfectionConfig::number_basestrains, InfectionConfig::number_substrains );
    }

    bool NodeEnvironmental::IsValidTransmissionRoute( string& transmissionRoute )
    {
        bool isValid = ((transmissionRoute == CONTACT) || (transmissionRoute == ENVIRONMENTAL));
        return isValid;
    }

    float NodeEnvironmental::getSeasonalAmplitude() const
    {
        float amplification = 0.0f;

        float peak_amplification = 1.0f;
        float ramp_down_days = environmental_ramp_down_duration;
        float ramp_up_days = environmental_ramp_up_duration;
        float cutoff_days = environmental_cutoff_days;
        float peak_start_day = floor(environmental_peak_start);
        if (peak_start_day > DAYSPERYEAR)
        {
            peak_start_day = peak_start_day - DAYSPERYEAR;
        }
        //this is mostly for calibtool purposes
        float peak_days = (DAYSPERYEAR - cutoff_days) - (ramp_down_days + ramp_up_days);
        float peak_end_day = peak_start_day + peak_days;
        if (peak_end_day > DAYSPERYEAR)
        {
            peak_end_day = peak_end_day - DAYSPERYEAR;
        }

        float slope_up = peak_amplification / ramp_up_days;
        float slope_down = peak_amplification / ramp_down_days;

        int SimDay = (int)GetTime().time; // is this the date of the simulated year?
        int nDayOfYear = SimDay % DAYSPERYEAR;
#define OFFSET (0.0f) // (0.5f)
        if (peak_start_day - ramp_up_days > 0)
        {
            if ((nDayOfYear >= peak_start_day-ramp_up_days) && ( nDayOfYear < peak_start_day))
            { // beginning of wastewater irrigation
                amplification=((nDayOfYear- (peak_start_day-ramp_up_days))+ OFFSET )*(slope_up);
                LOG_DEBUG_F( "Seasonal Amplification 1 stage A: ramping up." );
            }
            else if ((peak_start_day - peak_end_day >= 0) && ((nDayOfYear >= peak_start_day)  || (nDayOfYear<=peak_end_day)))
            { // peak of wastewater irrigation
                amplification= peak_amplification;
                LOG_DEBUG_F( "Seasonal Amplification 1 stage B: peak amp plateau." );
            }
            else if ((peak_start_day - peak_end_day < 0) && (nDayOfYear >= peak_start_day) && (nDayOfYear <= peak_end_day))
            { // peak of wastewater irrigation
                amplification= peak_amplification;
                LOG_DEBUG_F( "Seasonal Amplification 1 stage C: peak amp plateau." );
            }
            else if ((peak_end_day + ramp_down_days < DAYSPERYEAR) && (nDayOfYear > peak_end_day) && (nDayOfYear <= (peak_end_day+ramp_down_days)))
            {
                amplification= peak_amplification-(((nDayOfYear-peak_end_day)- OFFSET )*slope_down);
                LOG_DEBUG_F( "Seasonal Amplification 1 stage D: slope down." );
            }
            else if ((peak_end_day + ramp_down_days >= DAYSPERYEAR) && ((nDayOfYear > peak_end_day) || (nDayOfYear < ramp_down_days - (DAYSPERYEAR- peak_end_day))))
            {
                // end of wastewater irrigation
                if (nDayOfYear > peak_end_day)
                {
                    amplification = peak_amplification-(((nDayOfYear-peak_end_day)- OFFSET )*slope_down);
                    LOG_DEBUG_F( "Seasonal Amplification 1 stage E(i): slope down." );
                }
                if (nDayOfYear < ramp_down_days - (DAYSPERYEAR - peak_end_day))
                {
                    amplification = peak_amplification - (((DAYSPERYEAR-peak_end_day)+nDayOfYear- OFFSET )*slope_down);
                    LOG_DEBUG_F( "Seasonal Amplification 1 stage E(ii): slope down." );
                }
            }
            else
            {
                LOG_DEBUG_F( "Seasonal Amplification 1 stage F: zero amp plateau." );
            }
        }
        else if (peak_start_day - ramp_up_days <= 0)
        {
            if ((nDayOfYear >= peak_start_day-ramp_up_days+DAYSPERYEAR) || ( nDayOfYear < peak_start_day))
            { // beginning of wastewater irrigation
                if (nDayOfYear >= peak_start_day-ramp_up_days+DAYSPERYEAR)
                {
                    amplification= (nDayOfYear - (peak_start_day-ramp_up_days+DAYSPERYEAR)+ OFFSET )*(slope_up);
                    LOG_DEBUG_F( "Seasonal Amplification 2 stage A(i): ramping up." );
                }
                else if (nDayOfYear < peak_start_day)
                {
                    amplification= (((ramp_up_days-peak_start_day) + nDayOfYear)  +  OFFSET )*(slope_up);
                    LOG_DEBUG_F( "Seasonal Amplification 2 stage A(ii): ramping up." );
                }
            }
            if ((peak_start_day - peak_end_day > 0) && ((nDayOfYear >= peak_start_day)  || (nDayOfYear<=peak_end_day)))
            { // peak of wastewater irrigation
                amplification= peak_amplification;
                LOG_DEBUG_F( "Seasonal Amplification 2 stage B: peak plateau." );
            }
            else if ((peak_start_day - peak_end_day < 0) && (nDayOfYear >= peak_start_day) && (nDayOfYear <= peak_end_day))
            { // peak of wastewater irrigation
                amplification= peak_amplification;
                LOG_DEBUG_F( "Seasonal Amplification 2 stage C: peak plateau." );
            }
            else if ((nDayOfYear > peak_end_day) && (nDayOfYear <= (peak_end_day+ramp_down_days)) )
            { // end of wastewater irrigation
                amplification= peak_amplification-(((nDayOfYear-peak_end_day)- OFFSET )*slope_down);
                LOG_DEBUG_F( "Seasonal Amplification 2 stage D: bottom plateau." );
            }
        }

        LOG_VALID_F("amplification calculated as %f: day of year=%d, start=%f, end=%f, ramp_up=%f, ramp_down=%f, cutoff=%f.\n", amplification, nDayOfYear, peak_start_day, peak_end_day, ramp_up_days, ramp_down_days, cutoff_days );
        return amplification;
    }

    void NodeEnvironmental::SetupIntranodeTransmission()
    {
        transmissionGroups = CreateTransmissionGroups();

        if( IPFactory::GetInstance() && IPFactory::GetInstance()->HasIPs() && params()->heterogeneous_intranode_transmission_enabled )
        {
            ValidateIntranodeTransmissionConfiguration();

            for( auto p_ip : IPFactory::GetInstance()->GetIPList() )
            {
                auto hint = p_ip->GetIntraNodeTransmission( GetExternalID() );
                auto matrix = hint.GetMatrix();

                if ( matrix.size() > 0 )
                {
                    std::string routeName = hint.GetRouteName();
                    AddRoute( routeName );
                    ITransmissionGroups* txGroups = (routeName == CONTACT) ? transmissionGroups : txEnvironment;
                    txGroups->AddProperty( p_ip->GetKeyAsString(),
                                           p_ip->GetValues<IPKeyValueContainer>().GetValuesToList(),
                                           matrix );
                }
                else if ( hint.GetRouteToMatrixMap().size() > 0 )
                {
                    for (auto entry : hint.GetRouteToMatrixMap())
                    {
                        std::string routeName = entry.first;
                        auto& matrix = entry.second;
                        AddRoute( routeName );
                        ITransmissionGroups* txGroups = (routeName == CONTACT) ? transmissionGroups : txEnvironment;
                        txGroups->AddProperty( p_ip->GetKeyAsString(),
                                               p_ip->GetValues<IPKeyValueContainer>().GetValuesToList(),
                                               matrix );
                    }
                }
                else //HINT is enabled, but no transmission matrix is detected
                {
                    // This is okay. It is not required for all IndividualProperties to participate in HINT.
                }
            }
        }
        else
        {
            // Default with no custom HINT
            LOG_DEBUG("non-HINT: Adding route 'environmental' and 'contact'.\n");
            AddRoute( CONTACT );
            AddRoute( ENVIRONMENTAL );
        }

        BuildTransmissionRoutes( 1.0f );
    }

    void NodeEnvironmental::DepositFromIndividual( const IStrainIdentity& strain_IDs, float contagion_quantity, TransmissionGroupMembership_t individual, TransmissionRoute::Enum route )
    {
        LOG_DEBUG_F( "deposit from individual: antigen index =%d, substain index = %d, quantity = %f, route = %d\n", strain_IDs.GetAntigenID(), strain_IDs.GetGeneticID(), contagion_quantity, uint32_t(route) );

        switch (route)
        {
            case TransmissionRoute::TRANSMISSIONROUTE_CONTACT:
                transmissionGroups->DepositContagion( strain_IDs, contagion_quantity, individual );
                break;

            case TransmissionRoute::TRANSMISSIONROUTE_ENVIRONMENTAL:
                txEnvironment->DepositContagion( strain_IDs, contagion_quantity, individual );
                break;

            default:
                // TODO - try to get proper string from route enum
                throw new BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "route", uint32_t(route), "???" );
        }
    }

    float NodeEnvironmental::GetContagionByRouteAndProperty( const std::string& route, const IPKeyValue& property_value )
    {
        float contagion = nanf("NAN");

        ITransmissionGroups* txGroups = (route == CONTACT) ? transmissionGroups : txEnvironment;
        contagion = txGroups->GetContagionByProperty( property_value );

        return contagion;
    }

    void NodeEnvironmental::GetGroupMembershipForIndividual(const RouteList_t& routes, const tProperties& properties, TransmissionGroupMembership_t& transmissionGroupMembership)
    {
        LOG_DEBUG_F( "Calling %s.\n", __FUNCTION__ );
        for (auto& route: routes)
        {
            ITransmissionGroups* txGroups = (route == CONTACT) ? transmissionGroups : txEnvironment;
            txGroups->GetGroupMembershipForProperties( properties, transmissionGroupMembership );
        }
    }

    void NodeEnvironmental::UpdateTransmissionGroupPopulation(const tProperties& properties, float size_delta, float mc_weight)
    {
        TransmissionGroupMembership_t contact;
        transmissionGroups->GetGroupMembershipForProperties( properties, contact );
        transmissionGroups->UpdatePopulationSize(contact, size_delta, mc_weight);
        TransmissionGroupMembership_t environmental;
        txEnvironment->GetGroupMembershipForProperties( properties, environmental );
        txEnvironment->UpdatePopulationSize(environmental, size_delta, mc_weight);
    }

    void NodeEnvironmental::ExposeIndividual(IInfectable* candidate, TransmissionGroupMembership_t individual, float dt)
    {
        Node::ExposeIndividual( candidate, individual, dt );    // Will expose to contact contagion.
        txEnvironment->ExposeToContagion(candidate, individual, dt, TransmissionRoute::TRANSMISSIONROUTE_ENVIRONMENTAL);
    }

    std::map<std::string, float> NodeEnvironmental::GetContagionByRoute() const
    {
        std::map< std::string, float > returnThis;

        returnThis.insert( std::make_pair( CONTACT,       transmissionGroups->GetTotalContagion() ) );
        returnThis.insert( std::make_pair( ENVIRONMENTAL, txEnvironment->GetTotalContagion() ) );

        LOG_VALID_F( "contact: %f, environmental: %f\n", returnThis[CONTACT], returnThis[ENVIRONMENTAL] );

        return returnThis;
    }

    REGISTER_SERIALIZABLE(NodeEnvironmental);

    void NodeEnvironmental::serialize(IArchive& ar, NodeEnvironmental* obj)
    {
        Node::serialize(ar, obj);
        NodeEnvironmental& node = *obj;
        ar.labelElement("contagion") & node.contagion;
        ar.labelElement("node_contagion_decay_fraction") & node.node_contagion_decay_fraction;
    }
}

#endif // ENABLE_POLIO
