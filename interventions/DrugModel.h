
#pragma once

#include "IDrug.h"
#include "ISerializable.h"
#include "MalariaDrugTypeParameters.h"

namespace Kernel
{
    struct IIndividualHumanInterventionsContext;
    struct IDistribution;
    class RANDOMBASE;

    class DrugModel : public IDrug, public ISerializable
    {
    public:
        // Making GenericDrug a friend to reduce code changes and for GenericDrug::Configure()
        friend class GenericDrug;

        DrugModel( const std::string& rDefaultName = "UNINITIALIZED" );
        DrugModel( const DrugModel& rMaster );
        virtual ~DrugModel();
        virtual DrugModel* Clone();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void **ppvObject ) override;
        virtual int AddRef() override { return 0; };
        virtual int Release() override { return 0; };

        // IDrug
        virtual const std::string& GetDrugName() const override;
        virtual float GetDrugCurrentEfficacy() const override;
        virtual float GetDrugCurrentConcentration() const override;
        virtual int GetNumRemainingDoses() const override;

        // Other
        virtual void PkPdParameterValidation();
        virtual void  ConfigureDrugTreatment( IIndividualHumanInterventionsContext * ivc );
        virtual float GetDrugReducedAcquire()  const;
        virtual float GetDrugReducedTransmit() const;
        virtual void SetDrugName( const std::string& rDrugName );
        virtual void SetFastDecayTimeConstant( float fastDecay );
        virtual void SetDrugReducedAcquire( float reducedAcquire );
        virtual void SetDrugReducedTransmit( float reducedTransmit );
        virtual void SetDrugCurrentEfficacy( float efficacy );
        virtual void TakeDose( float dt, RANDOMBASE* pRNG );
        virtual void DecayAndUpdateEfficacy( float dt );

    protected:
        virtual void TakeDoseSimple( float dt, RANDOMBASE* pRNG );
        virtual void TakeDoseWithPkPd( float dt, RANDOMBASE* pRNG );
        virtual void DecayAndUpdateEfficacySimple( float dt );
        virtual void DecayAndUpdateEfficacyWithPkPd( float dt );

        float CalculateEfficacy( float c50, float startConcentration, float endConcentration );


        std::string drug_name;
        PKPDModel::Enum durability_time_profile;
        float fast_decay_time_constant;
        float slow_decay_time_constant;
        float fast_component;
        float slow_component;

        float start_concentration;
        float end_concentration;
        float current_concentration;
        float current_efficacy;
        float current_reducedacquire;
        float current_reducedtransmit;
        float pk_rate_mod;
        float Cmax;
        float Vd;
        float drug_c50;
        float fraction_defaulters;
        IDistribution* p_uniform_distribution;

        DECLARE_SERIALIZABLE( DrugModel );
    };
}
