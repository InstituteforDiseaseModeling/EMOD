
#pragma once

#include "ITransmissionGroups.h"
#include "TransmissionGroupMembership.h"
#include "ContagionPopulationSimple.h"

using namespace std;

namespace Kernel
{
    class RANDOMBASE;
    class IPKeyValueContainer;

    typedef map<const string, PropertyValueList_t> PropertyToValuesMap_t;
    typedef map<const string, int> ValueToIndexMap_t;
    typedef map<const string, ValueToIndexMap_t> PropertyValueToIndexMap_t;
    typedef map<const string, ScalingMatrix_t> PropertyToMatrixMap_t;

    template<typename T>
    class ContagionAccumulatorTemplate : public vector<T>
    {
    };

    template<typename T>
    class VectorContagionAccumulatorTemplate : public vector<ContagionAccumulatorTemplate<T>>
    {
    };

    template<typename T>
    class SubstrainMapTemplate : public map<uint32_t,T>
    {
    };
    typedef SubstrainMapTemplate<float> SubstrainMap_t;

    template<typename T>
    class GroupSubstrainMapTemplate : public vector<SubstrainMapTemplate<T>>
    {
    };

    template<typename T>
    class AntigenGroupSubstrainMapTemplate : public vector<GroupSubstrainMapTemplate<T>>
    {
    };


    template<typename T>
    class StrainAwareTransmissionGroupsTemplate : public ITransmissionGroups
    {
    public:
        StrainAwareTransmissionGroupsTemplate( RANDOMBASE* prng );
        virtual ~StrainAwareTransmissionGroupsTemplate();

        // ITransmissionGroups
        virtual void AddProperty(const string& property, const PropertyValueList_t& values, const ScalingMatrix_t& scalingMatrix) override;
        virtual void Build(float contagionDecayRate, int numberOfStrains = 1, int numberOfSubstrains = 1) override;
        virtual void GetGroupMembershipForProperties( const IPKeyValueContainer& properties, TransmissionGroupMembership_t& membershipOut ) const override;
        virtual void UpdatePopulationSize(const TransmissionGroupMembership_t& transmissionGroupMembership, float size_changes, float mc_weight) override;
        virtual void ExposeToContagion(IInfectable* candidate, TransmissionGroupMembership_t transmissionGroupMembership, float deltaTee, TransmissionRoute::Enum tx_route) override;
        virtual void CorrectInfectivityByGroup(float infectivityCorrection, TransmissionGroupMembership_t transmissionGroupMembership) override;
        virtual void EndUpdate(float infectivityMultiplier = 1.0f, float InfectivityAddition = 0.0f ) override;

        virtual void UseTotalPopulationForNormalization() override;
        virtual void UseGroupPopulationForNormalization() override;

        virtual void SetTag( const std::string& tag ) override;
        virtual const std::string& GetTag( void ) const override;

        virtual float GetPopulationSize( const TransmissionGroupMembership_t& transmissionGroupMembership ) const override;
        virtual void ClearPopulationSize() override;
        
        virtual void ClearStrain( const IStrainIdentity* pStrain, const TransmissionGroupMembership_t& membership ) override;
        
        // Methods to be implemented by classes with specific types (i.e. float, GeneticProbability)
        virtual void DepositContagion( const IStrainIdentity& strain,
                                       float amount,
                                       TransmissionGroupMembership_t transmissionGroupMembership ) = 0;
        virtual float GetTotalContagion( void ) = 0;
        virtual float GetContagionByProperty( const IPKeyValue& property_value ) = 0;
        virtual float GetTotalContagionForGroup( TransmissionGroupMembership_t group ) = 0;
// NOTYET        virtual float GetTotalContagionForProperties( const IPKeyValueContainer& property_value ) override;             // Return total contagion on for given properties (maps to membership).

    protected:
        // Function names are lower case to differentiate from externally visible methods.

        // More methods to be implemented by classes with specific types (i.e. float, GeneticProbability)
        virtual void exposeCandidate( IInfectable* candidate,
                                      int iAntigen,
                                      const T& forceOfInfection,
                                      const SubstrainMapTemplate<T>& substrainMap,
                                      float dt,
                                      TransmissionRoute::Enum txRoute ) = 0;
        virtual bool isGreaterThanZero( const T& forceOfInfection ) const = 0;

