
#pragma once

#include "Configure.h"
#include "JsonConfigurableCollection.h"
#include "GeneticProbability.h"

namespace Kernel
{
    class VectorSpeciesCollection;

    ENUM_DEFINE(ResistanceType, 
        ENUM_VALUE_SPEC(LARVAL_KILLING , 0)
        ENUM_VALUE_SPEC(REPELLING      , 1)
        ENUM_VALUE_SPEC(BLOCKING       , 2)
        ENUM_VALUE_SPEC(KILLING        , 3))

    // Reads in a single AlleleCombo and Modifier
    class AlleleComboProbabilityConfig : public JsonConfigurable
    {
        IMPLEMENT_NO_REFERENCE_COUNTING();
    public:
        AlleleComboProbabilityConfig( const VectorSpeciesCollection* pSpeciesCollection );
        AlleleComboProbabilityConfig( const AlleleComboProbabilityConfig& rMaster );
        virtual ~AlleleComboProbabilityConfig();

        virtual bool Configure( const Configuration *json ) override;

        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }

        const AlleleComboProbability& GetProbability( ResistanceType::Enum rt ) const;

    private:
        const VectorSpeciesCollection* m_pSpeciesCollection;
        std::vector<AlleleComboProbability> m_Probabilities;
    };


    // Read a collection of AlleleComboProbabilityConfig's so that you can get
    // a single GeneticProbability/
    class AlleleComboProbabilityConfigCollection : public JsonConfigurableCollection<AlleleComboProbabilityConfig>
    {
    public:
        AlleleComboProbabilityConfigCollection( const VectorSpeciesCollection* pSpeciesCollection );
        AlleleComboProbabilityConfigCollection( const AlleleComboProbabilityConfigCollection& rMaster );
        virtual ~AlleleComboProbabilityConfigCollection();

        virtual void CheckConfiguration() override;

        const GeneticProbability& GetProbability( ResistanceType::Enum rt ) const;

    protected:
        virtual AlleleComboProbabilityConfig* CreateObject() override;

        const VectorSpeciesCollection* m_pSpeciesCollection;
        std::vector<GeneticProbability> m_Probabilities;
    };


    // Modification parameters for a specific insecticde.
    class Insecticide : public JsonConfigurable
    {
        IMPLEMENT_NO_REFERENCE_COUNTING();
    public:
        Insecticide( const VectorSpeciesCollection* pSpeciesCollection );
        Insecticide( const Insecticide& rMaster );
        virtual ~Insecticide();

        virtual bool Configure( const Configuration *json ) override;

        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }

        const std::string& GetName() const;
        const GeneticProbability& GetResistance( ResistanceType::Enum ) const;

    private:
        const VectorSpeciesCollection* m_pSpeciesCollection;
        std::string m_Name;
        std::vector<GeneticProbability> m_Resistances;
    };


    class InsecticideCollection : public JsonConfigurableCollection<Insecticide>
    {
    public:
        InsecticideCollection( const VectorSpeciesCollection* pSpeciesCollection );
        InsecticideCollection( const InsecticideCollection& rMaster );
        virtual ~InsecticideCollection();

        virtual void CheckConfiguration() override;

        const jsonConfigurable::tDynamicStringSet& GetInsecticideNames() const;
        const Insecticide* GetInsecticide( const std::string& rName ) const;

    protected:
        virtual Insecticide* CreateObject() override;

        const VectorSpeciesCollection* m_pSpeciesCollection;
        jsonConfigurable::tDynamicStringSet m_InsecticideNames;
    };

    class InsecticideName : public jsonConfigurable::ConstrainedString
    {
    public:
        InsecticideName();
        InsecticideName( const InsecticideName& rMaster );
        ~InsecticideName();

        void CheckConfiguration( const std::string& rOwner, const std::string& rParameterName );
        const Insecticide* GetInsecticide() const;
    };
}
