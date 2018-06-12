/***************************************************************************************************

Copyright (c) 2018 Intellectual Ventures Property Holdings, LLC (IVPH) All rights reserved.

EMOD is licensed under the Creative Commons Attribution-Noncommercial-ShareAlike 4.0 License.
To view a copy of this license, visit https://creativecommons.org/licenses/by-nc-sa/4.0/legalcode

***************************************************************************************************/

#include "stdafx.h"
#include "SerializedPopulation.h"

#include <ctime>
#include "Debug.h"  // release_assert
#include "Exceptions.h"
#include "FileSystem.h"
#include <iomanip>
#include "JsonObject.h"
#include "JsonFullReader.h"
#include "JsonFullWriter.h"
#include "Log.h"
#include <lz4.h>
#include "NoCrtWarnings.h"
#include <numeric>
#include "Node.h"
#include <snappy.h>
#include "Sugar.h"  // EnvPtr
#include "Simulation.h"

SETUP_LOGGING( "SerializedPopulation" )

namespace SerializedState {
    enum Scheme
    {
        NONE = 0,
        SNAPPY = 1,
        LZ4 = 2,
    };

    #define SERIAL_POP_VERSION  (4)

    struct Header
    {
        uint32_t version;
        string date;
        bool compressed;
        size_t byte_count;
        string compression;
        uint32_t chunk_count;
        vector<size_t> chunk_sizes;

        Header() {}
        Header(const string& engine, const vector<size_t>& chunk_sizes)
            : version(SERIAL_POP_VERSION)
            , compressed(engine != "NONE")
            , byte_count(std::accumulate(chunk_sizes.begin(), chunk_sizes.end(), size_t(0)))
            , compression(engine)
            , chunk_count(chunk_sizes.size())
            , chunk_sizes(chunk_sizes)
        {
            std::time_t now = std::time(nullptr);
            struct tm gmt;
            gmtime_s(&gmt, &now);
            char asc_time[256];
            strftime(asc_time, sizeof( asc_time ), "%a %b %d %H:%M:%S %Y", &gmt);
            date = asc_time;
        }

        string ToString() const {
            std::ostringstream temp_stream;
            temp_stream << '{'
                << "\"version\":" << version << ','
                << "\"author\":\"IDM\","
                << "\"tool\":\"DTK\","
                << "\"date\":" << '"' << date.c_str() << "\","
                << "\"compression\":\"" << compression.c_str() << "\","
                << "\"bytecount\":" << byte_count << ','
                << "\"chunkcount\":" << chunk_count << ','
                << "\"chunksizes\":[";
            temp_stream << chunk_sizes[0];
            for (size_t i = 1; i < chunk_sizes.size(); ++i)
            {
                temp_stream << ',' << chunk_sizes[i];
            }
            temp_stream << ']'  // chunksizes
                << '}';         // root

            return temp_stream.str();
        }

        void Validate() const
        {
            switch (version)
            {
                case 1:
                case 2:
                case 3:
                case 4:
                    // Check specified compression engine
                    if ( (compression != "NONE") && (compression != "LZ4") && (compression != "SNAPPY"))
                    {
                        ostringstream msg;
                        msg << "Unknown compression scheme, '" << compression.c_str() << "', specified in header." << std::endl;
                        throw Kernel::SerializationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
                    }

                    // Check chunk_count matches number of chunk_sizes
                    if ( chunk_sizes.size() != chunk_count )
                    {
                        ostringstream msg;
                        msg << "Chunk count, " << chunk_count << ", does not match size of chunksizes array, " << chunk_sizes.size() << ", specified in header." << std::endl;
                        throw Kernel::SerializationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
                    }
                break;

                default:
                {
                    ostringstream msg;
                    msg << "Unexpected version found in header: " << version << std::endl;
                    throw Kernel::SerializationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
                }
            }
        }
    };

