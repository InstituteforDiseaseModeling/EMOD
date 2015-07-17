/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#include "stdafx.h"

#include "Drugs.h" // for IDrug interface
#include "SimpleTypemapRegistration.h"
#include "Sugar.h"
#include "MalariaContexts.h"

#include "InterventionFactory.h"
#include "MalariaInterventionsContainer.h"

static const char* _module = "MalariaInterventionsContainer";

namespace Kernel
{
    BEGIN_QUERY_INTERFACE_DERIVED(MalariaInterventionsContainer, VectorInterventionsContainer)
        HANDLE_INTERFACE(IMalariaDrugEffects)
        HANDLE_INTERFACE(IMalariaDrugEffectsApply)
    END_QUERY_INTERFACE_DERIVED(MalariaInterventionsContainer, VectorInterventionsContainer)

    MalariaInterventionsContainer::MalariaInterventionsContainer() :
        VectorInterventionsContainer(),
        drug_IRBC_killrate(0.0),
        drug_hepatocyte(0.0),
        drug_gametocyte02(0.0),
        drug_gametocyte34(0.0),
        drug_gametocyteM(0.0),
        individual(NULL)
    {
    }

    MalariaInterventionsContainer::~MalariaInterventionsContainer()
    {
    }
  
    void MalariaInterventionsContainer::Update(float dt)
    {
        drug_IRBC_killrate = 0;
        drug_hepatocyte = 0;
        drug_gametocyte02 = 0;
        drug_gametocyte34 = 0;
        drug_gametocyteM = 0;

        VectorInterventionsContainer::Update(dt);
    }

    bool MalariaInterventionsContainer::GiveDrug(IDrug* drug)
    {
        IDistributableIntervention * di;
        drug->QueryInterface(GET_IID(IDistributableIntervention), (void**)&di);
        bool keep = false;

        if( individual == NULL )
        {
            if( parent->QueryInterface( GET_IID( IMalariaHumanContext ), (void**) &individual ) != s_OK )
            {
                throw QueryInterfaceException( __FILE__, __LINE__, __FUNCTION__, "individual", "", "IMalariaHumanContext" );
            }
        }

        if(drug->GetDrugUsageType() == DrugUsageType::FullTreatmentParasiteDetect || drug->GetDrugUsageType() == DrugUsageType::SingleDoseParasiteDetect)
        {
            // TODO: use QI here now (EAW)
            if(individual->CheckForParasitesWithTest(1))
            {
                keep = true;
            }
        }
        else if(drug->GetDrugUsageType() == DrugUsageType::FullTreatmentNewDetectionTech || drug->GetDrugUsageType() == DrugUsageType::SingleDoseNewDetectionTech)
        {
            if(individual->CheckForParasitesWithTest(2))
            {
                keep = true;
            }
        }
        else if(drug->GetDrugUsageType() == DrugUsageType::FullTreatmentWhenSymptom || drug->GetDrugUsageType() == DrugUsageType::SingleDoseWhenSymptom)
        {
            if(individual->HasFever())
            {
                keep = true;
            }
        }
        else // not conditional, so just give the drug
        {
            keep = true;
        }

        if( keep )
        {
            drug->ConfigureDrugTreatment( this );
        }
        else
        {
            // TODO: We should get this working (Release)
            //drug->Release();
            //delete drug;
        }
        return keep;
    }

    // For now, before refactoring Drugs to work in new way, just check if the intervention is a
    // Drug, and if so, add to drugs list. In future, there will be no drugs list, just interventions.
    bool MalariaInterventionsContainer::GiveIntervention(
        IDistributableIntervention * pIV
    )
    {
        // NOTE: Calling this AFTER the QI/GiveDrug crashes!!! Both win and linux. Says SetContextTo suddenly became a pure virtual.
        pIV->SetContextTo( parent );
        IDrug * pDrug = NULL;
        if( s_OK == pIV->QueryInterface(GET_IID(IDrug), (void**) &pDrug) )
        {
            LOG_DEBUG("[MalariaInterventionsContainer::GiveIntervention] Got a drug\n");
            if ( GiveDrug( pDrug ) == false )
            {
                return false;
            }
        }
        return VectorInterventionsContainer::GiveIntervention( pIV );
    }

