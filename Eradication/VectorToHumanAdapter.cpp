
#include "stdafx.h"

#include "VectorToHumanAdapter.h"
#include "INodeContext.h"

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_BODY( VectorToHumanAdapter )
        HANDLE_INTERFACE( IIndividualHumanEventContext )
    END_QUERY_INTERFACE_BODY( VectorToHumanAdapter )

    VectorToHumanAdapter::VectorToHumanAdapter( INodeContext* pNodeContext, uint32_t vectorID )
        : m_pNodeContext( pNodeContext )
        , m_VectorID( vectorID )
    {
    }

    VectorToHumanAdapter::~VectorToHumanAdapter()
    {
    }

    //IIndividualHumanEventContext
    suids::suid VectorToHumanAdapter::GetSuid() const
    {
        suids::suid id;
        id.data = m_VectorID;
        return id;
    }

    INodeEventContext* VectorToHumanAdapter::GetNodeEventContext()
    {
        return m_pNodeContext->GetEventContext();
    }

    const IIndividualHuman* VectorToHumanAdapter::GetIndividualHumanConst() const
    {
        // user can use this to know that the object is really a vector
        return nullptr;
    }

    bool VectorToHumanAdapter::IsPregnant() const
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Should not be accessing this method in VectorToHumanAdapter" );
    }

    double VectorToHumanAdapter::GetAge() const
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Should not be accessing this method in VectorToHumanAdapter" );
    }

    int VectorToHumanAdapter::GetGender() const
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Should not be accessing this method in VectorToHumanAdapter" );
    }

    double VectorToHumanAdapter::GetMonteCarloWeight() const
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Should not be accessing this method in VectorToHumanAdapter" );
    }

    bool VectorToHumanAdapter::IsPossibleMother() const
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Should not be accessing this method in VectorToHumanAdapter" );
    }

    bool VectorToHumanAdapter::IsInfected() const
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Should not be accessing this method in VectorToHumanAdapter" );
    }

    bool VectorToHumanAdapter::IsSymptomatic() const
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Should not be accessing this method in VectorToHumanAdapter" );
    }

    float VectorToHumanAdapter::GetInfectiousness() const
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Should not be accessing this method in VectorToHumanAdapter" );
    }

    void VectorToHumanAdapter::Die( HumanStateChange )
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Should not be accessing this method in VectorToHumanAdapter" );
    }

    HumanStateChange VectorToHumanAdapter::GetStateChange( void ) const
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Should not be accessing this method in VectorToHumanAdapter" );
    }

    IIndividualHumanInterventionsContext* VectorToHumanAdapter::GetInterventionsContext() const
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Should not be accessing this method in VectorToHumanAdapter" );
    }

    IPKeyValueContainer* VectorToHumanAdapter::GetProperties()
    {
        throw IllegalOperationException( __FILE__, __LINE__, __FUNCTION__, "Should not be accessing this method in VectorToHumanAdapter" );
    }
}
