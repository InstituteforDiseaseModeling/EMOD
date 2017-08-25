/***************************************************************************************************

Copyright (c) 2017 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "BoostLibWrapper.h"

#include "Common.h"

#include "MalariaContexts.h"
#include "NodeVector.h"
#include "IndividualMalaria.h"

namespace Kernel
{
    class NodeMalaria : public NodeVector, INodeMalaria
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        static NodeMalaria *CreateNode(ISimulationContext *simulation, suids::suid suid);
        virtual ~NodeMalaria();

        virtual float GetParasitePositive()         const override { return m_Parasite_positive; }
        virtual float GetLogParasites()             const override { return m_Log_parasites; }
        virtual float GetFeverPositive()            const override { return m_Fever_positive; }
        virtual float GetNewClinicalCases()         const override { return m_New_Clinical_Cases; } 
        virtual float GetNewSevereCases()           const override { return m_New_Severe_Cases; }
        virtual float GetParasitePrevalence()       const override { return m_Parasite_Prevalence; }
        virtual float GetNewDiagnosticPositive()    const override { return m_New_Diagnostic_Positive; }
        virtual float GetNewDiagnosticPrevalence()  const override { return m_New_Diagnostic_Prevalence; }
        virtual float GetGeometricMeanParasitemia() const override { return m_Geometric_Mean_Parasitemia; }
        virtual float GetFeverPrevalence()          const override { return m_Fever_Prevalence; }
        virtual float GetMaternalAntibodyFraction() const override { return m_Maternal_Antibody_Fraction; }
        virtual const NodeDemographicsDistribution* GetMSP_mean_antibody_distribution()         const override { return MSP_mean_antibody_distribution;};
        virtual const NodeDemographicsDistribution* GetNonspec_mean_antibody_distribution()     const override { return nonspec_mean_antibody_distribution; };
        virtual const NodeDemographicsDistribution* GetPfEMP1_mean_antibody_distribution()      const override { return PfEMP1_mean_antibody_distribution; };
        virtual const NodeDemographicsDistribution* GetMSP_variance_antibody_distribution()     const override { return MSP_variance_antibody_distribution; };
        virtual const NodeDemographicsDistribution* GetNonspec_variance_antibody_distribution() const override { return nonspec_variance_antibody_distribution; };
        virtual const NodeDemographicsDistribution* GetPfEMP1_variance_antibody_distribution()  const override { return PfEMP1_variance_antibody_distribution; };

        virtual bool Configure( const Configuration* config ) override;

        virtual IIndividualHuman* addNewIndividual( float = 1.0f, float = 0.0f, int = 0, int = 0, float = 1.0f, float = 1.0f, float = 1.0f, float = 0) override;

    protected:
        float m_Parasite_positive;
        float m_Log_parasites;
        float m_Fever_positive;
        float m_New_Clinical_Cases; 
        float m_New_Severe_Cases;
        float m_Parasite_Prevalence;
        float m_New_Diagnostic_Positive;
        float m_New_Diagnostic_Prevalence;
        float m_Geometric_Mean_Parasitemia;
        float m_Fever_Prevalence;
        float m_Maternal_Antibody_Fraction;

        NodeDemographicsDistribution* MSP_mean_antibody_distribution;
        NodeDemographicsDistribution* nonspec_mean_antibody_distribution;
        NodeDemographicsDistribution* PfEMP1_mean_antibody_distribution;
        NodeDemographicsDistribution* MSP_variance_antibody_distribution;
        NodeDemographicsDistribution* nonspec_variance_antibody_distribution;
        NodeDemographicsDistribution* PfEMP1_variance_antibody_distribution;

        virtual void LoadImmunityDemographicsDistribution() override;
        virtual float drawInitialImmunity(float ind_init_age) override;

        virtual IIndividualHuman* createHuman( suids::suid id, float MCweight, float init_age, int gender, float init_poverty) override;

        virtual void updatePopulationStatistics(float = 1.0f) override;
        virtual void accumulateIndividualPopulationStatistics(float dt, IIndividualHuman* individual);
        virtual void updateNodeStateCounters(IIndividualHuman *ih);
        virtual void resetNodeStateCounters(void) override;

        virtual void setupEventContextHost() override;
        /* clorton virtual */ const SimulationConfig *params() /* clorton override */;

        DECLARE_SERIALIZABLE(NodeMalaria);
        
    private:
        NodeMalaria();
        NodeMalaria(ISimulationContext *simulation, suids::suid suid);
        virtual void Initialize() override;

        virtual INodeContext *getContextPointer() override { return static_cast<INodeContext*>(this); }
    };
}
