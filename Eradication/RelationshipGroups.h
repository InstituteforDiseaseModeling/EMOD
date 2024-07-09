
#pragma once

#include "ITransmissionGroups.h"
#include "CoitalAct.h"
#include "StrainIdentity.h"

namespace Kernel
{
    struct IContagionInfo: public ISupports
    {
    public:
        virtual const CoitalAct& GetCoitalAct() const = 0;
        virtual float GetInterventionReducedAcquire() const = 0;
    };

    struct CoitalActAndStrain
    {
        CoitalAct coital_act;
        StrainIdentity strain;

        CoitalActAndStrain( const CoitalAct& rCoitalAct, const IStrainIdentity& rStrain )
            : coital_act( rCoitalAct )
            , strain()
        {
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            // !!! Use the setters to initialize "strain" to avoid the checks in the constructor.
            // !!! We put the ID of the human that is the source of the infection in the antigenID.
            // !!! This number will be greater than InfectionConfig::number_basestrains so we need
            // !!! to work around the check in the constructor.  Should probably rethink where that
            // !!! check takes place.
            // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            strain.SetAntigenID( rStrain.GetAntigenID() );
            strain.SetGeneticID( rStrain.GetGeneticID() );
        }
    };

    class DiscreteContagionPopulation : public IContagionPopulation, public IContagionInfo
    {
        DECLARE_QUERY_INTERFACE()
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
    public:
        DiscreteContagionPopulation( const CoitalActAndStrain& rCoitalActAndStrain,
                                     float interventionReducedAcquire );
        virtual ~DiscreteContagionPopulation();

        // IContagionPopulation
        virtual int GetAntigenID( void ) const override;
        virtual float GetTotalContagion( void ) const override;
        virtual bool ResolveInfectingStrain( IStrainIdentity* strainId ) const override;

        // IContagionProbabilities
        virtual const CoitalAct& GetCoitalAct() const override;
        virtual float GetInterventionReducedAcquire() const override;

    protected:
        const CoitalActAndStrain& m_CoitalActAndStrain;
        float m_InterventionReducedAcquire;
    };


    struct INodeSTI;

    class RelationshipGroups: public ITransmissionGroups
    {
        public:
            RelationshipGroups();

            // ITransmissionGroups - invalid methods
            virtual void AddProperty( const std::string& property, const PropertyValueList_t& values, const ScalingMatrix_t& scalingMatrix );
            virtual void GetGroupMembershipForProperties( const IPKeyValueContainer& properties, TransmissionGroupMembership_t& membershipOut ) const override;

            // ITransmissionGroups - implemented methods
            virtual void Build(float contagionDecayRate, int numberOfStrains, int numberOfSubstrains = 1) override;
            virtual void UpdatePopulationSize(const TransmissionGroupMembership_t& transmissionGroupMembership, float size_changes, float mc_weight) override;
            virtual void DepositContagion(const IStrainIdentity& strain, float amount, TransmissionGroupMembership_t poolMembership) override;
            virtual void ExposeToContagion(IInfectable* candidate, TransmissionGroupMembership_t poolMembership, float deltaTee, TransmissionRoute::Enum tx_route = TransmissionRoute::TRANSMISSIONROUTE_CONTACT) override;
            virtual void CorrectInfectivityByGroup( float infectivityCorrection, TransmissionGroupMembership_t transmissionGroupMembership ) override;
            virtual void EndUpdate(float infectivityMultiplier = 1.0f, float infectivityAddition = 0.0f ) override;
            virtual float GetContagionByProperty( const IPKeyValue& property_value ) override;

            virtual void UseTotalPopulationForNormalization( void ) override;
            virtual void UseGroupPopulationForNormalization( void ) override;

            virtual float GetPopulationSize( const TransmissionGroupMembership_t& transmissionGroupMembership ) const override;
            virtual void ClearPopulationSize() override;

            virtual void SetTag( const std::string& tag ) override;
            virtual const std::string& GetTag( void ) const override;

            virtual void ClearStrain( const IStrainIdentity* pStrain, const TransmissionGroupMembership_t& membership ) override;

            virtual float GetTotalContagion( void ) override;
            virtual float GetTotalContagionForGroup( TransmissionGroupMembership_t group ) override;    // Return total contagion for given membership.

            // Special RelationGroups methods so we can use RelationshipID instead of strings with the ID in them
            virtual void SetParent( INodeSTI* parent );
            virtual void AddProperty( const std::string& property, const std::list<uint32_t>& values, const ScalingMatrix_t& scalingMatrix );
            virtual void addPropertyValuesToValueToIndexMap( const std::string& propertyName, const std::list<uint32_t>& valueSet, int currentMatrixSize );

            // Other
            virtual void DepositContagion( const IStrainIdentity& strain, const CoitalAct& rCoitalAct );

    protected:
            void CreateRandomIndexes( std::vector<int>& rRandomIndexes ) const;

            std::map<uint32_t,std::map<uint32_t, std::vector<CoitalActAndStrain>>> m_ExposedPersonsCoitalActsMap; // human_id to rel_id to acts for this relationship
            INodeSTI * m_parent;
    };
}
