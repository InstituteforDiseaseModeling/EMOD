/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once
#include "SusceptibilityAirborne.h"

namespace Kernel
{
    
    class SusceptibilityTBConfig: public JsonConfigurable
    {
        GET_SCHEMA_STATIC_WRAPPER(SusceptibilityTBConfig)
        friend class IndividualHumanTB;
        friend class IndividualHumanCoinfection;

    public:
        virtual bool Configure( const Configuration* config );
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()
        static float TB_immune_loss_fraction;
        static float TB_fast_progressor_fraction_child;
        static float TB_fast_progressor_fraction_adult;
        static float TB_smear_positive_fraction_child;
        static float TB_smear_positive_fraction_adult;
        static float TB_extrapulmonary_fraction_child;
        static float TB_extrapulmonary_fraction_adult;
    };

    
    class ISusceptibilityTB : public ISupports
    {
    public:
        virtual float GetFastProgressorFraction() = 0;
        virtual float GetSmearPositiveFraction() = 0;
        virtual float GetExtraPulmonaryFraction() = 0;
        virtual float GetCoughInfectiousness() = 0;
        virtual bool GetCD4ActFlag() const = 0;
        virtual void SetCD4ActFlag( bool bin) = 0;
    };

    class SusceptibilityTB : public SusceptibilityAirborne, public ISusceptibilityTB, SusceptibilityTBConfig
    {
    public:
        friend class IndividualHumanTB;
        friend class IndividualHumanCoinfection;
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()

        static SusceptibilityTB *CreateSusceptibility(IIndividualHumanContext *context, float age, float immmod, float riskmod);
        virtual ~SusceptibilityTB(void);

        virtual void Update(float dt = 0.0);
        virtual void UpdateInfectionCleared();
        virtual float GetFastProgressorFraction();
        virtual float GetSmearPositiveFraction();
        virtual float GetExtraPulmonaryFraction();
        virtual void  InitNewInfection();
        virtual bool  IsImmune() const;
        virtual float GetCoughInfectiousness();
        virtual bool GetCD4ActFlag() const;
        virtual void SetCD4ActFlag(bool bin);

    protected:
        bool  Flag_use_CD4_for_act;
        SusceptibilityTB();
        SusceptibilityTB(IIndividualHumanContext *context);

        void Initialize(float age, float immmod, float riskmod);

        // additional members of SusceptibilityTB
        bool m_is_immune_competent;
        bool m_is_immune;

        // keeping count of current infections to know when to begin immunity loss.  
        // TODO: alternatively, could add an interface, ITBHumanContext, with a function checking if infected?
        int  m_current_infections;  
        float m_cough_infectiousness;

    private:

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        friend class boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive & ar, SusceptibilityTB &sus, const unsigned int  file_version );
#endif
    };
}
