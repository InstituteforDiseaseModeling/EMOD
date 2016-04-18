/***************************************************************************************************

Copyright (c) 2016 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "VectorContexts.h"
#include "ISupports.h"
#include "MalariaEnums.h"     // for ClinicalSymptomsEnum, MalariaAntibodyType enum
#include "IMalariaAntibody.h"     // for ClinicalSymptomsEnum, MalariaAntibodyType enum

namespace Kernel
{
    struct IMalariaAntibody;

    class INodeMalaria : public ISupports
    {
    public:
        virtual float GetParasitePositive()         const = 0;
        virtual float GetLogParasites()             const = 0;
        virtual float GetFeverPositive()            const = 0;
        virtual float GetNewClinicalCases()         const = 0;
        virtual float GetNewSevereCases()           const = 0;
        virtual float GetParasitePrevalence()       const = 0;
        virtual float GetNewDiagnosticPositive()    const = 0;
        virtual float GetNewDiagnosticPrevalence()  const = 0;
        virtual float GetGeometricMeanParasitemia() const = 0;
        virtual float GetFeverPrevalence()          const = 0;
        virtual float GetMaternalAntibodyFraction() const = 0;
    };

    struct IMalariaSusceptibility : public ISupports
    {
        virtual float  get_fever()              const = 0;
        virtual float  get_fever_celsius()      const = 0;
        virtual float  get_cytokines()          const = 0;
        virtual double get_RBC_availability()   const = 0;
        virtual float  get_parasite_density()   const = 0;
        virtual float  get_inv_microliters_blood() const = 0;
        virtual void   ResetMaximumSymptoms()         = 0;
        virtual float  GetMaxFever()            const = 0;
        virtual float  GetMaxParasiteDensity()  const = 0;
        virtual float  GetHemoglobin()          const = 0;
        virtual bool   CheckForParasitesWithTest( int test_type ) const = 0;
        virtual float  CheckParasiteCountWithTest( int test_type ) const = 0;
        virtual float  get_fraction_of_variants_with_antibodies(MalariaAntibodyType::Enum type) const = 0;
        virtual IMalariaAntibody* RegisterAntibody( MalariaAntibodyType::Enum type, int variant, float capacity=0.0f ) = 0;
        virtual SevereCaseTypesEnum::Enum  CheckSevereCaseType() const = 0;
        virtual void BoostAntibody( MalariaAntibodyType::Enum type, int variant, float boosted_antibody_concentration ) = 0;
        virtual long long get_RBC_count()             const = 0;
        virtual void   SetAntigenPresent() = 0;
        virtual void   UpdateActiveAntibody( pfemp1_antibody_t &pfemp1_variant, int minor_variant, int major_variant ) = 0;
        virtual void   remove_RBCs(int64_t infectedAsexual, int64_t infectedGametocytes, double RBC_destruction_multiplier) = 0;
        virtual float  get_maternal_antibodies() const = 0;
        virtual void   init_maternal_antibodies(float mother_factor) = 0;
        virtual float  get_fever_killing_rate() const = 0;
    };

    struct IMalariaHumanContext : public ISupports
    {
        virtual const SimulationConfig *params() const = 0;
        virtual void  PerformMalariaTest(int test_type) = 0;
        virtual void  CountPositiveSlideFields(RANDOMBASE * rng, int nfields, float uL_per_field, int& positive_asexual_fields, int& positive_gametocyte_fields) const = 0;
        virtual bool  CheckForParasitesWithTest(int test_type) const = 0;
        virtual float CheckParasiteCountWithTest(int test_type) const = 0;
        virtual float CheckGametocyteCountWithTest(int test_type) const = 0; // the value with sensitivity and variability of a blood test
        virtual float GetGametocyteDensity() const = 0; // the "true" value in the model
        virtual bool  HasFever() const = 0;
        virtual void  AddClinicalSymptom(ClinicalSymptomsEnum::Enum symptom) = 0;
        virtual bool  HasClinicalSymptom(ClinicalSymptomsEnum::Enum symptom) const = 0;
        virtual IMalariaSusceptibility* GetMalariaSusceptibilityContext() const = 0;
        virtual std::vector< std::pair<int,int> > GetInfectingStrainIds() const = 0; // potentially this would be worth putting down in IndividualHuman (generic)
    };

    struct IMalariaHumanInfectable : public ISupports
    {
        virtual bool ChallengeWithBites( int n_infectious_bites ) = 0;
        virtual bool ChallengeWithSporozoites( int n_sporozoites ) = 0;
    };
}
