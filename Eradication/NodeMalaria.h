/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "BoostLibWrapper.h"

#include "Common.h"

#include "MalariaContexts.h"
#include "NodeVector.h"
#include "IndividualMalaria.h"
#include "InterventionEnums.h"

namespace Kernel
{
    class NodeMalaria : public NodeVector, INodeMalaria
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        static NodeMalaria *CreateNode(ISimulationContext *simulation, suids::suid suid);
        virtual ~NodeMalaria();

        virtual float GetParasitePositive()         const { return m_Parasite_positive; }
        virtual float GetLogParasites()             const { return m_Log_parasites; }
        virtual float GetFeverPositive()            const { return m_Fever_positive; }
        virtual float GetNewClinicalCases()         const { return m_New_Clinical_Cases; } 
        virtual float GetNewSevereCases()           const { return m_New_Severe_Cases; }
        virtual float GetParasitePrevalence()       const { return m_Parasite_Prevalence; }
        virtual float GetNewDiagnosticPositive()    const { return m_New_Diagnostic_Positive; }
        virtual float GetNewDiagnosticPrevalence()  const { return m_New_Diagnostic_Prevalence; }
        virtual float GetGeometricMeanParasitemia() const { return m_Geometric_Mean_Parasitemia; }
        virtual float GetFeverPrevalence()          const { return m_Fever_Prevalence; }
        virtual float GetMaternalAntibodyFraction() const { return m_Maternal_Antibody_Fraction; }

        virtual bool Configure( const Configuration* config );

        virtual IndividualHuman *addNewIndividual(float = 1.0f, float = 0.0f, int = 0, int = 0, float = 1.0f, float = 1.0f, float = 1.0f, float = 0);

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

        virtual void populateNewIndividualsFromDemographics(int count_new_individuals);
        virtual IndividualHuman *createHuman(suids::suid id, float MCweight, float init_age, int gender, float init_poverty);

        virtual void updatePopulationStatistics(float = 1.0f);
        virtual void accumulateIndividualPopulationStatistics(float dt, IndividualHuman* individual);
        virtual void updateNodeStateCounters(IndividualHuman *ih);
        virtual void resetNodeStateCounters(void);

        virtual void setupEventContextHost();
        const SimulationConfig *params();

    private:
        NodeMalaria();
        NodeMalaria(ISimulationContext *simulation, suids::suid suid);
        void Initialize();

        virtual INodeContext *getContextPointer() { return (INodeContext*)this; }

#if USE_BOOST_SERIALIZATION
        friend class boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive & ar, NodeMalaria& node, const unsigned int  file_version );
#endif
    };
}