    void GenerateFilename(uint32_t time_step, string& filename_with_path);
    FILE* OpenFileForWriting(const string& filename);
    void PrepareSimulationData(Kernel::ISimulation* sim, vector<Kernel::IArchive*>& writers, vector<const char*>& json_texts, vector<size_t>& json_sizes);
    void PrepareNodeData(Kernel::Simulation::NodeMap_t& nodes, vector<Kernel::IArchive*>& writers, vector<const char*>& json_texts, vector<size_t>& json_sizes);
    void SetCompressionScheme(bool compress, const vector<size_t>& json_sizes, Scheme& compression_scheme, string& scheme_name);
    void PrepareChunkContents(const vector<const char*>& json, Scheme compression_scheme, const vector<size_t>& json_sizes, size_t& total, vector<size_t>& chunk_sizes, vector<string*>& compressed_data);
    void ConstructHeaderSize(const Header& header, string& size_string);
    void WriteMagicNumber(FILE* f);
    void WriteHeaderSize(const string& size_string, FILE* f);
    void WriteHeader(const Header& header, FILE* f);
    void WriteChunks(Scheme compression_scheme, const vector<const char*>& json_texts, const vector<size_t>& chunk_sizes, const vector<string*>& compressed_data, FILE* f);
    void FreeMemory(vector<string*>& compressed_data, vector<Kernel::IArchive*>& writers);

    void WriteIdtkFileWrapper(Kernel::ISimulation* sim, uint32_t time_step, bool compress);
    void WriteIdtkFile(const char* data, size_t length, uint32_t time_step, bool compress);

