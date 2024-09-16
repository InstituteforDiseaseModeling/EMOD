
#pragma once

#include "stdafx.h"
#include "IMigrationInfoVector.h"
#include "Migration.h"
#include "EnumSupport.h"
#include "VectorEnums.h"
#include "SimulationEnums.h"

namespace Kernel
{
    struct IVectorSimulationContext;

    ENUM_DEFINE(ModifierEquationType,
        ENUM_VALUE_SPEC(LINEAR       , 1)
        ENUM_VALUE_SPEC(EXPONENTIAL  , 2))

    // ----------------------------------
    // --- MigrationInfoNullVector
    // ----------------------------------
    class MigrationInfoNullVector : public MigrationInfoNull, public IMigrationInfoVector
    {
    public:
        IMPLEMENT_NO_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        //IMigrationInfoVector
        virtual void UpdateRates( const suids::suid& rThisNodeId,
                                  const std::string& rSpeciesID,
                                  IVectorSimulationContext* pivsc ) {};

        virtual Gender::Enum ConvertVectorGender(VectorGender::Enum gender) const 
        {
            return (gender == VectorGender::VECTOR_FEMALE ? Gender::FEMALE : Gender::MALE);
        };


    protected:
        friend class MigrationInfoFactoryVector;
        friend class MigrationInfoFactoryVectorDefault;

        MigrationInfoNullVector();
        virtual ~MigrationInfoNullVector();
    };

    // ----------------------------------
    // --- MigrationInfoAgeAndGenderVector
    // ----------------------------------

    class MigrationInfoAgeAndGenderVector : public MigrationInfoAgeAndGender, public IMigrationInfoVector
    {
    public:
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
            DECLARE_QUERY_INTERFACE()
    public:
        virtual ~MigrationInfoAgeAndGenderVector();

        // IMigrationInfoVector
        virtual void UpdateRates( const suids::suid& rThisNodeId,
                                  const std::string& rSpeciesID,
                                  IVectorSimulationContext* pivsc ) override;

        virtual Gender::Enum ConvertVectorGender (VectorGender::Enum gender ) const override;
        virtual void CalculateRates( Gender::Enum gender, float ageYears) override;
        virtual float GetTotalRate( Gender::Enum gender = Gender::MALE ) const override;
        virtual const std::vector<float>& GetCumulativeDistributionFunction( Gender::Enum gender = Gender::MALE ) const override;
        const std::vector<suids::suid>& GetReachableNodes( Gender::Enum gender = Gender::MALE ) const override;


    protected:
        friend class MigrationInfoFactoryVector;
        friend class MigrationInfoFactoryVectorDefault;

        MigrationInfoAgeAndGenderVector( INodeContext* _parent,
                                         ModifierEquationType::Enum equation,
                                         float habitatModifier,
                                         float foodModifier,
                                         float stayPutModifier );

        virtual void Initialize( const std::vector<std::vector<MigrationRateData>>& rRateData ) override;
        virtual void SaveRawRates( std::vector<float>& r_rate_cdf, Gender::Enum gender )  override;

        float CalculateModifiedRate( const suids::suid& rNodeId,
                                     float rawRate,
                                     float populationRatio,
                                     float habitatRatio);

        typedef std::function<int(const suids::suid& rNodeId,
                                  const std::string& rSpeciesID,
                                  IVectorSimulationContext* pivsc)> tGetValueFunc;

        std::vector<float> GetRatios(const std::vector<suids::suid>& rReachableNodes,
                                     const std::string& rSpeciesID,
                                     IVectorSimulationContext* pivsc,
                                     tGetValueFunc getValueFunc);


    private:
        std::vector<float>          m_RawMigrationRate;
        std::vector<suids::suid>    m_ReacheableNodesFemaleUpdated;
        float                       m_TotalRateFemale;
        std::vector<float>          m_RateCDFFemale;
        suids::suid                 m_ThisNodeId;
        ModifierEquationType::Enum  m_ModifierEquation;
        float                       m_ModifierHabitat;
        float                       m_ModifierFood;
        float                       m_ModifierStayPut;
    };



    // ----------------------------------
    // --- MigrationInfoFactoryVector
    // ----------------------------------

    class MigrationInfoFactoryVector : public IMigrationInfoFactoryVector
    {
    public:
        MigrationInfoFactoryVector( bool enableVectorMigration );
        virtual ~MigrationInfoFactoryVector();

        // IMigrationInfoFactoryVector
        virtual void ReadConfiguration( JsonConfigurable* pParent, const ::Configuration* config ) override;
        virtual IMigrationInfoVector* CreateMigrationInfoVector( const std::string& idreference,
                                                                 INodeContext *parent_node, 
                                                                 const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap ) override;

    private:
        MigrationInfoFile          m_InfoFileVector;
        ModifierEquationType::Enum m_ModifierEquation;
        float                      m_ModifierHabitat;
        float                      m_ModifierFood;
        float                      m_ModifierStayPut;
    };

    // ----------------------------------
    // --- MigrationInfoFactoryVectorDefault
    // ----------------------------------

    class MigrationInfoFactoryVectorDefault : public IMigrationInfoFactoryVector
    {
    public:
        MigrationInfoFactoryVectorDefault( bool enableVectorMigration, int defaultTorusSize );
        virtual ~MigrationInfoFactoryVectorDefault();

        // IMigrationInfoFactoryVector
        virtual void ReadConfiguration( JsonConfigurable* pParent, const ::Configuration* config ) {};
        virtual IMigrationInfoVector* CreateMigrationInfoVector( const std::string& idreference,
                                                                 INodeContext *parent_node, 
                                                                 const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap ) override;

    protected: 

    private: 
        bool m_IsVectorMigrationEnabled;
        int  m_TorusSize;
    };
}
