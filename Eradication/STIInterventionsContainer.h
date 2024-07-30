
#pragma once

#include <string>
#include <list>
#include <vector>

#include "ISTIInterventionsContainer.h"
#include "Interventions.h"
#include "InterventionsContainer.h"
#include "IRelationship.h"

namespace Kernel
{
    // this container becomes a help implementation member of the IndividualHumanSTI class 
    // it needs to implement consumer interfaces for all the relevant intervention types


    class STIInterventionsContainer
        : public InterventionsContainer
        , public ISTIInterventionsContainer
        , public ISTIBarrierConsumer
        , public ISTICircumcisionConsumer 
        , public ISTICoInfectionStatusChangeApply
        , public ICoitalActRiskData
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()

    public:
        STIInterventionsContainer();
        virtual ~STIInterventionsContainer();

        // ISupports
        virtual QueryResult QueryInterface(iid_t iid, void** pinstance) override;

        // InterventionsContainer
        virtual void Update( float dt ) override;

        // ISTIInterventionsContainer
        virtual void AddNonPfaRelationshipStarter( INonPfaRelationshipStarter* pSNR ) override;
        virtual void StartNonPfaRelationships() override;


        // ISTIBarrierConsumer 
        virtual void UpdateSTIBarrierProbabilitiesByType( RelationshipType::Enum rel_type, const Sigmoid& config_overrides ) override;
        virtual const Sigmoid& GetSTIBarrierProbabilitiesByRelType( const IRelationshipParameters* pRelParams ) const override;

        // ISTICircumcisionConsumer 
        virtual bool IsCircumcised( void ) const override;
        virtual float GetCircumcisedReducedAcquire() const override;
        virtual void ApplyCircumcision( float reduceAcquire ) override;

        // IIndividualHumanInterventionsContext
        virtual void ChangeProperty( const char *property, const char* new_value) override;

        virtual float GetInterventionReducedAcquire() const override;
        virtual float GetInterventionReducedTransmit() const override;

        // ISTICoInfectionStatusChangeApply 
        virtual void SpreadStiCoInfection() override;
        virtual void CureStiCoInfection() override;

        // ICoitalActRiskFactor
        virtual void UpdateCoitalActRiskFactors( float acqMod, float tranMod ) override;
        virtual float GetCoitalActRiskAcquisitionFactor() const override;
        virtual float GetCoitalActRiskTransmissionFactor() const override;

    protected:
        bool is_circumcised;
        float circumcision_reduced_require;
        float risk_modifier_acquisition;
        float risk_modifier_transmission;

        std::vector<bool>    has_blocking_overrides;
        std::vector<Sigmoid> STI_blocking_overrides;

        std::vector<INonPfaRelationshipStarter*> snr_inv_list;

        DECLARE_SERIALIZABLE(STIInterventionsContainer);
    };
}
