
#pragma once

#include "BaseTextReport.h"
#include "IVectorMigrationReporting.h"
#include "ReportUtilitiesMalaria.h"
#include "suids.hpp"
#include "VectorGenome.h"
#include "JsonConfigurableCollection.h"
#include "ReportFilter.h"

#ifndef _REPORT_DLL
#include "ReportFactory.h"
#endif

namespace Kernel
{
    struct IVectorPopulationReporting;

    ENUM_DEFINE( StratifyBy,
                 ENUM_VALUE_SPEC( GENOME,          0 )
                 ENUM_VALUE_SPEC( SPECIFIC_GENOME, 1 )
                 ENUM_VALUE_SPEC( ALLELE,          2 )
                 ENUM_VALUE_SPEC( ALLELE_FREQ,     3 ))

    // NameToColumnData represents one row of stratified data in the report
    // for one time step and node.  A list of these structs is used a common
    // data structure to write whether we are outputing data via genome or allele.
    struct NameToColumnData
    {
        std::string name;             //either the genome name of the list of allele of interest
        std::vector<uint32_t> counts; //the data for each column in the row.
        std::vector<float> avg_age;
        std::vector<float> other;     //other columns by subclasses
    };

    // AlleleComboCount represents one row of data stratified by data time, node,
    // and allele combinations.  This should get cleared and updated each timestep
    // and node.
    struct AlleleComboCount
    {
        std::string name;                          // The name written to the Allele column.
        std::vector<std::string> allele_name_list; // The list of allele names that should be included in the genome
        uint32_t vector_pop;                       // infectious + infected + adult populations
        std::vector<uint32_t> state_counts;        // population for each vector state (Eggs, Larva, etc)
        uint32_t vector_pop_death;                 // infectious + infected + adult vectors that died
        float vector_pop_death_sum_age;            // the sum of the age of the vectors that died in vector_pop_death
        std::vector<uint32_t> death_counts;        // vectors that died per state
        std::vector<float> sum_age;                // sum of the age of the vectors that died per state

        AlleleComboCount();
        AlleleComboCount( const std::string& rAlleleName );
        AlleleComboCount( const std::vector<std::string>& rAlleleNameList );
        AlleleComboCount( const AlleleComboCount& rThat );
        ~AlleleComboCount();

        // Returns true if each allele in this combo exists in the genome name
        // They just have to exist.  Can be in either gamete.
        bool IsInGenome( const std::string& rGenomeName ) const;

        // Returns true if the allele_name_list has one value and that value
        // appears twice in the genome name.  Used with ALLELE_FREQ
        bool IsHomozygous( const std::string& rGenomeName ) const;
    };

    // The data in the model is saved for each vector state and genome.  That is,
    // we know how many vectors with a particular genome are in a given state.
    // When extracting data for allele output, we get the data for a genome out
    // of the model and then update the counts for the allele combos that are in
    // this genome.
    struct GenomeToAlleleComboCount
    {
        VectorGenome genome;     // possible genome
        std::string genome_name; // string representation of the genome
        std::vector<AlleleComboCount*> allele_counts; // AlleleCombos that are in this genome
    };

    // A class for reading an allele combination that is a two dimensional array of strings.
    // The outer array should contain one array for each gene/locus.  The inner arrays should
    // have exactly two values.  Each value must be an allele as defined in the Genes configuration.
    // A value can be '*' which means all of the alleles at that locus.  The array for a given
    // locus cannot have both values as '*'.  To imply all combinations for a given locus,
    // the user just does not define an array for that locus.  That is, only define the loci
    // with restrictions.
    class RVG_AlleleCombo : public JsonConfigurable
    {
    public:
        RVG_AlleleCombo();
        virtual ~RVG_AlleleCombo();

        //JsonConfigurable
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) { return e_NOINTERFACE; }
        IMPLEMENT_NO_REFERENCE_COUNTING()
        virtual bool Configure( const Configuration* config ) override;

        // Other
        const std::vector<std::vector<std::string>>& GetComboStrings() const;

    private:
        std::vector<std::vector<std::string>> m_ComboStrings;
    };

    // This contains a collection of AlleleCombos.
    class AlleleComboCollection : public JsonConfigurableCollection<RVG_AlleleCombo>
    {
    public:
        AlleleComboCollection();
        virtual ~AlleleComboCollection();

        virtual void CheckConfiguration() override;

    protected:
        virtual RVG_AlleleCombo* CreateObject() override;
    };


    class ReportVectorGenetics : public BaseTextReport
    {
#ifndef _REPORT_DLL
        DECLARE_FACTORY_REGISTERED( ReportFactory, ReportVectorGenetics, IReport )
#endif
    public:
        ReportVectorGenetics();
        virtual ~ReportVectorGenetics();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance );
        virtual int32_t AddRef() { return BaseTextReport::AddRef(); }
        virtual int32_t Release() { return BaseTextReport::Release(); }

        // BaseEventReport
        virtual bool Configure( const Configuration* ) override;
        virtual void CheckForValidNodeIDs( const std::vector<ExternalNodeId_t>& nodeIds_demographics ) override;

        virtual std::string GetHeader() const override;
        virtual void LogNodeData( Kernel::INodeContext * pNC ) override;

    protected:
        ReportVectorGenetics( const std::string& rReportName );
        virtual void ResetOtherCounters() {};
        virtual void CollectOtherDataByGenome( NameToColumnData& rColumnData ) {};

        void UpdateReportName();
        void CheckSpecificGenomeCombination();
        bool IncludeVectorStateColumn( VectorStateEnum::Enum state ) const;

        std::vector<PossibleGenome> FindSpecificGenomes( IVectorPopulationReporting* pIVPR );
        std::vector<AlleleComboCount> CreateAlleleComboCounts( IVectorPopulationReporting* pIVPR );
        void LinkGenomesToAlleleComboCounts( const std::vector<PossibleGenome>& rPossibleGenomes,
                                             const std::vector<AlleleComboCount>& rAlleleComboCounts,
                                             std::vector<GenomeToAlleleComboCount>& rGenomeToAlleleComboCountVector );

        virtual void InitializeStratificationData( IVectorPopulationReporting* pIVPR );
        NameToColumnData CreateColumnData( const std::string& rName,
                                           uint32_t vectorPop,
                                           const std::vector<uint32_t>& rStateCounts,
                                           uint32_t vectorPopDeath,
                                           float vectorPopSumAge,
                                           const std::vector<uint32_t>& rDeathCount,
                                           const std::vector<float>& rSumAge );
        std::vector<NameToColumnData> CollectDataByAllele( IVectorPopulationReporting* pIVPR );
        std::vector<NameToColumnData> CollectDataByGenome( IVectorPopulationReporting* pIVPR );

        bool m_IncludeVectorStateColumns;
        bool m_IncludeDeathByStateColumns;
        bool m_CombineSimilarGenomes;
        bool m_HasStratificationData;
        StratifyBy::Enum m_StratifyBy;
        AlleleComboCollection m_SpecificGenomeCombination;
        std::vector<std::vector<std::string>> m_GenomeHasAlleleCombination;
        std::string m_Species;
        VectorGender::Enum m_Gender;
        std::vector<PossibleGenome> m_PossibleGenomes;
        std::vector<AlleleComboCount> m_AlleleComboCounts;
        std::vector<GenomeToAlleleComboCount> m_GenomeToAlleleComboCountVector;
        std::vector<std::string> m_AllelesForStratification;
        ReportFilter m_ReportFilter;
    };
}
