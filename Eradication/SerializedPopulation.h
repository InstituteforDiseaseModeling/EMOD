
#pragma once

#include <cstdint>
#include <iostream>
#include <numeric>
#include <vector>
#include "Simulation.h"
#include "ProgVersion.h"


namespace SerializedState {

    #define SERIAL_POP_VERSION  (5)

    enum Scheme
    {
        NONE = 0,
        SNAPPY = 1,
        LZ4 = 2,
    };
    
    struct Header
    {
        uint32_t version;
        std::string date;
        bool compressed;
        size_t byte_count;
        std::string compression;
        uint32_t chunk_count;
        std::vector<size_t> chunk_sizes;
        ProgDllVersion emod_info;

        Header();
        Header(const std::string& engine, const std::vector<size_t>& chunk_sizes);

        std::string ToString() const;
        json::Object ToJson() const;
        void Validate() const;
    };

    Kernel::ISimulation* LoadSerializedSimulation(const char* filename);
    void SaveSerializedSimulation(Kernel::Simulation* sim, uint32_t time_step, bool compress, bool use_full_precision);
    Kernel::ISimulation* ReadDtkVersion2(FILE* f, const char* filename, Header& header);
    Kernel::ISimulation* ReadDtkVersion34(FILE* f, const char* filename, Header& header);
    
    // Declare this function so the friend declarations in Simulation.h will work:
    Kernel::NodeMap_t& GetNodes( Kernel::Simulation* sim );

    FILE* OpenFileForReading( const char* filename );
    void ReadHeader( FILE* f, const char* filename, Header& header );
    void WriteHeader(const Header& header, FILE* f);
    FILE* OpenFileForWriting(const std::string& filename);

    void ConstructHeaderSize( const Header& header, std::string& size_string );
    void WriteMagicNumber( FILE* f );
    void WriteHeaderSize( const std::string& size_string, FILE* f );
    void GenerateFilename( uint32_t time_step, std::string& filename_with_path );
    void PrepareSimulationData( Kernel::Simulation* sim, std::vector<Kernel::IArchive*>& writers, std::vector<const char*>& json_texts, std::vector<size_t>& json_sizes, bool use_full_precision );
    void SetCompressionScheme( bool compress, const std::vector<size_t>& json_sizes, Scheme& compression_scheme, std::string& scheme_name );
    void PrepareChunkContents( const std::vector<const char*>& json, Scheme compression_scheme, const std::vector<size_t>& json_sizes, size_t& total, std::vector<size_t>& chunk_sizes, std::vector<std::string*>& compressed_data );
    void WriteChunks( Scheme compression_scheme, const std::vector<const char*>& json_texts, const std::vector<size_t>& chunk_sizes, const std::vector<std::string*>& compressed_data, FILE* f );
    void FreeMemory( std::vector<std::string*>& compressed_data, std::vector<Kernel::IArchive*>& writers );

    void WriteIdtkFileWrapper( Kernel::ISimulation* sim, uint32_t time_step, bool compress );
    void WriteIdtkFile( const char* data, size_t length, uint32_t time_step, bool compress );
    void CheckMagicNumber( FILE* f, const char* filename );
}
