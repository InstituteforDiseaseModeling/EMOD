
#pragma once

#include "DrugModel.h"
#include "MalariaInterventionsContainerContexts.h"  // for IMalariaDrugEffects methods

namespace Kernel
{
    class MalariaDrugTypeParameters;


    class DrugModelAntiMalarial : public DrugModel, public IMalariaDrugEffects
    {
    public:
        DrugModelAntiMalarial( const std::string& rName = "UNITIALIZED", const MalariaDrugTypeParameters* pParams = nullptr );
        virtual ~DrugModelAntiMalarial();
        virtual DrugModel* Clone();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void **ppvObject ) override;
        virtual int32_t AddRef() override { return DrugModel::AddRef(); }
        virtual int32_t Release() override { return DrugModel::Release(); }

        // DrugModel
        virtual void  ConfigureDrugTreatment( IIndividualHumanInterventionsContext * ivc ) override;

        // IMalariaDrugEffects
        float get_drug_IRBC_killrate( const IStrainIdentity& rStrain ) override;
        float get_drug_hepatocyte( const IStrainIdentity& rStrain ) override;
        float get_drug_gametocyte02( const IStrainIdentity& rStrain ) override;
        float get_drug_gametocyte34( const IStrainIdentity& rStrain ) override;
        float get_drug_gametocyteM( const IStrainIdentity& rStrain ) override;

        const MalariaDrugTypeParameters* GetDrugParams();

    protected:
        static float BodyWeightByAge( float age_in_days );
        static const float _adult_bodyweight_kg;
        typedef std::map<float, float> bodyweight_map_t;
        static bodyweight_map_t create_bodyweight_map();
        static const bodyweight_map_t bodyweight_map_;

        virtual float CalculateModifiedEfficacy( const IStrainIdentity& rStrain );

        const MalariaDrugTypeParameters* pMalariaDrugTypeParameters;

        DECLARE_SERIALIZABLE( DrugModelAntiMalarial );
    };
}
