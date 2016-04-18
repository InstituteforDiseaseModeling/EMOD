/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"

#include "NodeMalaria.h"
#include "IndividualMalaria.h"

#include "Common.h"
#include "Malaria.h"
#include "NodeMalariaEventContext.h"
#include "SimulationConfig.h"
#include "MathFunctions.h"

static const char* _module = "NodeMalaria";

namespace Kernel
{
    // QI stuff in case we want to use it more extensively
    BEGIN_QUERY_INTERFACE_DERIVED(NodeMalaria, NodeVector)
        HANDLE_INTERFACE(INodeMalaria)
    END_QUERY_INTERFACE_DERIVED(NodeMalaria, NodeVector)

    NodeMalaria::NodeMalaria() : NodeVector(),
        m_Parasite_positive(0),
        m_Log_parasites(0),
        m_Fever_positive(0),
        m_New_Clinical_Cases(0),
        m_New_Severe_Cases(0),
        m_Parasite_Prevalence(0),
        m_New_Diagnostic_Positive(0),
        m_New_Diagnostic_Prevalence(0),
        m_Geometric_Mean_Parasitemia(0),
        m_Fever_Prevalence(0),
        m_Maternal_Antibody_Fraction(0)
    {
    }

    NodeMalaria::NodeMalaria(ISimulationContext *simulation, suids::suid suid) : NodeVector(simulation, suid),
        m_Parasite_positive(0),
        m_Log_parasites(0),
        m_Fever_positive(0),
        m_New_Clinical_Cases(0),
        m_New_Severe_Cases(0),
        m_Parasite_Prevalence(0),
        m_New_Diagnostic_Positive(0),
        m_New_Diagnostic_Prevalence(0),
        m_Geometric_Mean_Parasitemia(0),
        m_Fever_Prevalence(0),
        m_Maternal_Antibody_Fraction(0)
    {
    }

    bool NodeMalaria::Configure( const Configuration * config )
    {
        return NodeVector::Configure( config );
    }

    void NodeMalaria::Initialize()
    {
        NodeVector::Initialize();
    }

    NodeMalaria *NodeMalaria::CreateNode(ISimulationContext *simulation, suids::suid suid)
    {
        NodeMalaria *newnode = _new_ NodeMalaria(simulation, suid);
        newnode->Initialize();

        return newnode;
    }

    NodeMalaria::~NodeMalaria()
    {
        // individual deletion done by ~Node
        // vectorpopulation deletion handled at _Vector level
    }

    IIndividualHuman* NodeMalaria::createHuman( suids::suid suid, float monte_carlo_weight, float initial_age, int gender, float poverty_level)
    {
        return IndividualHumanMalaria::CreateHuman(getContextPointer(), suid, monte_carlo_weight, initial_age, gender,  poverty_level);
    }

    IIndividualHuman* NodeMalaria::addNewIndividual( float monte_carlo_weight, float initial_age, int gender, int initial_infection_count, float immparam, float riskparam, float mighet, float init_poverty)
    {
        //VALIDATE(boost::format("NodeMalaria::addNewIndividual(%f, %f, %d, %d, %f)") % monte_carlo_weight % initial_age % gender % initial_infection_count % init_poverty);

        // just the base class for now
        return NodeVector::addNewIndividual(monte_carlo_weight, initial_age, gender, initial_infection_count, immparam, riskparam, mighet, init_poverty);
    }

    void NodeMalaria::LoadImmunityDemographicsDistribution()
    {
        demographic_distributions[NodeDemographicsDistribution::MSP_mean_antibody_distribution]         = NodeDemographicsDistribution::CreateDistribution(demographics["MSP_mean_antibody_distribution"],         "age");
        demographic_distributions[NodeDemographicsDistribution::nonspec_mean_antibody_distribution]     = NodeDemographicsDistribution::CreateDistribution(demographics["nonspec_mean_antibody_distribution"],     "age");
        demographic_distributions[NodeDemographicsDistribution::PfEMP1_mean_antibody_distribution]      = NodeDemographicsDistribution::CreateDistribution(demographics["PfEMP1_mean_antibody_distribution"],      "age");
        demographic_distributions[NodeDemographicsDistribution::MSP_variance_antibody_distribution]     = NodeDemographicsDistribution::CreateDistribution(demographics["MSP_variance_antibody_distribution"],     "age");
        demographic_distributions[NodeDemographicsDistribution::nonspec_variance_antibody_distribution] = NodeDemographicsDistribution::CreateDistribution(demographics["nonspec_variance_antibody_distribution"], "age");
        demographic_distributions[NodeDemographicsDistribution::PfEMP1_variance_antibody_distribution]  = NodeDemographicsDistribution::CreateDistribution(demographics["PfEMP1_variance_antibody_distribution"],  "age");
    }