        T getTotalContagionInner( void );
        void depositContagionInner( const IStrainIdentity& strain,
                                    const T& amount,
                                    TransmissionGroupMembership_t transmissionGroupMembership );

        void checkForDuplicatePropertyName( const string& property ) const;
        void addScalingMatrixToPropertyToMatrixMap( const string& property, const ScalingMatrix_t& scalingMatrix );
        virtual void addPropertyValuesToValueToIndexMap( const string& propertyName, const PropertyValueList_t& valueSet, int currentMatrixSize );
        void getGroupIndicesForProperty( const IPKeyValue& property_value, const PropertyToValuesMap_t& propertyNameToValuesMap, std::vector<size_t>& indices );
        void addPropertyValueListToPropertyToValueMap( const string& property, const PropertyValueList_t& values );
        void buildScalingMatrix( void );
        void allocateAccumulators( NaturalNumber numberOfStrains, NaturalNumber numberOfSubstrains );
        int getGroupCount();

        RANDOMBASE* pRNG;
        PropertyToMatrixMap_t propertyNameToMatrixMap;
        PropertyValueToIndexMap_t propertyValueToIndexMap;      // Used to determine group membership

        PropertyToValuesMap_t propertyToValuesMap;
        ScalingMatrix_t scalingMatrix;
        float contagionDecayRate;
        float populationSize;
        vector<float> populationSizeByGroup;

        int antigenCount;
        int substrainCount;
        bool normalizeByTotalPopulation;

        VectorContagionAccumulatorTemplate<T> newlyDepositedContagionByAntigenAndGroup;       // All antigen (substrains summed) shed this timestep
        VectorContagionAccumulatorTemplate<T> currentContagionByAntigenAndSourceGroup;        // All antigen (substrains summed) current contagion (by contagion source)
        VectorContagionAccumulatorTemplate<T> currentContagionByAntigenAndDestinationGroup;   // All antigen (substrains summed) current contagion (by contagion destination)
        VectorContagionAccumulatorTemplate<T> forceOfInfectionByAntigenAndGroup;              // All antigen (substrains summed) force of infection (current contagion normalized)

        AntigenGroupSubstrainMapTemplate<T> newContagionByAntigenGroupAndSubstrain;
        AntigenGroupSubstrainMapTemplate<T> currentContagionByAntigenSourceGroupAndSubstrain;
        AntigenGroupSubstrainMapTemplate<T> currentContagionByAntigenDestinationGroupAndSubstrain;
        AntigenGroupSubstrainMapTemplate<T> forceOfInfectionByAntigenGroupAndSubstrain;

        std::string tag;
        T totalContagion;
    };

    class StrainAwareTransmissionGroups : public StrainAwareTransmissionGroupsTemplate<float>
    {
    public:
        StrainAwareTransmissionGroups( RANDOMBASE* prng );
        virtual ~StrainAwareTransmissionGroups();
        
        virtual void DepositContagion(const IStrainIdentity& strain, float amount, TransmissionGroupMembership_t transmissionGroupMembership) override;
        virtual float GetTotalContagion( void ) override;
        virtual float GetContagionByProperty( const IPKeyValue& property_value ) override;
        virtual float GetTotalContagionForGroup( TransmissionGroupMembership_t group ) override;

    protected:
        virtual bool isGreaterThanZero( const float& forceOfInfection ) const override;
        virtual void exposeCandidate( IInfectable* candidate,
                                      int iAntigen,
                                      const float& forceOfInfection,
                                      const SubstrainMapTemplate<float>& substrainMap,
                                      float dt,
                                      TransmissionRoute::Enum txRoute ) override;
    };

    class ContagionPopulationSubstrain : public ContagionPopulationSimple
    {
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()
    public:
        ContagionPopulationSubstrain( RANDOMBASE* prng,
                                      int _antigenId,
                                      float _quantity,
                                      const SubstrainMap_t& _substrainDistribution );
        virtual ~ContagionPopulationSubstrain();

        virtual bool ResolveInfectingStrain( IStrainIdentity* strainId ) const override;

    private:
        RANDOMBASE* m_pRNG;
        const SubstrainMap_t& m_rSubstrainDistribution;
    };
}
