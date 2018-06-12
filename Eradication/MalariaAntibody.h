/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#pragma once

#include "IMalariaAntibody.h"

namespace Kernel
{
    class SusceptibilityMalariaConfig;

    class MalariaAntibody : public IMalariaAntibody
    {
    public:

        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_ONLY_REFERENCE_COUNTING()

        virtual ~MalariaAntibody() {}

    public:
        // IMalariaAntibody methods
        virtual void  Decay( float dt ) override;
        virtual float StimulateCytokines( float dt, float inv_uL_blood ) override;  // TODO: inv_uL_blood arguments can be removed when we switch to m_antigen_concentration from m_antigen_count
        virtual void  UpdateAntibodyCapacity( float dt, float inv_uL_blood ) override;
        virtual void  UpdateAntibodyCapacityByRate( float dt, float growth_rate ) override;
        virtual void  UpdateAntibodyConcentration( float dt ) override;
        virtual void  ResetCounters() override;

        virtual void  IncreaseAntigenCount( int64_t antigenCount ) override;    // TODO: rename to IncreaseAntigenConcentration( float antigenConcentration )
        virtual void  SetAntigenicPresence( bool antigenPresent ) override;

        virtual int64_t GetAntigenCount() const override;
        virtual bool    GetAntigenicPresence() const override;
        virtual float   GetAntibodyCapacity() const override;
        virtual float   GetAntibodyConcentration() const override;

        virtual void    SetAntibodyCapacity( float antibody_capacity ) override;
        virtual void    SetAntibodyConcentration(float antibody_concentration) override;

        virtual MalariaAntibodyType::Enum GetAntibodyType() const override;
        virtual int GetAntibodyVariant() const override;

    protected:
        float   m_antibody_capacity;
        float   m_antibody_concentration;
        int64_t m_antigen_count;            // TODO: change to "float m_antigen_concentration;"
        bool    m_antigen_present;          // TODO: resolve redundancy with previous data member

        MalariaAntibodyType::Enum m_antibody_type;
        int m_antibody_variant;

        MalariaAntibody();
        void Initialize( MalariaAntibodyType::Enum type, int variant, float capacity = 0, float concentration = 0 );

        DECLARE_SERIALIZABLE(MalariaAntibody);
    };

    // -----------------------------------------------------------

    class MalariaAntibodyCSP : public MalariaAntibody
    {
    public:
        static IMalariaAntibody* CreateAntibody( int variant, float capacity=0.0f );
        virtual void UpdateAntibodyConcentration( float dt ) override;
        virtual void Decay( float dt ) override;

    protected:
        DECLARE_SERIALIZABLE(MalariaAntibodyCSP);
    };

    class MalariaAntibodyMSP : public MalariaAntibody
    {
    public:
        static IMalariaAntibody* CreateAntibody( int variant, float capacity=0.0f );

    protected:
        DECLARE_SERIALIZABLE(MalariaAntibodyMSP);
    };

    class MalariaAntibodyPfEMP1Minor : public MalariaAntibody
    {
    public:
        static IMalariaAntibody* CreateAntibody( int variant, float capacity=0.0f );
        virtual void UpdateAntibodyCapacity( float dt, float inv_uL_blood ) override;

    protected:
        DECLARE_SERIALIZABLE(MalariaAntibodyPfEMP1Minor);
    };

    class MalariaAntibodyPfEMP1Major : public MalariaAntibody
    {
    public:
        static IMalariaAntibody* CreateAntibody( int variant, float capacity=0.0f );
        virtual void UpdateAntibodyCapacity( float dt, float inv_uL_blood ) override;

    protected:
        DECLARE_SERIALIZABLE(MalariaAntibodyPfEMP1Major);
    };
}
