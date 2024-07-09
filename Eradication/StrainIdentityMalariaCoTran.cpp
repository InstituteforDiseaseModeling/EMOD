
#include "stdafx.h"

#include "StrainIdentityMalariaCoTran.h"
#include "Exceptions.h"
#include "Log.h"
#include "IArchive.h"

SETUP_LOGGING( "StrainIdentityMalariaCoTran" )


namespace Kernel
{
    TransmittedInfection::TransmittedInfection()
        : TransmittedInfection( 0, 0.0f )
    {
    }

    TransmittedInfection::TransmittedInfection( uint32_t tranInfectId,
                                                float gameDen )
        : infection_id( tranInfectId )
        , gametocyte_density( gameDen )
    {
    }

    void TransmittedInfection::serialize( IArchive& ar, TransmittedInfection& info )
    {
        ar.startObject();
        ar.labelElement( "infection_id"       ) & info.infection_id;
        ar.labelElement( "gametocyte_density" ) & info.gametocyte_density;
        ar.endObject();
    }

    AcquiredInfection::AcquiredInfection( uint32_t acqInfectId )
        : infection_id( acqInfectId )
    {
    }

    void AcquiredInfection::serialize( IArchive& ar, AcquiredInfection& info )
    {
        ar.startObject();
        ar.labelElement( "infection_id" ) & info.infection_id;
        ar.endObject();
    }

    BEGIN_QUERY_INTERFACE_BODY( StrainIdentityMalariaCoTran )
        HANDLE_INTERFACE(IStrainIdentity)
    END_QUERY_INTERFACE_BODY( StrainIdentityMalariaCoTran )

    StrainIdentityMalariaCoTran::StrainIdentityMalariaCoTran( uint32_t geneticID, uint32_t transPersonID )
        : m_GeneticID( geneticID )
        , m_TransmittedPersonID( transPersonID )
        , m_TimeOfVectorInfection( 0.0f )
        , m_VectorID(0)
        , m_AcquiredPersonID( 0 )
        , m_TransmittedInfections()
        , m_AcquiredInfections()
    {
    }

    StrainIdentityMalariaCoTran::StrainIdentityMalariaCoTran( const StrainIdentityMalariaCoTran& rMaster )
        : m_GeneticID( rMaster.m_GeneticID )
        , m_TransmittedPersonID( rMaster.m_TransmittedPersonID )
        , m_TimeOfVectorInfection( rMaster.m_TimeOfVectorInfection )
        , m_VectorID( rMaster.m_VectorID )
        , m_AcquiredPersonID( rMaster.m_AcquiredPersonID )
        , m_TransmittedInfections( rMaster.m_TransmittedInfections )
        , m_AcquiredInfections( rMaster.m_AcquiredInfections )
    {
    }

    StrainIdentityMalariaCoTran::~StrainIdentityMalariaCoTran( void )
    {
    }

    IStrainIdentity* StrainIdentityMalariaCoTran::Clone() const
    {
        return new StrainIdentityMalariaCoTran( *this );
    }

    int  StrainIdentityMalariaCoTran::GetAntigenID(void) const
    {
        return 0;
    }

    int  StrainIdentityMalariaCoTran::GetGeneticID(void) const
    {
        return m_GeneticID;
    }

    void StrainIdentityMalariaCoTran::SetAntigenID(int in_antigenID)
    {
        // do nothing because it is fixed at zero
    }

    void StrainIdentityMalariaCoTran::SetGeneticID(int in_geneticID)
    {
        m_GeneticID = in_geneticID;
    }

    bool StrainIdentityMalariaCoTran::ResolveInfectingStrain( IStrainIdentity* strainId ) const
    {
        strainId->SetGeneticID( m_GeneticID );
        return true;
    }

    uint32_t StrainIdentityMalariaCoTran::GetTransmittedPersonID() const
    {
        return m_TransmittedPersonID;
    }

    void StrainIdentityMalariaCoTran::SetTransmittedPersonID( uint32_t personID )
    {
        m_TransmittedPersonID = personID;
    }

    void StrainIdentityMalariaCoTran::AddTransmittedInfection( uint32_t infectId, float gameDen )
    {
        m_TransmittedInfections.push_back( TransmittedInfection( infectId, gameDen ) );
    }

    const std::vector<TransmittedInfection>& StrainIdentityMalariaCoTran::GetTransmittedInfections() const
    {
        return m_TransmittedInfections;
    }

    void StrainIdentityMalariaCoTran::SetTransmittedData( const StrainIdentityMalariaCoTran& rTrans )
    {
        m_TransmittedPersonID   = rTrans.m_TransmittedPersonID;
        m_TransmittedInfections = rTrans.m_TransmittedInfections;
        m_VectorID              = rTrans.m_VectorID;
        m_TimeOfVectorInfection = rTrans.m_TimeOfVectorInfection;
    }

    uint32_t StrainIdentityMalariaCoTran::GetVectorID() const
    {
        return m_VectorID;
    }

    void StrainIdentityMalariaCoTran::SetVectorID( uint32_t vectorID )
    {
        m_VectorID = vectorID;
    }

    float StrainIdentityMalariaCoTran::GetTimeOfVectorInfection() const
    {
        return m_TimeOfVectorInfection;
    }

    void StrainIdentityMalariaCoTran::SetTimeOfVectorInfection( float time )
    {
        m_TimeOfVectorInfection = time;
    }

    uint32_t StrainIdentityMalariaCoTran::GetAcquiredPersonID() const
    {
        return m_AcquiredPersonID;
    }

    void StrainIdentityMalariaCoTran::SetAcquiredPersonID( uint32_t personID )
    {
        m_AcquiredPersonID = personID;
    }

    void StrainIdentityMalariaCoTran::AddAcquiredInfection( uint32_t infectId )
    {
        m_AcquiredInfections.push_back( AcquiredInfection( infectId ) );
    }

    const std::vector<AcquiredInfection>& StrainIdentityMalariaCoTran::GetAcquiredInfections() const
    {
        return m_AcquiredInfections;
    }

    void StrainIdentityMalariaCoTran::Clear()
    {
        m_GeneticID = 0;
        m_TransmittedPersonID = 0;
        m_TimeOfVectorInfection = 0.0;
        m_VectorID = 0;
        m_AcquiredPersonID = 0;
        m_TransmittedInfections.clear();
        m_AcquiredInfections.clear();
    }

    REGISTER_SERIALIZABLE(StrainIdentityMalariaCoTran);

    void StrainIdentityMalariaCoTran::serialize(IArchive& ar, StrainIdentityMalariaCoTran* obj)
    {
        StrainIdentityMalariaCoTran& strain = *obj;
        ar.labelElement( "m_GeneticID"             ) & strain.m_GeneticID;
        ar.labelElement( "m_TransmittedPersonID"   ) & strain.m_TransmittedPersonID;
        ar.labelElement( "m_TimeOfVectorInfection" ) & strain.m_TimeOfVectorInfection;
        ar.labelElement( "m_VectorID"              ) & strain.m_VectorID;
        ar.labelElement( "m_AcquiredPersonID"      ) & strain.m_AcquiredPersonID;
        ar.labelElement( "m_TransmittedInfections" ) & strain.m_TransmittedInfections;
        ar.labelElement( "m_AcquiredInfections"    ) & strain.m_AcquiredInfections;
    }
}