/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "SimulationHIV.h"

#include "NodeHIV.h"
#include "ReportHIV.h"
#include "SimulationConfig.h"
#include "HivObjectFactory.h"
#include "IHIVCascadeStateIntervention.h"
#include "HIVReportEventRecorder.h"
#include "IndividualHIV.h"

static const char * _module = "SimulationHIV";

namespace Kernel
{
    GET_SCHEMA_STATIC_WRAPPER_IMPL(SimulationHIV,SimulationHIV)
    BEGIN_QUERY_INTERFACE_BODY(SimulationHIV)
        HANDLE_INTERFACE(IGlobalContext)
    END_QUERY_INTERFACE_BODY(SimulationHIV)

    SimulationHIV::SimulationHIV()
    : report_hiv_by_age_and_gender(false)
    , report_hiv_ART(false)
    , report_hiv_infection(false)
    , report_hiv_mortality(false)
    , report_hiv_period(DAYSPERYEAR)
    , valid_cascade_states()
    {
        initConfigTypeMap( "Report_HIV_ByAgeAndGender",     &report_hiv_by_age_and_gender,   Report_HIV_ByAgeAndGender_DESC_TEXT,  false);
        initConfigTypeMap( "Report_HIV_ART",                &report_hiv_ART,                 Report_HIV_ART_DESC_TEXT,  false);
        initConfigTypeMap( "Report_HIV_Infection",          &report_hiv_infection,           Report_HIV_Infection_DESC_TEXT,  false);
        initConfigTypeMap( "Report_HIV_Mortality",          &report_hiv_mortality,           Report_HIV_Mortality_DESC_TEXT,  false);
        initConfigTypeMap( "Report_HIV_Period",             &report_hiv_period,              Report_HIV_Period_DESC_TEXT,     30.0, 36500.0, 730.0);
        initConfigTypeMap( "Valid_Cascade_States",          &valid_cascade_states,           Report_HIV_Valid_Cascade_States_DESC_TEXT);

        reportClassCreator = ReportHIV::CreateReport;
        eventReportClassCreator = HIVReportEventRecorder::CreateReport;
    }

    SimulationHIV::~SimulationHIV(void)
    {
        LOG_DEBUG("Deleting ~SimulationHIV" );
    }

    SimulationHIV *SimulationHIV::CreateSimulation()
    {
        SimulationHIV *newsimulation = _new_ SimulationHIV();
        newsimulation->Initialize();

        InterventionValidator::SetDiseaseSpecificValidator( newsimulation );

        return newsimulation;
    }

    SimulationHIV *SimulationHIV::CreateSimulation(const ::Configuration *config)
    {
        SimulationHIV *newsimulation = _new_ SimulationHIV();
        if (newsimulation)
        {
            InterventionValidator::SetDiseaseSpecificValidator( newsimulation );

            // This sequence is important: first
            // Creation-->Initialization-->Validation
            newsimulation->Initialize(config);
            if(!ValidateConfiguration(config))
            {
                delete newsimulation;
                newsimulation = nullptr;
            }
        }

        return newsimulation;
    }

    void
    SimulationHIV::Initialize()
    {
        return SimulationSTI::Initialize();
    }

    void
    SimulationHIV::Initialize(
        const ::Configuration *config
    )
    {
        SimulationSTI::Initialize(config);

        release_assert( GET_CONFIGURABLE(SimulationConfig) );
        if( GET_CONFIGURABLE(SimulationConfig)->vital_dynamics == 0 )
        {
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Enable_Vital_Dynamics", "0", "Simulation_Type", "HIV_SIM", "Mortality must be on for HIV." );
        }

        IndividualHumanHIV fakeHumanHIV;
        LOG_INFO( "Calling Configure on fakeHumanHIV\n" );
        fakeHumanHIV.Configure( config );
    }

    bool
    SimulationHIV::Configure(
        const Configuration * inputJson
    )
    {
        bool ret = SimulationSTI::Configure( inputJson );
        return ret;
    }