    void MalariaInterventionsContainer::ApplyDrugVaccineReducedAcquireEffect(
        float prob
    )
    {
        drugVaccineReducedAcquire  *= (1.0f-prob);
    }

    void MalariaInterventionsContainer::ApplyDrugVaccineReducedTransmitEffect(
        float prob
    )
    {
        drugVaccineReducedTransmit *= (1.0f-prob);
    }

    void MalariaInterventionsContainer::ApplyDrugIRBCKillRateEffect(
        float rate
    )
    {
        drug_IRBC_killrate += rate;
    }

    void MalariaInterventionsContainer::ApplyDrugHepatocyteEffect(
        float rate
    )
    {
        drug_hepatocyte += rate;
    }

    void MalariaInterventionsContainer::ApplyDrugGametocyte02Effect(
        float rate
    )
    {
        drug_gametocyte02 += rate;
    }

    void MalariaInterventionsContainer::ApplyDrugGametocyte34Effect(
        float rate
    )
    {
        drug_gametocyte34 += rate;
    }

    void MalariaInterventionsContainer::ApplyDrugGametocyteMEffect(
        float rate
    )
    {
        drug_gametocyteM += rate;
    }

    int32_t MalariaInterventionsContainer::AddRef() { return InterventionsContainer::AddRef(); }
    int32_t MalariaInterventionsContainer::Release() { return InterventionsContainer::Release(); }
    float MalariaInterventionsContainer::get_drug_IRBC_killrate() { return drug_IRBC_killrate; }
    float MalariaInterventionsContainer::get_drug_hepatocyte()    { return drug_hepatocyte;    }
    float MalariaInterventionsContainer::get_drug_gametocyte02()  { return drug_gametocyte02;  }
    float MalariaInterventionsContainer::get_drug_gametocyte34()  { return drug_gametocyte34;  }
    float MalariaInterventionsContainer::get_drug_gametocyteM()   { return drug_gametocyteM;   }
    MalariaDrugTypeParameters::tMDTPMap& MalariaInterventionsContainer::GetMdtParams()
    {
        return MalariaDrugTypeParameters::_mdtMap;
    }
}

#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
BOOST_CLASS_EXPORT(Kernel::MalariaInterventionsContainer)

namespace Kernel {
    template<class Archive>
    void serialize(Archive &ar, MalariaInterventionsContainer& mic, const unsigned int v)
    {
        ar & mic.drug_IRBC_killrate;
        ar & mic.drug_hepatocyte;
        ar & mic.drug_gametocyte02;
        ar & mic.drug_gametocyte34;
        ar & mic.drug_gametocyteM;

        // this works only because this class is bound uniquely to a node which owns these pointers
        ar & boost::serialization::base_object<Kernel::VectorInterventionsContainer>(mic);
    }
    template void serialize( boost::mpi::packed_oarchive&, Kernel::MalariaInterventionsContainer&, unsigned int);
    template void serialize( boost::mpi::detail::mpi_datatype_oarchive&, Kernel::MalariaInterventionsContainer&, unsigned int);
    template void serialize( boost::mpi::packed_skeleton_oarchive&, Kernel::MalariaInterventionsContainer&, unsigned int);
    template void serialize( boost::mpi::detail::content_oarchive&, Kernel::MalariaInterventionsContainer&, unsigned int);
    template void serialize( boost::mpi::packed_iarchive&, Kernel::MalariaInterventionsContainer&, unsigned int);
    template void serialize( boost::mpi::packed_skeleton_iarchive&, Kernel::MalariaInterventionsContainer&, unsigned int);
    template void serialize( boost::archive::binary_iarchive&, Kernel::MalariaInterventionsContainer&, unsigned int);
    template void serialize( boost::archive::binary_oarchive&, Kernel::MalariaInterventionsContainer&, unsigned int);
}
#endif
