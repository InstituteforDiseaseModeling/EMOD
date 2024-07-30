
#include "StandardEventCoordinator.h"
#include "InterpolatedValueMap.h"
#include "Types.h"

namespace Kernel
{
    class ReferenceTrackingEventCoordinator : public StandardInterventionDistributionEventCoordinator 
    {
        DECLARE_FACTORY_REGISTERED_EXPORT(EventCoordinatorFactory, ReferenceTrackingEventCoordinator, IEventCoordinator)    

    public:
        DECLARE_CONFIGURED(ReferenceTrackingEventCoordinator)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

        ReferenceTrackingEventCoordinator();
        virtual ~ReferenceTrackingEventCoordinator() { } 

        virtual void SetContextTo(ISimulationEventContext *isec) override;
        virtual void Update(float dt) override;
        virtual void CheckStartDay( float campaignStartDay ) const override;

    protected:
        virtual void InitializeRepetitions( const Configuration* inputJson ) override;
        virtual void preDistribute() override;
        virtual void DistributeInterventionsToIndividuals( INodeEventContext* event_context ) override;

        InterpolatedValueMap m_Year2ValueMap;
        float m_EndYear;
        float m_NumQualifiedWithout;
        float m_NumQualifiedNeeding;
        std::map<INodeEventContext*,std::vector<IIndividualHumanEventContext*>> m_QualifiedPeopleWithoutMap;
    };
}