    void
    SimulationHIV::Reports_CreateBuiltIn()
    {
        SimulationSTI::Reports_CreateBuiltIn();

        if (report_relationship_start)
        {
            LOG_INFO( "Using HIV RelationshipStartReporter.\n" );
            reports.push_back(HivObjectFactory::CreateRelationshipStartReporter(this));
        }

        if (report_transmission)
        {
            LOG_INFO( "Using HIV TransmissionReporter.\n" );
            reports.push_back(HivObjectFactory::CreateTransmissionReporter(this));
        }

        if (report_hiv_mortality)
        {
            LOG_INFO( "Using HIV Mortality Reporter.\n" );
            reports.push_back(HivObjectFactory::CreateHIVMortalityReporter(this));
        }

        if (report_hiv_by_age_and_gender)
        {
            LOG_INFO( "Using HIV ByAgeAndGender Reporter.\n" );
            reports.push_back(HivObjectFactory::CreateHIVByAgeAndGenderReporter(this,report_hiv_period));
        }

        if (report_hiv_ART)
        {
            LOG_INFO( "Using HIV ART Reporter.\n" );
            reports.push_back(HivObjectFactory::CreateHIVARTReporter(this));
        }

        if (report_hiv_infection)
        {
            LOG_INFO( "Using HIV Infection Reporter.\n" );
            reports.push_back(HivObjectFactory::CreateHIVInfectionReporter(this));
        }

        // set so the header can have extra information
        for( auto report : reports )
        {
            BaseChannelReport* p_bcr = dynamic_cast<BaseChannelReport*>(report);
            if( p_bcr != nullptr )
            {
                p_bcr->SetAugmentor( this );
            }
        }
    }

    bool SimulationHIV::ValidateConfiguration(const ::Configuration *config)
    {
        if (!Simulation::ValidateConfiguration(config))
            return false;

        // TODO: any disease-specific validation goes here.
        // Warning: static climate parameters are not configured until after this function is called

        return true;
    }

    void SimulationHIV::addNewNodeFromDemographics(suids::suid node_suid, NodeDemographicsFactory *nodedemographics_factory, ClimateFactory *climate_factory)
    {
        NodeHIV *node = NodeHIV::CreateNode(this, node_suid);
        addNode_internal(node, nodedemographics_factory, climate_factory);
    }

    void SimulationHIV::Validate( const std::string& rClassName,
                                  IDistributableIntervention* pInterventionToValidate )
    {
        IHIVCascadeStateIntervention* pInterventionWithStates = nullptr;
        if (pInterventionToValidate->QueryInterface(GET_IID(IHIVCascadeStateIntervention), (void**)&pInterventionWithStates) == s_OK)
        {
            Validate( rClassName, pInterventionWithStates );
        }
    }

    void SimulationHIV::Validate( const std::string& rClassName,
                                  INodeDistributableIntervention* pInterventionToValidate )
    {
        IHIVCascadeStateIntervention* pInterventionWithStates = nullptr;
        if (pInterventionToValidate->QueryInterface(GET_IID(IHIVCascadeStateIntervention), (void**)&pInterventionWithStates) == s_OK)
        {
            Validate( rClassName, pInterventionWithStates );
        }
    }

    void SimulationHIV::Validate( const std::string& rClassName,
                                  IHIVCascadeStateIntervention* pInterventionWithStates )
    {
        // Validate Intervention's Cascade State
        const std::string& r_cascade_state = pInterventionWithStates->GetCascadeState();
        CheckState( rClassName, "Cascade", r_cascade_state );

        // Validate Interventions Abort States
        const auto& r_abort_states = pInterventionWithStates->GetAbortStates();
        for( auto& r_state : r_abort_states )
        {
            CheckState( rClassName, "Abort", r_state );
        }
    }

    void SimulationHIV::CheckState( const std::string& rClassName,
                                    const char* pVarName,
                                    const std::string& rState )
    {
        if( !((valid_cascade_states.size() <= 0) && rState.empty()) && 
             (valid_cascade_states.find( rState ) == valid_cascade_states.end()) )
        {
            std::stringstream ss ;
            ss << "Error creating the intervention '" << rClassName << "'.  " ;
            ss << "The " << pVarName << " State (" << rState << ") is not a valid state." ;
            throw GeneralConfigurationException( __FILE__, __LINE__, __FUNCTION__, ss.str().c_str() );
        }
    }

    void SimulationHIV::AddDataToHeader( IJsonObjectAdapter* pIJsonObj )
    {
        pIJsonObj->Insert("Base_Year", SimulationSTI::base_year);
    }
}
