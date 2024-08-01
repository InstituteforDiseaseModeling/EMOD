
#pragma once

#include "IndividualEventContext.h"
#include "VectorEnums.h"
#include "IIndividualHumanContext.h"

namespace Kernel
{
    struct INodeContext;

    // Used so we can get event data on vectors
    class VectorToHumanAdapter : public IIndividualHumanEventContext
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
    public:
        VectorToHumanAdapter( INodeContext* pNodeContext, uint32_t vectorID);
        VectorToHumanAdapter( INodeContext* pNodeContext, uint32_t vectorID, VectorGender::Enum vector_gender);
        virtual ~VectorToHumanAdapter();

        virtual suids::suid GetSuid() const;
        virtual INodeEventContext *GetNodeEventContext();
        virtual double GetAge()              const;
        virtual int    GetGender()           const;


        // The following throw an exception if used
        virtual const IIndividualHuman* GetIndividualHumanConst() const override;
        virtual bool   IsPregnant()          const;
        virtual double GetMonteCarloWeight() const;
        virtual bool   IsPossibleMother()    const;
        virtual bool   IsInfected()          const;
        virtual bool   IsSymptomatic()       const;
        virtual float  GetInfectiousness()   const;
        virtual void   Die( HumanStateChange );
        virtual HumanStateChange GetStateChange( void ) const;
        virtual IIndividualHumanInterventionsContext *GetInterventionsContext() const;
        virtual IPKeyValueContainer * GetProperties();

    private:
        INodeContext* m_pNodeContext;
        uint32_t m_VectorID;
        VectorGender::Enum m_VectorGender;
    };

}
