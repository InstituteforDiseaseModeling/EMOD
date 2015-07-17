/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include "IMalariaAntibody.h"
#include "BoostLibWrapper.h"
#include "Log.h"

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
        virtual void  Decay( float dt, SusceptibilityMalariaConfig* params );
        virtual float StimulateCytokines( float dt, float inv_uL_blood ); // TODO: inv_uL_blood arguments can be removed when we switch to m_antigen_concentration from m_antigen_count
        virtual void  UpdateAntibodyCapacity( float dt, SusceptibilityMalariaConfig* params, float inv_uL_blood );
        virtual void  UpdateAntibodyCapacity( float dt, float growth_rate );
        virtual void  UpdateAntibodyConcentration( float dt, SusceptibilityMalariaConfig* params);
        virtual void  ResetCounters();

        virtual void  IncreaseAntigenCount( int64_t antigenCount ); // TODO: rename to IncreaseAntigenConcentration( float antigenConcentration )
        virtual void  SetAntigenicPresence( bool antigenPresent );

        virtual int64_t GetAntigenCount() const;
        virtual bool    GetAntigenicPresence() const;
        virtual float   GetAntibodyCapacity() const;
        virtual float   GetAntibodyConcentration() const;

        virtual void    SetAntibodyCapacity( float antibody_capacity );
        virtual void    SetAntibodyConcentration(float antibody_concentration);

        virtual MalariaAntibodyType::Enum GetAntibodyType() const;
        virtual int GetAntibodyVariant() const;

    protected:
        float   m_antibody_capacity;
        float   m_antibody_concentration;
        int64_t m_antigen_count;            // TODO: change to "float m_antigen_concentration;"
        bool    m_antigen_present;          // TODO: resolve redundancy with previous data member

        MalariaAntibodyType::Enum m_antibody_type;
        int m_antibody_variant;

        MalariaAntibody();
        void Initialize( MalariaAntibodyType::Enum type, int variant, float capacity = 0, float concentration = 0 );

#if USE_JSON_SERIALIZATION || USE_JSON_MPI
    public:
        // IJsonSerializable Interfaces
        virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
        virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );
#endif

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
    private:
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, MalariaAntibody &ab, const unsigned int v);
#endif
    };

    // -----------------------------------------------------------

    class MalariaAntibodyCSP : public MalariaAntibody
    {
    public:
        static IMalariaAntibody* CreateAntibody( int variant, float capacity=0.0f );
        virtual void UpdateAntibodyConcentration( float dt, SusceptibilityMalariaConfig* params );
        virtual void Decay( float dt, SusceptibilityMalariaConfig* params );

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
    private:
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, MalariaAntibodyCSP &ab, const unsigned int v);
#endif

#if USE_JSON_SERIALIZATION || USE_JSON_MPI
    public:
        virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
        virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );
#endif
    };

    class MalariaAntibodyMSP : public MalariaAntibody
    {
    public:
        static IMalariaAntibody* CreateAntibody( int variant, float capacity=0.0f );

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
    private:
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, MalariaAntibodyMSP &ab, const unsigned int v);
#endif

#if USE_JSON_SERIALIZATION || USE_JSON_MPI
    public:
        virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
        virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );
#endif
    };

    class MalariaAntibodyPfEMP1Minor : public MalariaAntibody
    {
    public:
        static IMalariaAntibody* CreateAntibody( int variant, float capacity=0.0f );
        virtual void UpdateAntibodyCapacity( float dt, SusceptibilityMalariaConfig* params, float inv_uL_blood );

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
    private:
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, MalariaAntibodyPfEMP1Minor &ab, const unsigned int v);
#endif

#if USE_JSON_SERIALIZATION || USE_JSON_MPI
    public:
        virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
        virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );
#endif
    };

    class MalariaAntibodyPfEMP1Major : public MalariaAntibody
    {
    public:
        static IMalariaAntibody* CreateAntibody( int variant, float capacity=0.0f );
        virtual void UpdateAntibodyCapacity( float dt, SusceptibilityMalariaConfig* params, float inv_uL_blood );

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
    private:
        friend class ::boost::serialization::access;
        template<class Archive>
        friend void serialize(Archive &ar, MalariaAntibodyPfEMP1Major &ab, const unsigned int v);
#endif

#if USE_JSON_SERIALIZATION || USE_JSON_MPI
    public:
        virtual void JSerialize( IJsonObjectAdapter* root, JSerializer* helper ) const;
        virtual void JDeserialize( IJsonObjectAdapter* root, JSerializer* helper );
#endif
    };
}
