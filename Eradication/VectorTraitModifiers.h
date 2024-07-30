
#pragma once

#include "EnumSupport.h"
#include "Configure.h"
#include "JsonConfigurableCollection.h"
#include "VectorGenome.h"

namespace Kernel
{
    class VectorGeneCollection;

    ENUM_DEFINE(VectorTrait,
        ENUM_VALUE_SPEC( INFECTED_BY_HUMAN,     0) // Modify the probability of the vector getting infected.
        ENUM_VALUE_SPEC( FECUNDITY,             1) // Modify the number of eggs laid
        ENUM_VALUE_SPEC( FEMALE_EGG_RATIO,      2) // Modify the number of eggs that produce female vectors versus male vectors.
        ENUM_VALUE_SPEC( STERILITY,             3) // 0 => vector is sterile, != 0 => not sterile
        ENUM_VALUE_SPEC( TRANSMISSION_TO_HUMAN, 4) // modify the probability that sporosoites in the salivary gland infect a human individual
        ENUM_VALUE_SPEC( ADJUST_FERTILE_EGGS,   5) // Modify the egg generation for some eggs being non-fertile
        ENUM_VALUE_SPEC( MORTALITY,             6) // Modify the rate at which a vector dies - modifier/XXX_Life_Expectancy
        ENUM_VALUE_SPEC( INFECTED_PROGRESS,     7) // Modify the infected_progress_this_timestep - the amount the vectors progress
                                                   // from infected to infectious each timestep, essentially the time for oocysts to become sporozoites
        ENUM_VALUE_SPEC( OOCYST_PROGRESSION,    8) // If vector has a parasite that matches the barcode, modify the temperature dependent progression
                                                   // This is another multiplier on top of INFECTED_PROGRESS.
        ENUM_VALUE_SPEC( SPOROZOITE_MORTALITY,  9))// If vector has a parasite that matches the barcode, modify the rate a which the
                                                   // sporozoites die - Sporozoite_Life_Expectancy

    class TraitModifier : public JsonConfigurable
    {
    public:
        TraitModifier();
        virtual ~TraitModifier();

        //ISupports
        virtual QueryResult QueryInterface(iid_t iid, void **ppvObject) { return e_NOINTERFACE; }
        IMPLEMENT_NO_REFERENCE_COUNTING()

        virtual bool Configure( const Configuration* config ) override;

        VectorTrait::Enum GetTrait() const;
        float GetModifier() const;
        const std::vector<int64_t>& GetPossibleParasiteBarcodeHashesA() const;
        const std::vector<int64_t>& GetPossibleParasiteBarcodeHashesB() const;

    protected:
        void CheckForEmptyBarcode( VectorTrait::Enum trait,
                                   const std::string& rParameterName,
                                   const std::string& rBarcode ) const;
        std::vector<int64_t> ConvertBarcode( VectorTrait::Enum trait,
                                             const std::string& rParameterName,
                                             const std::string& rBarcode ) const;


        VectorTrait::Enum m_Trait;
        float m_Modifier;
        std::vector<int64_t> m_PossibleParasiteBarcodeHashesA;
        std::vector<int64_t> m_PossibleParasiteBarcodeHashesB;
    };

    // This class is a collection of TraitModifier objects that will be associated with
    // a particular 'Allele_Combinations'.
    class TraitModifierCollection : public JsonConfigurableCollection<TraitModifier>
    {
    public:
        TraitModifierCollection();
        virtual ~TraitModifierCollection();

        virtual void CheckConfiguration() override;

    protected:
        virtual TraitModifier* CreateObject() override;
    };

    // This reads an entry in 'Gene_To_Trait_Modifiers' array in the config file.
    class GeneToTraitModifierConfig : public JsonConfigurable
    {
    public:
        GeneToTraitModifierConfig( VectorGeneCollection* pGenes );
        virtual ~GeneToTraitModifierConfig();

        //JsonConfigurable
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) { return e_NOINTERFACE; }
        IMPLEMENT_NO_REFERENCE_COUNTING()
        virtual bool Configure( const Configuration* config ) override;

        // other
        VectorGameteBitPair_t GetGenomeBitMask() const;
        const std::vector<VectorGameteBitPair_t>& GetPossibleGenomes() const;
        const TraitModifierCollection& GetTraitModifiers() const;

    private:
        VectorGeneCollection* m_pGenes;
        std::vector<std::vector<std::string>> m_ComboStrings;
        VectorGameteBitPair_t m_GenomeBitMask;
        std::vector<VectorGameteBitPair_t> m_PossibleGenomes;
        TraitModifierCollection m_TraitModifiers;
    };

    // Used to determine if a trait is modified for a given set of alleles
    class GeneToTraitModifier
    {
    public:
        GeneToTraitModifier( VectorTrait::Enum trait,
                             const VectorGameteBitPair_t& rBitMask,
                             const std::vector<VectorGameteBitPair_t>& rPossibleGenomes,
                             const std::vector<int64_t>& rPossibleParasiteBarcodeHashesA,
                             const std::vector<int64_t>& rPossibleParasiteBarcodeHashesB,
                             float modifier );
        ~GeneToTraitModifier();

        VectorTrait::Enum GetTrait() const;
        bool IsTraitModified( const VectorGenome& rGenome,
                              int64_t parasiteBarcodeHashA,
                              int64_t parasiteBarcodeHashB ) const;
        float GetModifier() const;

    private:
        VectorTrait::Enum                  m_Trait;
        VectorGameteBitPair_t              m_BitMask;
        std::vector<VectorGameteBitPair_t> m_PossibleGenomes;
        std::vector<int64_t>               m_PossibleParasiteBarcodeHashesA;
        std::vector<int64_t>               m_PossibleParasiteBarcodeHashesB;
        float                              m_Modifier;
    };

    // This class is responsible for determining the modifier for a trait for a
    // species given a vector's genome.  When a vector needs the trait information,
    // VectorTraitModifiers will be used to see how to modify the trait for the
    // given VectorGenome.
    class VectorTraitModifiers : public JsonConfigurableCollection<GeneToTraitModifierConfig>
    {
    public:
        VectorTraitModifiers( VectorGeneCollection* pGenes );
        virtual ~VectorTraitModifiers();

        // JsonConfigurableCollection
        virtual void CheckConfiguration() override;

        // Other
        void AddModifier( GeneToTraitModifier* pNewModifier );
        float GetModifier( VectorTrait::Enum trait,
                           const VectorGenome& rGenome,
                           int64_t parasiteBarcodeHashA = 0,
                           int64_t parasiteBarcodeHashB = 0 ) const;

    protected:
        virtual GeneToTraitModifierConfig* CreateObject() override;

    private:
        VectorGeneCollection* m_pGenes;
        std::vector<std::vector<GeneToTraitModifier*>> m_Modifiers;
    };
}
