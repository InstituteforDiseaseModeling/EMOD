
#pragma once

#include "IndividualMalaria.h"

namespace Kernel
{
    struct INodeMalariaGenetics;
    struct IParasiteCohort;

    class IndividualHumanMalariaGenetics : public IndividualHumanMalaria
    {
        //friend class SimulationMalaria;
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING();
        DECLARE_QUERY_INTERFACE();

    public:
        static void InitializeStaticsMalariaGenetics( const Configuration* config );

        static IndividualHumanMalariaGenetics *CreateHuman( INodeContext *context,
                                                            suids::suid _suid,
                                                            double monte_carlo_weight = 1.0f,
                                                            double initial_age = 0.0f,
                                                            int gender = 0 );
        virtual ~IndividualHumanMalariaGenetics();

        virtual void SetContextTo( INodeContext* context ) override;
        virtual void UpdateInfectiousness( float dt ) override;
        virtual void ExposeToInfectivity( float dt, TransmissionGroupMembership_t tgm ) override;

    protected:
        IndividualHumanMalariaGenetics( suids::suid id = suids::nil_suid(),
                                        double monte_carlo_weight = 1.0,
                                        double initial_age = 0.0,
                                        int gender = 0 );
        IndividualHumanMalariaGenetics( INodeContext *context );

        virtual IInfection* createInfection( suids::suid _suid ) override;
        virtual void StoreGametocyteCounts( const IStrainIdentity& rStrain,
                                            int64_t femaleMatureGametocytes,
                                            int64_t maleMatureGametocytes ) override;

        void ClearMatureGametocyteCohorts();

        INodeMalariaGenetics *m_pNodeGenetics;
        std::vector<IParasiteCohort*> m_MatureGametocytesFemale;
        std::vector<IParasiteCohort*> m_MatureGametocytesMale;

        DECLARE_SERIALIZABLE( IndividualHumanMalariaGenetics );
    };
}
