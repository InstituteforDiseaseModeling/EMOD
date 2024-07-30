

#pragma once

#include "ReportNodeDemographicsMalaria.h"
#include "ParasiteGenome.h"

namespace Kernel
{
#define NUM_YEARS_FOR_BARCODES (1)

    // A class for containing the IdentityByState and IdentityByDecent information between two genomes.
    class GenomeComboMatrixColumn
    {
    public:
        GenomeComboMatrixColumn();
        GenomeComboMatrixColumn( const ParasiteGenome& rRowGenome, const ParasiteGenome& rColGenome );
        ~GenomeComboMatrixColumn();

        inline float GetFractionSameAllele() const { return m_FractionSameAllele; }
        inline float GetFractionSameRoot()   const { return m_FractionSameRoot;   }

    private:
        void CalculateFractionInfo( const ParasiteGenome& rRowGenome,
                                    const ParasiteGenome& rColGenome );

        //ParasiteGenome m_ColGenome;
        float m_FractionSameAllele;
        float m_FractionSameRoot;
    };

    // A structure for containing information about how one genome, the row genome,
    // is similar to a set of other genomes, the columns.  It is assumed that if row
    // genome is the Xth index in the matrix, then it is also the Xth column in the matrix.
    class GenomeComboMatrixRow
    {
    public:
        GenomeComboMatrixRow( const ParasiteGenome& rGenome, float timeAdded );
        ~GenomeComboMatrixRow();

        inline const   ParasiteGenome& GetRowGenome()  const { return m_RowGenome;                  }
        inline int64_t GetGenomeHash()                 const { return m_RowGenome.GetHashcode();    }
        inline float   GetTimeAdded()                  const { return m_TimeAdded;                  }
        inline float   GetRowFractionSameAlleleTotal() const { return m_RowFractionSameAlleleTotal; }
        inline float   GetRowFractionSameRootTotal()   const { return m_RowFractionSameRootTotal;   }

        const GenomeComboMatrixColumn& AddColumn( const ParasiteGenome& rColGenome );
        void AddColumn( const GenomeComboMatrixColumn& rCol );
        void RemoveColumn( int colIndex );

    private:
        ParasiteGenome m_RowGenome;
        std::vector<GenomeComboMatrixColumn> m_Columns;
        float m_TimeAdded;
        float m_RowFractionSameAlleleTotal;
        float m_RowFractionSameRootTotal;
    };

    // The purpose of this class is to keep track of the similarity between a collection of genomes.
    // One requirement is that one must be able to add and remove genomes from the collection.
    // It maintains a square, symmetrical matrix where the list of genomes for the rows is also the
    // list of genomes for the columns.  (This results in the diagonal always having a value of 1.0
    // for similarity.)
    // I created this class because I wanted fast acces and iterations that are provided by std::vector.
    // By keeping it square one only has to find the index of the genome once and then the index
    // can be reused for index of the column.  The use of the back()/pop_back() pattern keeps us from
    // trying to remove an element from an array while maintaining order.  However, this also removes
    // the benefit of the row genomes staying in the order that they were added.
    // NOTE:  This starts to use a lot of RAM and time when there are 10K genomes or more.
    class GenomeComboMatrix
    {
    public:
        GenomeComboMatrix( float timeWindow );
        ~GenomeComboMatrix();

        void AddGenome( const ParasiteGenome& rNewGenome, float currentTime );
        void RemoveRow( int index );
        void RemoveOld( float currentTime );
        void CalculateAverages( float* pAvgIBS, float* pAvgIBD );

        int CalculateNumCombinations( int n );
        int GetNumCombinations() const;
        float GetMatrixTimeWindow() const;
        int GetNumGenomes() const;

    private:
        int m_NumCombinations;
        float m_MatrixTimeWindow;
        std::vector<int64_t> m_RowsGenomeHash;
        std::vector<GenomeComboMatrixRow*> m_Rows;
    };


    class NodeDataMalariaGenetics : public NodeDataMalaria
    {
    public:
        NodeDataMalariaGenetics();
        virtual ~NodeDataMalariaGenetics();

        virtual void Reset();

        std::vector<ReportUtilitiesMalaria::BarcodeColumn*>      barcode_columns;
        std::map<int32_t,ReportUtilitiesMalaria::BarcodeColumn*> barcode_columns_map;
        ReportUtilitiesMalaria::BarcodeColumn                    barcode_other;

        std::vector<ReportUtilitiesMalaria::BarcodeColumn*>           drug_resistant_columns;
        std::map<std::string, ReportUtilitiesMalaria::BarcodeColumn*> drug_resistant_columns_map;
        ReportUtilitiesMalaria::BarcodeColumn                         drug_resistant_other;

        std::vector<ReportUtilitiesMalaria::BarcodeColumn*>           hrp_columns;
        std::map<std::string, ReportUtilitiesMalaria::BarcodeColumn*> hrp_columns_map;
        ReportUtilitiesMalaria::BarcodeColumn                         hrp_other;

        int32_t current_num_infections;
        std::set<uint32_t> current_bite_ids; // delivering infections
        int32_t current_num_bites_multi_inf; // bites delivering multiple infections
        std::set<int64_t> total_unique_genomes;
        std::set<int64_t> total_unique_barcodes;
        std::set<int64_t> current_unique_genomes;
        std::set<int64_t> current_unique_barcodes;
        std::vector<std::map<int64_t,float>> unique_genome_hash_2_first_appeared_by_year;
        std::vector<std::map<int64_t,float>> unique_barcode_hash_2_first_appeared_by_year;

        std::vector<std::vector<uint32_t>> num_occurrences_root_position;
        std::vector<std::pair<ParasiteGenome,float>> new_infections_last_year;

        GenomeComboMatrix genome_combo_matrix;
    };


    ENUM_DEFINE( DrugResistantStatType,
                 ENUM_VALUE_SPEC( NUM_PEOPLE_WITH_RESISTANT_INFECTION, 0 )
                 ENUM_VALUE_SPEC( NUM_INFECTIONS, 1 ) )

    class ReportNodeDemographicsMalariaGenetics: public ReportNodeDemographicsMalaria
    {
        DECLARE_FACTORY_REGISTERED( ReportFactory, ReportNodeDemographicsMalariaGenetics, IReport )
    public:
        ReportNodeDemographicsMalariaGenetics();
        virtual ~ReportNodeDemographicsMalariaGenetics();

        // ISupports
        virtual QueryResult QueryInterface( iid_t iid, void** pinstance ) override;
        virtual int32_t AddRef() override { return ReportNodeDemographics::AddRef(); }
        virtual int32_t Release() override { return ReportNodeDemographics::Release(); }

        // ReportNodeDemographics
        virtual bool Configure( const Configuration* ) override;
        virtual std::string GetHeader() const override;
        virtual void LogNodeData( INodeContext* pNC ) override;

        // IObserver
        virtual bool notifyOnEvent( IIndividualHumanEventContext *pEntity,
                                    const EventTrigger& trigger ) override;

    protected:
        virtual NodeData* CreateNodeData();
        virtual void WriteNodeData( const NodeData* pData );
        virtual void LogIndividualData( IIndividualHuman* individual, NodeData* pNodeData ) override;

        void UpdateIdentityData( float current_time,
                                 const ParasiteGenome& rGenome,
                                 NodeDataMalariaGenetics* pNDMG );


        std::vector<std::string> m_Barcodes;
        std::vector<std::string> m_DrugResistantStrings;
        std::vector<std::string> m_HrpStrings;
        DrugResistantStatType::Enum m_DrugResistantStatType;
        bool m_IncludeIdentityBy;
    };
}
