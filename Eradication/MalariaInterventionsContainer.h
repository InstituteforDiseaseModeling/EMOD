/***************************************************************************************************

Copyright (c) 2015 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode.

***************************************************************************************************/

#pragma once

#include <string>
#include <list>
#include <vector>

#include "BoostLibWrapper.h"

#include "VectorInterventionsContainer.h"
#include "SimpleTypemapRegistration.h"
#include "MalariaContexts.h"
#include "MalariaDrugTypeParameters.h"

namespace Kernel
{
    // this container becomes a help implementation member of the relevant IndividualHuman class 
    // it needs to implement consumer interfaces for all the relevant intervention types
    struct IMalariaDrugEffects : public ISupports
    {
        virtual MalariaDrugTypeParameters::tMDTPMap& GetMdtParams() = 0;
        virtual float get_drug_IRBC_killrate() = 0;
        virtual float get_drug_hepatocyte() = 0;
        virtual float get_drug_gametocyte02() = 0;
        virtual float get_drug_gametocyte34() = 0;
        virtual float get_drug_gametocyteM() = 0;
        virtual ~IMalariaDrugEffects() { }
    };

    struct IMalariaDrugEffectsApply : public ISupports
    {
        virtual void ApplyDrugVaccineReducedAcquireEffect( float rate ) = 0;
        virtual void ApplyDrugVaccineReducedTransmitEffect( float rate ) = 0;
        virtual void ApplyDrugIRBCKillRateEffect( float rate ) = 0;
        virtual void ApplyDrugHepatocyteEffect( float rate ) = 0;
        virtual void ApplyDrugGametocyte02Effect( float rate ) = 0;
        virtual void ApplyDrugGametocyte34Effect( float rate ) = 0;
        virtual void ApplyDrugGametocyteMEffect( float rate ) = 0;
    };

    struct IDrug;
    class MalariaInterventionsContainer :
        public VectorInterventionsContainer,
        public IMalariaDrugEffects, // Getters
        public IMalariaDrugEffectsApply
    {
    public:
        // TODO - WHY IS THIS NECESSARY? Making compiler happy (but not me). Make go away soon.
        virtual int32_t AddRef();
        virtual int32_t Release();

        MalariaInterventionsContainer();
        virtual ~MalariaInterventionsContainer();

        virtual QueryResult QueryInterface(iid_t iid, void** pinstance);

        // IDistributableIntervention
        virtual bool GiveIntervention( IDistributableIntervention * pIV );

        // IMalariaDrugEffectsApply
        virtual void ApplyDrugVaccineReducedAcquireEffect( float rate );
        virtual void ApplyDrugVaccineReducedTransmitEffect( float rate );
        virtual void ApplyDrugIRBCKillRateEffect( float rate );
        virtual void ApplyDrugHepatocyteEffect( float rate );
        virtual void ApplyDrugGametocyte02Effect( float rate );
        virtual void ApplyDrugGametocyte34Effect( float rate );
        virtual void ApplyDrugGametocyteMEffect( float rate );

        //IMalariaDrugEffects(Get): TODO move impl to cpp.
        virtual MalariaDrugTypeParameters::tMDTPMap& GetMdtParams();
        virtual float get_drug_IRBC_killrate();
        virtual float get_drug_hepatocyte();
        virtual float get_drug_gametocyte02();
        virtual float get_drug_gametocyte34();
        virtual float get_drug_gametocyteM();

        virtual void Update(float dt); // hook to update interventions if they need it

    protected:
        float drug_IRBC_killrate;
        float drug_hepatocyte;
        float drug_gametocyte02;
        float drug_gametocyte34;
        float drug_gametocyteM;

        IMalariaHumanContext* individual;

        bool GiveDrug(IDrug* drug); //do some special stuff for drugs.

    private:
#if USE_BOOST_SERIALIZATION || USE_BOOST_MPI
        template<class Archive>
        friend void serialize(Archive &ar, MalariaInterventionsContainer& mic, const unsigned int v);
#endif
    };    
}