    float NodeMalaria::drawInitialImmunity(float ind_init_age)
    {
        switch( params()->immunity_initialization_distribution_type ) 
        {
        case DistributionType::DISTRIBUTION_COMPLEX:
        case DistributionType::DISTRIBUTION_OFF:
            // For MALARIA_SIM, ImmunityDistributionFlag, ImmunityDistribution1, ImmunityDistribution2 map to Innate_Immune_Variation (e.g. variable pyrogenic threshold, cytokine killing)
            return float(Probability::getInstance()->fromDistribution(immunity_dist_type, immunity_dist1, immunity_dist2, 1.0));

        case DistributionType::DISTRIBUTION_SIMPLE:
            throw IncoherentConfigurationException( __FILE__, __LINE__, __FUNCTION__, "Immunity_Initialization_Distribution_Type", "DISTRIBUTION_SIMPLE", "Simulation_Type", "MALARIA_SIM");

        default:
            if( !JsonConfigurable::_dryrun )
            {
                throw BadEnumInSwitchStatementException( __FILE__, __LINE__, __FUNCTION__, "Immunity_Initialization_Distribution_Type", params()->immunity_initialization_distribution_type, DistributionType::pairs::lookup_key( params()->immunity_initialization_distribution_type ) );
            }
        }

        return 1.0f;
    }

