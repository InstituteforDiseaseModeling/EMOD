/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include "NodeMalaria.h"
#include "IndividualMalaria.h"

#include "Common.h"
#include "Malaria.h"
#include "NodeMalariaEventContext.h"
#include "SimulationConfig.h"

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

    IndividualHuman *NodeMalaria::createHuman(suids::suid suid, float monte_carlo_weight, float initial_age, int gender, float poverty_level)
    {
        return IndividualHumanMalaria::CreateHuman(getContextPointer(), suid, monte_carlo_weight, initial_age, gender,  poverty_level);
    }

    IndividualHuman *NodeMalaria::addNewIndividual(float monte_carlo_weight, float initial_age, int gender, int initial_infection_count, float immparam, float riskparam, float mighet, float init_poverty)
    {
        //VALIDATE(boost::format("NodeMalaria::addNewIndividual(%f, %f, %d, %d, %f)") % monte_carlo_weight % initial_age % gender % initial_infection_count % init_poverty);

        // just the base class for now
        return NodeVector::addNewIndividual(monte_carlo_weight, initial_age, gender, initial_infection_count, immparam, riskparam, mighet, init_poverty);
    }

    void NodeMalaria::populateNewIndividualsFromDemographics(int count_new_individuals)
    {
        // Cache pointers to the initial immunity distributions with the node, so it doesn't have to be created for each individual.
        // After the demographic initialization is complete, they can be removed from the map and deleted
        if ( GET_CONFIGURABLE(SimulationConfig)->enable_immunity_initialization_distribution )
        {
            demographic_distributions[NodeDemographicsDistribution::MSP_mean_antibody_distribution]         = NodeDemographicsDistribution::CreateDistribution(demographics["MSP_mean_antibody_distribution"],         "age");
            demographic_distributions[NodeDemographicsDistribution::nonspec_mean_antibody_distribution]     = NodeDemographicsDistribution::CreateDistribution(demographics["nonspec_mean_antibody_distribution"],     "age");
            demographic_distributions[NodeDemographicsDistribution::PfEMP1_mean_antibody_distribution]      = NodeDemographicsDistribution::CreateDistribution(demographics["PfEMP1_mean_antibody_distribution"],      "age");
            demographic_distributions[NodeDemographicsDistribution::MSP_variance_antibody_distribution]     = NodeDemographicsDistribution::CreateDistribution(demographics["MSP_variance_antibody_distribution"],     "age");
            demographic_distributions[NodeDemographicsDistribution::nonspec_variance_antibody_distribution] = NodeDemographicsDistribution::CreateDistribution(demographics["nonspec_variance_antibody_distribution"], "age");
            demographic_distributions[NodeDemographicsDistribution::PfEMP1_variance_antibody_distribution]  = NodeDemographicsDistribution::CreateDistribution(demographics["PfEMP1_variance_antibody_distribution"],  "age");
        }

        // Populate the initial population
        Node::populateNewIndividualsFromDemographics(count_new_individuals);

        // Don't need these distributions after demographic initialization is completed
        // EXCEPT FOR OUTBREAK node intervention using ImportCases
        // If keeping these around has an unacceptable memory cost for very large multi-node simulations, 
        // we may reconsider how to address recreation of these distributions when they are used outside of populating new individuals
        //if ( GET_CONFIGURABLE(SimulationConfig)->enable_immunity_initialization_distribution )
        //{
        //    EraseAndDeleteDemographicsDistribution(NodeDemographicsDistribution::MSP_mean_antibody_distribution);
        //    EraseAndDeleteDemographicsDistribution(NodeDemographicsDistribution::nonspec_mean_antibody_distribution);
        //    EraseAndDeleteDemographicsDistribution(NodeDemographicsDistribution::PfEMP1_mean_antibody_distribution);
        //    EraseAndDeleteDemographicsDistribution(NodeDemographicsDistribution::MSP_variance_antibody_distribution);
        //    EraseAndDeleteDemographicsDistribution(NodeDemographicsDistribution::nonspec_variance_antibody_distribution);
        //    EraseAndDeleteDemographicsDistribution(NodeDemographicsDistribution::PfEMP1_variance_antibody_distribution);
        //}
    }

    void NodeMalaria::accumulateIndividualPopulationStatistics(float dt, IndividualHuman* basic_individual)
    {
        // Do base-class behavior, e.g. UpdateInfectiousness, statPop, Possible_Mothers
        Node::accumulateIndividualPopulationStatistics(dt, basic_individual);

        // Cast from IndividualHuman to IndividualHumanMalaria
        IMalariaHumanContext *individual = NULL;
        if( basic_individual->QueryInterface( GET_IID( IMalariaHumanContext ), (void**)&individual ) != s_OK )
        {
            throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "IndividualHumanMalaria", "IndividualHuman" );
        }
        float mc_weight = (float)basic_individual->GetMonteCarloWeight();

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

    void NodeMalaria::updateNodeStateCounters(IndividualHuman *ih)
    {
        float weight = ih->GetMonteCarloWeight();
        IMalariaHumanContext *malaria_human = NULL;
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
}

#if USE_BOOST_SERIALIZATION
BOOST_CLASS_EXPORT(Kernel::NodeMalaria)
namespace Kernel {
    template<class Archive>
    void serialize(Archive & ar, NodeMalaria& node, const unsigned int file_version )
    {
        // Register derived types - N/A
        ar.template register_type<Kernel::IndividualHumanMalaria>();

        // Serialize base class
        ar & boost::serialization::base_object<Kernel::NodeVector>(node);
    }
}
#endif
