#pragma once

#include "Configure.h"
#include "JsonConfigurableCollection.h"
#include "GeneticProbability.h"

namespace Kernel
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // !!! These classes will support reading a GeneticProbability data for either
    // !!! a single species or for a collection of them.  Pass the collection when
    // !!! you want the GeneticProbability to read data from multiple species or
    // !!! pass in a single VectorSpeciesParameters if the data for that species.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    class VectorSpeciesCollection;
    class VectorSpeciesParameters;

    // Used to convert JSON to AlleleComboProbability
    class AlleleComboProbabilityConfig : public JsonConfigurable
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
    public:
        AlleleComboProbabilityConfig( const VectorSpeciesParameters* pVsp );
        AlleleComboProbabilityConfig( const VectorSpeciesCollection* pSpeciesCollection );
        AlleleComboProbabilityConfig( const AlleleComboProbabilityConfig& rMaster );
        virtual ~AlleleComboProbabilityConfig();

        virtual bool Configure( const Configuration *json ) override;

        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }

        const AlleleComboProbability& GetProbability() const;

    private:
        const VectorSpeciesParameters* m_pVectorSpeciesParameters;
        const VectorSpeciesCollection* m_pSpeciesCollection;
        AlleleComboProbability m_acp;
    };

    // Read a collection of AlleleComboProbabilityConfig's so that you can get
    // a single GeneticProbability
    class AlleleComboProbabilityConfigCollection : public JsonConfigurableCollection<AlleleComboProbabilityConfig>
    {
    public:
        AlleleComboProbabilityConfigCollection( const VectorSpeciesParameters* pVsp );
        AlleleComboProbabilityConfigCollection( const VectorSpeciesCollection* pSpeciesCollection );
        AlleleComboProbabilityConfigCollection( const AlleleComboProbabilityConfigCollection& rMaster );
        virtual ~AlleleComboProbabilityConfigCollection();

        virtual void CheckConfiguration() override;

        const GeneticProbability& GetProbability() const;

    protected:
        virtual AlleleComboProbabilityConfig* CreateObject() override;

        const VectorSpeciesParameters* m_pVectorSpeciesParameters;
        const VectorSpeciesCollection* m_pSpeciesCollection;
    };

    // Read/convert JSON to a GeneticProbability
    class GeneticProbabilityConfig : public JsonConfigurable
    {
        IMPLEMENT_NO_REFERENCE_COUNTING()
    public:
        GeneticProbabilityConfig( const VectorSpeciesParameters* pVsp );
        GeneticProbabilityConfig( const VectorSpeciesCollection* pSpeciesCollection );
        GeneticProbabilityConfig( const GeneticProbabilityConfig& rMaster );
        virtual ~GeneticProbabilityConfig();

        virtual bool Configure( const Configuration *json ) override;

        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }

        const GeneticProbability& GetProbability() const;

    private:
        const VectorSpeciesParameters* m_pVectorSpeciesParameters;
        const VectorSpeciesCollection* m_pSpeciesCollection;
        AlleleComboProbabilityConfigCollection m_ACPConfigCollection;
        GeneticProbability m_Probability;
    };
}