    void NodeMalaria::accumulateIndividualPopulationStatistics( float dt, IIndividualHuman* basic_individual)
    {
        // Do base-class behavior, e.g. UpdateInfectiousness, statPop, Possible_Mothers
        Node::accumulateIndividualPopulationStatistics(dt, basic_individual);

        // Cast from IndividualHuman to IndividualHumanMalaria
        IMalariaHumanContext *individual = nullptr;
        if( basic_individual->QueryInterface( GET_IID( IMalariaHumanContext ), (void**)&individual ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IndividualHumanMalaria", "IndividualHuman" );
        }
        float mc_weight = float(basic_individual->GetMonteCarloWeight());

        // NOTE: always perform the malaria test (so we have a definite number to report), but
        // once Node is not longer the one accumulating parasite-counts/positives, we can remove
        // the call to GetParasiteCountByType() and just do that in the reports.
        individual->PerformMalariaTest(MALARIA_TEST_BLOOD_SMEAR);
        float tempParasiteCount = individual->CheckParasiteCountWithTest(MALARIA_TEST_BLOOD_SMEAR);
        //LOG_DEBUG_F( "tempParasiteCount = %f\n", tempParasiteCount );

        if (tempParasiteCount > 0)
        {
            m_Parasite_positive += mc_weight;
            m_Log_parasites     += mc_weight * log(tempParasiteCount);
        }

        individual->PerformMalariaTest(MALARIA_TEST_NEW_DIAGNOSTIC);
        if (individual->CheckForParasitesWithTest(MALARIA_TEST_NEW_DIAGNOSTIC))
        {
            m_New_Diagnostic_Positive += mc_weight;
        }

        if ( individual->HasFever() )
        {
            m_Fever_positive += mc_weight;
        }

        if ( basic_individual->IsPossibleMother() )
        {
            m_Maternal_Antibody_Fraction += mc_weight * individual->GetMalariaSusceptibilityContext()->get_fraction_of_variants_with_antibodies(MalariaAntibodyType::PfEMP1_major);
        }
    }

    void NodeMalaria::updatePopulationStatistics(float dt)
    {
        // Update population statistics and vector lifecycle probabilities as in base class
        NodeVector::updatePopulationStatistics(dt);

        // normalize and calculate a few more derived quantities
        if ( statPop > 0 )
        {
            m_Parasite_Prevalence        = m_Parasite_positive       / statPop;
            //LOG_DEBUG_F( "m_Parasite_Prevalence = %f/%f = %f\n", m_Parasite_positive, statPop, m_Parasite_Prevalence );
            m_New_Diagnostic_Prevalence  = m_New_Diagnostic_Positive / statPop;
            m_Fever_Prevalence           = m_Fever_positive          / statPop;

            m_Geometric_Mean_Parasitemia = (m_Parasite_positive > 0) ? exp(m_Log_parasites / m_Parasite_positive) : 0;
        }

        if ( Possible_Mothers > 0 )
        {
            m_Maternal_Antibody_Fraction /= Possible_Mothers;
        }
    }

    void NodeMalaria::updateNodeStateCounters( IIndividualHuman *ih)
    {
        float weight = ih->GetMonteCarloWeight();
        IMalariaHumanContext *malaria_human = nullptr;
        if( ih->QueryInterface( GET_IID( IMalariaHumanContext ), (void**) &malaria_human ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "ih", "IndividualHuman", "IndividualHumanMalaria" );
        }

        m_New_Clinical_Cases += weight * malaria_human->HasClinicalSymptom(ClinicalSymptomsEnum::CLINICAL_DISEASE);
        m_New_Severe_Cases   += weight * malaria_human->HasClinicalSymptom(ClinicalSymptomsEnum::SEVERE_DISEASE);

        malaria_human->GetMalariaSusceptibilityContext()->ResetMaximumSymptoms();

        Node::updateNodeStateCounters(ih); // this must be at the end, since it clears the NewInfectionState
        //malaria_human->Release();
    }

    void NodeMalaria::resetNodeStateCounters()
    {
        NodeVector::resetNodeStateCounters();

        m_Parasite_positive       = 0;
        m_Log_parasites           = 0;
        m_New_Diagnostic_Positive = 0;
        m_Fever_positive          = 0;

        m_Parasite_Prevalence        = 0;
        m_New_Diagnostic_Prevalence  = 0;
        m_Geometric_Mean_Parasitemia = 0;
        m_Fever_Prevalence           = 0;

        m_New_Clinical_Cases      = 0;
        m_New_Severe_Cases        = 0;

        m_Maternal_Antibody_Fraction = 0;
    }

    const SimulationConfig* NodeMalaria::params()
    {
        return GET_CONFIGURABLE(SimulationConfig);
    }

    void NodeMalaria::setupEventContextHost()
    {
        event_context_host = _new_ NodeMalariaEventContextHost(this);
    }

    REGISTER_SERIALIZABLE(NodeMalaria);

    void NodeMalaria::serialize(IArchive& ar, NodeMalaria* obj)
    {
        NodeVector::serialize(ar, obj);
        NodeMalaria& node = *obj;
        ar.labelElement("m_Parasite_positive") & node.m_Parasite_positive;
        ar.labelElement("m_Log_parasites") & node.m_Log_parasites;
        ar.labelElement("m_Fever_positive") & node.m_Fever_positive;
        ar.labelElement("m_New_Clinical_Cases") & node.m_New_Clinical_Cases;
        ar.labelElement("m_New_Severe_Cases") & node.m_New_Severe_Cases;
        ar.labelElement("m_Parasite_Prevalence") & node.m_Parasite_Prevalence;
        ar.labelElement("m_New_Diagnostic_Positive") & node.m_New_Diagnostic_Positive;
        ar.labelElement("m_New_Diagnostic_Prevalence") & node.m_New_Diagnostic_Prevalence;
        ar.labelElement("m_Geometric_Mean_Parasitemia") & node.m_Geometric_Mean_Parasitemia;
        ar.labelElement("m_Fever_Prevalence") & node.m_Fever_Prevalence;
        ar.labelElement("m_Maternal_Antibody_Fraction") & node.m_Maternal_Antibody_Fraction;
    }
}