    void SaveSerializedSimulation(Kernel::Simulation* sim, uint32_t time_step, bool compress)
    {
        string filename;
        GenerateFilename(time_step, filename);
        LOG_INFO_F("Writing state to '%s'\n", filename.c_str());
        FILE* f = OpenFileForWriting(filename);

        try {
            vector<Kernel::IArchive*> writers;

            vector<const char*> json_texts;
            vector<size_t> json_sizes;

            PrepareSimulationData(sim, writers, json_texts, json_sizes);
            PrepareNodeData(sim->nodes, writers, json_texts, json_sizes);

            Scheme scheme = LZ4;
            string scheme_name("LZ4");
            SetCompressionScheme(compress, json_sizes, scheme, scheme_name);

            size_t total = 0;
            vector<string*> compressed_data;
            vector<size_t> chunk_sizes;

            PrepareChunkContents(json_texts, scheme, json_sizes, total, chunk_sizes, compressed_data);

            Header header(scheme_name, chunk_sizes);
            string size_string;
            ConstructHeaderSize(header, size_string);

            WriteMagicNumber(f);
            WriteHeaderSize(size_string, f);
            WriteHeader(header, f);
            WriteChunks(scheme, json_texts, chunk_sizes, compressed_data, f);

            fflush(f);
            fclose(f);
        
            // _Don't_ clean up vector<const char*> json_text - they just point into the writers
            FreeMemory(compressed_data, writers);
        }
        catch (...) {
            fclose(f);
            ostringstream msg;
            msg << "Exception writing serialized population file '" << filename.c_str() << "'." << std::endl;
            LOG_ERR( msg.str().c_str() );

            // We do not know what went wrong or exactly what the exception is. We will just pass it along.
            throw;  // Kernel::SerializationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
    }

    void GenerateFilename(uint32_t time_step, string& filename_with_path)
    {
        // Make sure length accounts for "state-#####-###.dtk" (19 chars)
        size_t length = strlen("state-#####-###.dtk") + 1;  // Don't forget the null terminator (for sprintf_s)!
        string filename;
        filename.resize(length);
        char* buffer = &*filename.begin();

        if (EnvPtr->MPI.NumTasks == 1)
        {
            // Don't bother with rank suffix if only one task.
            sprintf_s(buffer, length, "state-%05d.dtk", time_step);
        }
        else
        {   // Add rank suffix when running multi-core.
            sprintf_s(buffer, length, "state-%05d-%03d.dtk", time_step, EnvPtr->MPI.Rank);
        }

        filename_with_path = FileSystem::Concat<string>(EnvPtr->OutputPath, filename);
    }

    FILE* OpenFileForWriting(const string& filename)
    {
        FILE* f = nullptr;
        errno = 0;
        bool opened = (fopen_s(&f, filename.c_str(), "wb") == 0);

        if (!opened)
        {
            std::stringstream ss;
            ss << "Received error '" << FileSystem::GetSystemErrorMessage() << "' while opening file for writing.";
            throw Kernel::FileIOException( __FILE__, __LINE__, __FUNCTION__, filename.c_str(), ss.str().c_str() );
        }

        return f;
    }

    void PrepareSimulationData(Kernel::ISimulation* sim, vector<Kernel::IArchive*>& writers, vector<const char*>& json_texts, vector<size_t>& json_sizes)
    {
        Kernel::IArchive* writer = static_cast<Kernel::IArchive*>(new Kernel::JsonFullWriter(false));
        (*writer) & sim;

        writers.push_back(writer);
        json_texts.push_back(writer->GetBuffer());
        json_sizes.push_back(writer->GetBufferSize());
    }

    void PrepareNodeData(Kernel::Simulation::NodeMap_t& nodes, vector<Kernel::IArchive*>& writers, vector<const char*>& json_texts, vector<size_t>& json_sizes)
    {
        for (auto& entry : nodes)
        {
            // entry.first is const which doesn't play well with IArchive operator '&'
            Kernel::suids::suid suid(entry.first);
            Kernel::IArchive* writer = static_cast<Kernel::IArchive*>(new Kernel::JsonFullWriter(false));
            (*writer) & entry.second;

            writers.push_back(writer);
            json_texts.push_back(writer->GetBuffer());
            json_sizes.push_back(writer->GetBufferSize());
        }
    }

    void SetCompressionScheme(bool compress, const vector<size_t>& json_sizes, Scheme& scheme, string& scheme_name)
    {
    // #define LZ4_MAX_INPUT_SIZE      (0x7E000000) // defined in lz4.h
    #define SNAPPY_MAX_INPUT_SIZE   (0xFFFFFFFF)

        // check json_size for lz4 limit (0x7E000000)
        // drop back to snappy if possible (0x7E000000 < size <= 0xFFFFFFFF)
        // disable compression if necessary
        if (compress)
        {
            for (size_t size : json_sizes)
            {
                if ((scheme == LZ4) && (size > LZ4_MAX_INPUT_SIZE))
                {
                    // If the payload is too big, drop back to snappy.
                    // Note that the if check below will check for snappy limits.
                    scheme = SNAPPY;
                    scheme_name = "SNAPPY";
                }

                if ((scheme == SNAPPY) && (size > SNAPPY_MAX_INPUT_SIZE))
                {
                    scheme = NONE;
                    scheme_name = "NONE";
                    break;
                }
            }
        }
        else
        {
            scheme = NONE;
            scheme_name = "NONE";
        }
    }

    void PrepareChunkContents(const vector<const char*>& json_texts, Scheme compression_scheme, const vector<size_t>& json_sizes, size_t& total, vector<size_t>& chunk_sizes, vector<string*>& compressed_data)
    {
        for (size_t i = 0; i < json_texts.size(); ++i)
        {
            switch (compression_scheme)
            {
                case NONE:
                {
                    // No compression, just use the raw JSON text.
                    size_t size = json_sizes[i];
                    total += size;
                    chunk_sizes.push_back(size);
                }
                break;

                case LZ4:
                {
                    size_t source_size = json_sizes[i];
                    size_t required = LZ4_COMPRESSBOUND(source_size);
                    string* chunk = new string(required, 0);
                    char* buffer = &*(*chunk).begin();
                    *reinterpret_cast<uint32_t*>(buffer) = uint32_t(source_size);
                    char* payload = buffer + sizeof(uint32_t);
                    size_t compressed_size = LZ4_compress_default(json_texts[i], payload, int32_t(source_size), int32_t(required));
                    // Python binding for LZ4 writes the source_size first, account for that space
                    size_t payload_size = compressed_size + sizeof(uint32_t);
                    (*chunk).resize(payload_size);
                    total += payload_size;
                    compressed_data.push_back(chunk);
                    chunk_sizes.push_back(payload_size);
                }
                break;

                case SNAPPY:
                {
                    string* buffer = new string();
                    size_t compressed_size = snappy::Compress(json_texts[i], json_sizes[i], buffer);
                    total += compressed_size;
                    compressed_data.push_back(buffer);
                    chunk_sizes.push_back(compressed_size);
                }
                break;

                default:
                {
                    ostringstream msg;
                    msg << "Unknown compression scheme chosen: " << (uint32_t)compression_scheme << std::endl;
                    throw Kernel::SerializationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
                }
            }
        }
    }

    void ConstructHeaderSize(const Header& header, string& size_string)
    {
        std::ostringstream temp;
        uint32_t header_size = header.ToString().size();
        temp << setw(12) << right << header_size;
        size_string = temp.str();
    }

    void WriteMagicNumber(FILE* f)
    {
        fwrite("IDTK", 1, 4, f);
    }

    void WriteHeaderSize(const string& size_string, FILE* f)
    {
        fwrite(size_string.c_str(), 1, size_string.length(), f);
    }

    void WriteHeader(const Header& header, FILE* f)
    {
        string header_string = header.ToString();
        fwrite(header_string.c_str(), 1, header_string.length(), f);
    }

    void WriteChunks(Scheme compression_scheme, const vector<const char*>& json_texts, const vector<size_t>& chunk_sizes, const vector<string*>& compressed_data, FILE* f) 
    {
        switch (compression_scheme)
        {
        case NONE:
            for (size_t i = 0; i < json_texts.size(); ++i)
            {
                fwrite(json_texts[i], sizeof(char), chunk_sizes[i], f);
                // Nothing to clean up.
            }
            break;

        case LZ4:
        case SNAPPY:
            for (size_t i = 0; i < compressed_data.size(); ++i)
            {
                fwrite((*compressed_data[i]).c_str(), sizeof(char), chunk_sizes[i], f);
            }
            break;

        default:
            {
                ostringstream msg;
                msg << "Unexpected compression_scheme: " << uint32_t(compression_scheme) << '.' << std::endl;
                throw Kernel::SerializationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
            }
        }
    }

    void FreeMemory(vector<string*>& compressed_data, vector<Kernel::IArchive*>& writers)
    {
        for (auto entry : compressed_data)
        {
            delete entry;
        }

        for (auto pointer : writers)
        {
            delete pointer;
        }
    }


    /*************************************************************************************************/


    FILE* OpenFileForReading(const char* filename);
    void CheckMagicNumber(FILE* f, const char* filename);
    void ReadHeader(FILE* f, const char* filename, Header& header);
    // Kernel::ISimulation* ReadDtkVersion2(FILE* f, Header& header);   // Forward declared in SerializedPopulation.h


    Kernel::ISimulation* LoadSerializedSimulation(const char* filename)
    {
        Kernel::ISimulation* newsim = nullptr;

        FILE* f = OpenFileForReading(filename);

        try
        {
            CheckMagicNumber(f, filename);
            Header header;
            ReadHeader(f, filename, header);

            switch (header.version)
            {
            case 1:
                // ReadHeader() will fill in values so ReadDtkVersion2() will work correctly.
                // See comment in ReadHeader() below for differences between version 1 and version 2.
            case 2:
                newsim = ReadDtkVersion2(f, filename, header);
                break;

            case 3:
            case 4:
                newsim = ReadDtkVersion34(f, filename, header);
                break;

            default:
                {
                    ostringstream msg;
                    msg << "Unrecognized version in serialized population file header: " << header.version << std::endl;
                    throw Kernel::SerializationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
                }
            }

            fclose( f );
        }
        catch (...)
        {
            fclose( f );
            ostringstream msg;
            msg << "Unexpected error reading serialized population file, '" << filename << "'." << std::endl;
            LOG_ERR( msg.str().c_str() );

            // We do not know what went wrong or exactly what the exception is. We will just pass it along.
            throw;  // Kernel::SerializationException( __FILE__, __LINE__, __FUNCTION__, msg.c_str() );
        }

        return newsim;
    }

    FILE* OpenFileForReading(const char* filename)
    {
        FILE* f = nullptr;
        errno = 0;
        bool opened = (fopen_s(&f, filename, "rb") == 0);

        if (!opened)
        {
            std::stringstream ss;
            ss << "Received error '" << FileSystem::GetSystemErrorMessage() << "' while opening file for reading.";
            throw Kernel::FileIOException( __FILE__, __LINE__, __FUNCTION__, filename, ss.str().c_str() );
        }

        return f;
    }

    uint32_t ReadMagicNumber(FILE* f, const char* filename)
    {
        uint32_t magic = 0x0BADBEEF;
        size_t count = 1;
        size_t size = sizeof( magic );
        size_t bytes_read = fread(&magic, count, size, f);
        if (bytes_read != (count * size))
        {
            ostringstream msg;
            msg << " read " << bytes_read << " of " << (count * size) << " bytes for magic number";
            throw Kernel::FileIOException( __FILE__, __LINE__, __FUNCTION__, filename, msg.str().c_str() );
        }

        return magic;
    }

    void CheckMagicNumber(FILE* f, const char* filename)
    {
        uint32_t magic = ReadMagicNumber(f, filename);
        bool valid = (magic == *(uint32_t*)"IDTK");
        if (!valid)
        {
            ostringstream msg;
            msg << "Serialized population file has wrong magic number, expected 0x" << std::hex << std::setw(8) << std::setfill('0') << *(uint32_t*)"IDTK";
            msg << " 'IDTK', got 0x" << std::hex << std::setw(8) << std::setfill('0') << magic << '.' << std::endl;
            throw Kernel::SerializationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }
    }

    uint32_t ReadHeaderSize(FILE* f, const char* filename)
    {
        char size_string[13];   // allocate space for null terminator
        size_t byte_count = sizeof( size_string ) - 1;
        size_t bytes_read = fread(size_string, 1, byte_count, f); // don't read null terminator
        if (bytes_read != byte_count)
        {
            ostringstream msg;
            msg << " read " << bytes_read << " of " << byte_count << " bytes for header";
            throw Kernel::FileIOException( __FILE__, __LINE__, __FUNCTION__, filename, msg.str().c_str() );
        }
        size_string[byte_count] = '\0';
        errno = 0;
        uint32_t header_size = strtoul(size_string, nullptr, 10);
        if (errno == ERANGE)
        {
            ostringstream msg;
            msg << "Could not convert header size from text ('" << size_string << "') to unsigned integer." << std::endl;
            throw Kernel::SerializationException( __FILE__, __LINE__, __FUNCTION__, msg.str().c_str() );
        }

        return header_size;
    }

    void CheckHeaderSize(uint32_t header_size)
    {
        if (header_size == 0)
        {
            string msg( "Serialized population header size is 0, which is not valid.\n" );
            throw Kernel::SerializationException( __FILE__, __LINE__, __FUNCTION__, msg.c_str() );
        }
    }

    void ReadHeader(FILE* f, const char* filename, Header& header)
    {
        /*
        Version 1 schema:

            { "metadata": {
                "version": 1,
                "date": "Day Mon day HH:MM:SS year",
                "compressed": true|false,
                "bytecount": #
            }

        Version 2|3 schema:

            { "metadata": {
                "version": 2|3,
                "date": "Day Mon day HH:MM:SS year",
                "compressed": true|false,
                "engine": NONE|LZ4|SNAPPY,
                "bytecount": #,
                "chunkcount": #,
                "chunksizes": [ #, #, #, ..., # ]
            }

        Version 4 schema:

            {
                "version": 4,
                "date": "Day Mon day HH:MM:SS year",
                "compression": NONE|LZ4|SNAPPY,
                "bytecount": #,
                "chunkcount": #,
                "chunksizes": [ #, #, #, ..., # ]
            }

        */

        uint32_t count = ReadHeaderSize(f, filename);
        CheckHeaderSize(count);
        string header_text;
        header_text.resize(count);
        char* buffer = &*header_text.begin();
        size_t bytes_read = fread(buffer, 1, count, f);
        if (bytes_read != count)
        {
            ostringstream msg;
            msg << "read " << bytes_read << " of " << count << " bytes for header";
            throw Kernel::FileIOException( __FILE__, __LINE__, __FUNCTION__, filename, msg.str().c_str() );
        }

        Kernel::IJsonObjectAdapter* adapter = Kernel::CreateJsonObjAdapter();
        adapter->Parse(header_text.c_str());
        // TODO clorton - wrap this with try/catch for JSON errors
        auto& file_info = (*adapter).Contains("metadata") ? *((*adapter)["metadata"]) : *adapter;
        header.version = file_info.GetUint("version");
        header.date = file_info.GetString("date");
        header.byte_count = file_info.GetUInt64("bytecount");

        switch (header.version)
        {
            case 1:
            {
                // Fill in V2 values for a V1 header.
                header.compressed = file_info.GetBool("compressed");
                header.compression = header.compressed ? "SNAPPY" : "NONE";
                header.chunk_count = 1;
                header.chunk_sizes.push_back(header.byte_count);
            }
            break;

            case 2:
            case 3:
            case 4:
            {
                header.compression = file_info.GetString((header.version < 4) ? "engine" : "compression");
                header.compressed = (header.compression != "NONE");
                header.chunk_count = file_info.GetUint("chunkcount");
                auto& chunk_sizes = *(file_info.GetJsonArray("chunksizes"));
                for (size_t i = 0; i < chunk_sizes.GetSize(); ++i)
                {
                    auto& item = *(chunk_sizes[Kernel::IndexType(i)]);
                    size_t chunk_size = item.AsUint64();
                    header.chunk_sizes.push_back(chunk_size);
                }
            }
            break;

            default:
                // Do nothing as header.Validate() below will catch this case.
                break;
        }

        header.Validate();

        delete adapter;
    }

    void ReadChunk(FILE* f, size_t byte_count, const char* filename, vector<char>& chunk)
    {
        chunk.resize(byte_count);
        size_t bytes_read = fread(chunk.data(), 1, byte_count, f);
        if (bytes_read != byte_count)
        {
            ostringstream msg;
            msg << "read " << bytes_read << " of " << byte_count << " bytes for chunk";
            throw Kernel::FileIOException( __FILE__, __LINE__, __FUNCTION__, filename, msg.str().c_str() );
        }
    }

    namespace lz4
    {
        // source is a block of LZ4 compressed data with the size of the uncompressed data prepended
        size_t Uncompress(const char* block, size_t block_size, string& result)
        {
            size_t result_size = *reinterpret_cast<uint32_t*>(const_cast<char*>(block));
            const char* lz4_data = block + sizeof(uint32_t);
            result.resize(result_size);
            char* buffer = &*result.begin();
            int32_t data_size = int32_t(block_size - sizeof(uint32_t));
            int32_t count = LZ4_decompress_safe(lz4_data, buffer, data_size, result_size);

            return size_t(count);
        }
    }

    void JsonFromChunk(const vector<char>& chunk, bool compressed, string& engine, string& json)
    {
        if (compressed)
        {
            if (engine == "LZ4")
            {
                lz4::Uncompress(chunk.data(), chunk.size(), json);
            }

            if (engine == "SNAPPY")
            {
                snappy::Uncompress(chunk.data(), chunk.size(), &json);
            }
        }
        else
        {
            json.resize(chunk.size());
            char* dest = &*json.begin();
            memcpy(dest, chunk.data(), chunk.size());
        }
    }

    Kernel::ISimulation* ReadDtkVersion2(FILE* f, const char* filename, Header& header)
    {
        Kernel::ISimulation* newsim = nullptr;

        // Read simulation JSON
        vector<char> chunk;
        ReadChunk(f, header.chunk_sizes[0], filename, chunk);

        string json;
        JsonFromChunk(chunk, header.compressed, header.compression, json);

        // Deserialize simulation
        Kernel::IArchive* reader = static_cast<Kernel::IArchive*>(new Kernel::JsonFullReader(json.c_str()));
        (*reader).labelElement("simulation") & newsim;
        delete reader;

        Kernel::Simulation* simulation = static_cast<Kernel::Simulation*>(newsim);

        // For each node
        for (size_t i = 1; i < header.chunk_count; ++i)
        {
            // Read node JSON
            ReadChunk(f, header.chunk_sizes[i], filename, chunk);
            JsonFromChunk(chunk, header.compressed, header.compression, json);

            // Deserialize node
            reader = static_cast<Kernel::IArchive*>(new Kernel::JsonFullReader(json.c_str()));
            Kernel::suids::suid suid;
            Kernel::ISerializable* obj;
            (*reader).labelElement("suid") & suid;
            (*reader).labelElement("node") & obj;

            delete reader;

            // Add node to simulation.nodes
            simulation->nodes[suid] = static_cast<Kernel::Node*>(obj);
        }

        return newsim;
    }

    Kernel::ISimulation* ReadDtkVersion34(FILE* f, const char* filename, Header& header)
    {
        Kernel::ISimulation* newsim = nullptr;

        // Read simulation JSON
        vector<char> chunk;
        ReadChunk(f, header.chunk_sizes[0], filename, chunk);

        string json;
        JsonFromChunk(chunk, header.compressed, header.compression, json);

        // Deserialize simulation
        Kernel::IArchive* reader = static_cast<Kernel::IArchive*>(new Kernel::JsonFullReader(json.c_str()));
        (*reader) & newsim;
        delete reader;

        Kernel::Simulation* simulation = static_cast<Kernel::Simulation*>(newsim);

        // For each node
        for (size_t i = 1; i < header.chunk_count; ++i)
        {
            // Read node JSON
            ReadChunk(f, header.chunk_sizes[i], filename, chunk);
            JsonFromChunk(chunk, header.compressed, header.compression, json);

            // Deserialize node
            reader = static_cast<Kernel::IArchive*>(new Kernel::JsonFullReader(json.c_str()));
            Kernel::ISerializable* obj;
            (*reader) & obj;

            delete reader;

            // Add node to simulation.nodes
            Kernel::Node* node = static_cast<Kernel::Node*>(obj);
            simulation->nodes[node->GetSuid()] = node;
        }

        return newsim;
    }
}   // namespace SerializedState
