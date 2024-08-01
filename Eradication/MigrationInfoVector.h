
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

    ENUM_DEFINE(ModiferEquationType,
        ENUM_VALUE_SPEC(LINEAR       , 1)
        ENUM_VALUE_SPEC(EXPONENTIAL  , 2))

    // ----------------------------------
    // --- MigrationInfoNullVector
    // ----------------------------------
    class IDMAPI MigrationInfoNullVector : public MigrationInfoNull, public IMigrationInfoVector
    {
    public:
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
        DECLARE_QUERY_INTERFACE()

    public:
        //IMigrationInfoVector
        virtual void UpdateRates( const suids::suid& rThisNodeId,
                                  const std::string& rSpeciesID,
                                  IVectorSimulationContext* pivsc ) {};

        virtual Gender::Enum ConvertVectorGender(VectorGender::Enum gender) const { return Gender::MALE; };
        virtual const std::vector<suids::suid>& GetReachableNodesByGender(VectorGender::Enum gender) const 
        {
            static vector<suids::suid> empty_vector;
            return empty_vector;
        };
        virtual const std::vector<MigrationType::Enum>& GetMigrationTypesByGender(VectorGender::Enum gender) const 
        { 
            static vector<MigrationType::Enum> empty_vector;
            return empty_vector;
        };
        virtual void CalculateRates(VectorGender::Enum vector_gender) {};

    protected:
        friend class MigrationInfoFactoryVector;
        friend class MigrationInfoFactoryVectorDefault;

        MigrationInfoNullVector();
        virtual ~MigrationInfoNullVector();
    };

    // ----------------------------------
    // --- MigrationInfoFixedRateVector
    // ----------------------------------

    class IDMAPI MigrationInfoFixedRateVector : public MigrationInfoFixedRate, public IMigrationInfoVector
    {
    public:
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()
    public:
        virtual ~MigrationInfoFixedRateVector();

        virtual void PickMigrationStep( RANDOMBASE* pRNG,
                                        IIndividualHumanEventContext * traveler, 
                                        float migration_rate_modifier, 
                                        suids::suid &destination, 
                                        MigrationType::Enum &migration_type,
                                        float &timeUntilTrip ) override;

        // IMigrationInfoVector
        virtual void UpdateRates( const suids::suid& rThisNodeId, 
                                  const std::string& rSpeciesID, 
                                  IVectorSimulationContext* pivsc ) override;

    protected:
        friend class MigrationInfoFactoryVector;
        friend class MigrationInfoFactoryVectorDefault;

        MigrationInfoFixedRateVector( INodeContext* _parent,
                             ModiferEquationType::Enum equation,
                             float habitatModifier,
                             float foodModifier,
                             float stayPutModifier );

        virtual void Initialize( const std::vector<std::vector<MigrationRateData>>& rRateData ) override;

        virtual Gender::Enum ConvertVectorGender(VectorGender::Enum gender) const override;
        virtual const std::vector<suids::suid>& GetReachableNodesByGender(VectorGender::Enum gender) const override;
        virtual const std::vector<MigrationType::Enum>& GetMigrationTypesByGender(VectorGender::Enum gender) const override;
        virtual void CalculateRates(VectorGender::Enum vector_gender) override;

        virtual void SaveRawRates( std::vector<float>& r_rate_cdf, Gender::Enum gender ) override;
        float CalculateModifiedRate( const suids::suid& rNodeId, 
                                     float rawRate, 
                                     float populationRatio, 
                                     float habitatRatio );

        typedef std::function<int( const suids::suid& rNodeId, 
                                   const std::string& rSpeciesID, 
                                   IVectorSimulationContext* pivsc )> tGetValueFunc;

        std::vector<float> GetRatios( const std::vector<suids::suid>& rReachableNodes, 
                                      const std::string& rSpeciesID, 
                                      IVectorSimulationContext* pivsc, 
                                      tGetValueFunc getValueFunc );

    private:
        std::vector<float> m_RawMigrationRate;
        suids::suid m_ThisNodeId;
        ModiferEquationType::Enum m_ModifierEquation;
        float m_ModifierHabitat;
        float m_ModifierFood;
        float m_ModifierStayPut;
    };

    // ----------------------------------
    // --- MigrationInfoAgeAndGenderVector
    // ----------------------------------

    class IDMAPI MigrationInfoAgeAndGenderVector : public MigrationInfoAgeAndGender, public IMigrationInfoVector
    {
    public:
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()
            DECLARE_QUERY_INTERFACE()
    public:
        virtual ~MigrationInfoAgeAndGenderVector();

        // IMigrationInfoVector
        virtual void UpdateRates(const suids::suid& rThisNodeId,
            const std::string& rSpeciesID,
            IVectorSimulationContext* pivsc) override;

        virtual Gender::Enum ConvertVectorGender(VectorGender::Enum gender) const override;
        virtual const std::vector<suids::suid>& GetReachableNodesByGender(VectorGender::Enum gender) const override;
        virtual const std::vector<MigrationType::Enum>& GetMigrationTypesByGender(VectorGender::Enum gender) const override;
        virtual void CalculateRates(VectorGender::Enum vector_gender) override;

    protected:
        friend class MigrationInfoFactoryVector;
        friend class MigrationInfoFactoryVectorDefault;

        MigrationInfoAgeAndGenderVector(INodeContext* _parent,
            ModiferEquationType::Enum equation,
            float habitatModifier,
            float foodModifier,
            float stayPutModifier);

        virtual void Initialize(const std::vector<std::vector<MigrationRateData>>& rRateData) override;
        virtual void SaveRawRates(std::vector<float>& r_rate_cdf, Gender::Enum gender) override;
        float CalculateModifiedRate(const suids::suid& rNodeId,
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
        std::vector<std::vector<float>> m_RawMigrationRatesVectorGender;
        std::vector<float> m_TotalRatesVectorGender;
        std::vector<std::vector<float>> m_RateCDFVectorGender;
        suids::suid m_ThisNodeId;
        ModiferEquationType::Enum m_ModifierEquation;
        float m_ModifierHabitat;
        float m_ModifierFood;
        float m_ModifierStayPut;
    };



    // ----------------------------------
    // --- MigrationInfoFactoryVector
    // ----------------------------------

    class IDMAPI MigrationInfoFactoryVector : public MigrationInfoFactoryFile, public IMigrationInfoFactoryVector
    {
        GET_SCHEMA_STATIC_WRAPPER(MigrationInfoFactoryVector)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()

    public:
        MigrationInfoFactoryVector();
        virtual ~MigrationInfoFactoryVector();

        // MigrationInfoFactoryFile
        virtual void Initialize( const ::Configuration *config, const std::string& idreference ) override;

        // IMigrationInfoFactoryVector
        virtual IMigrationInfoVector* CreateMigrationInfoVector( 
            INodeContext *parent_node, 
            const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap ) override;
        virtual bool IsVectorMigrationEnabled() const override;

    protected:
        // MigrationInfoFactoryFile
        virtual void CreateInfoFileList() override;
        virtual void InitializeInfoFileList( const Configuration* config ) override;

    private:
        std::vector<MigrationInfoFile*> m_InfoFileListVector;
        bool m_IsVectorMigrationEnabled;
        ModiferEquationType::Enum m_ModifierEquation;
        float m_ModifierHabitat;
        float m_ModifierFood;
        float m_ModifierStayPut;
    };

    // ----------------------------------
    // --- MigrationInfoFactoryVectorDefault
    // ----------------------------------

    class IDMAPI MigrationInfoFactoryVectorDefault : public MigrationInfoFactoryDefault, public IMigrationInfoFactoryVector
    {
        GET_SCHEMA_STATIC_WRAPPER(MigrationInfoFactoryVectorDefault)
        IMPLEMENT_DEFAULT_REFERENCE_COUNTING()  
        DECLARE_QUERY_INTERFACE()

    public:
        MigrationInfoFactoryVectorDefault( int defaultTorusSize );
        MigrationInfoFactoryVectorDefault();
        virtual ~MigrationInfoFactoryVectorDefault();

        // JsonConfigurable methods
        virtual bool Configure( const Configuration* config ) override;

        // IMigrationInfoFactoryVector
        virtual IMigrationInfoVector* CreateMigrationInfoVector( 
            INodeContext *parent_node, 
            const boost::bimap<ExternalNodeId_t, suids::suid>& rNodeIdSuidMap ) override;
        virtual bool IsVectorMigrationEnabled() const override;

    protected:

    private:
        bool m_IsVectorMigrationEnabled;
        float m_xLocalModifierVector;
    };
}
