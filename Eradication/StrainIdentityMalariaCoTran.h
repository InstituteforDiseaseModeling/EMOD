
#pragma once

#include "IStrainIdentity.h"

namespace Kernel
{
    struct TransmittedInfection
    {
        uint32_t infection_id;
        float    gametocyte_density;

        TransmittedInfection();
        TransmittedInfection( uint32_t infection_id, float gameDen );

        static void serialize( IArchive& ar, TransmittedInfection& inf );
    };

    struct AcquiredInfection
    {
        uint32_t infection_id;

        AcquiredInfection( uint32_t infection_id=0 );

        static void serialize( IArchive& ar, AcquiredInfection& inf );
    };

    class StrainIdentityMalariaCoTran : public IStrainIdentity
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
    public:
        StrainIdentityMalariaCoTran( uint32_t geneticID = 0, uint32_t transPersonID = 0 );
        StrainIdentityMalariaCoTran( const StrainIdentityMalariaCoTran& rMaster );
        virtual ~StrainIdentityMalariaCoTran(void);

        // IStrainIdentity methods
        virtual IStrainIdentity* Clone() const;
        virtual int  GetAntigenID(void) const;
        virtual int  GetGeneticID(void) const;
        virtual void SetAntigenID(int in_antigenID);
        virtual void SetGeneticID(int in_geneticID);
        virtual bool ResolveInfectingStrain( IStrainIdentity* strainId ) const;

        // other
        uint32_t GetTransmittedPersonID() const;
        void SetTransmittedPersonID( uint32_t tranPersonID );
        void AddTransmittedInfection( uint32_t infectionID, float gametocyteDensity );
        const std::vector<TransmittedInfection>& GetTransmittedInfections() const;
        void SetTransmittedData( const StrainIdentityMalariaCoTran& rTrans );

        uint32_t GetVectorID() const;
        void SetVectorID( uint32_t vectorID );
        float GetTimeOfVectorInfection() const;
        void SetTimeOfVectorInfection( float time );

        uint32_t GetAcquiredPersonID() const;
        void SetAcquiredPersonID( uint32_t acqPersonID );
        void AddAcquiredInfection( uint32_t infectionID );
        const std::vector<AcquiredInfection>& GetAcquiredInfections() const;

        void Clear();

        DECLARE_SERIALIZABLE(StrainIdentityMalariaCoTran);

    protected:
        uint32_t m_GeneticID; // for StrainAwareTransmissionGroups - person or vector ID
        uint32_t m_TransmittedPersonID;
        float    m_TimeOfVectorInfection;
        uint32_t m_VectorID;
        uint32_t m_AcquiredPersonID;
        std::vector<TransmittedInfection> m_TransmittedInfections;
        std::vector<AcquiredInfection> m_AcquiredInfections;
    };
}
