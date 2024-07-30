
#pragma once

#include <map>
#include <vector>
#include <string>

#include "IdmApi.h"

namespace Kernel
{
    struct IJsonObjectAdapter;
}


struct IChannelDataMapOutputAugmentor
{
    // The implementor of this method is allowed to add data to the Header section of the output.
    virtual void AddDataToHeader( Kernel::IJsonObjectAdapter* pIJsonObj ) = 0;
};


// TODO: maybe optimize map lookup with something that reduces to an array reference? string comparisons or hash lookups can get out of hand <ERAD-202>
// TODO: explicit channel initialization? channels are added implicitly when Accumulate() is called. should allow better performance if they are preregistered

// ChannelID is to be used by reports to make access faster
// to particular channels.  It allows ChannelDataMap to use an
// index into a std::vector instead of a string into a map.
// The index makes getting the channel much faster than the map.
// One uses ChannelDataMap::AddChannel() to get ChannelID for a
// particular channel and can the use it later with
// ChannelDataMap::Accumulate().
// TODO: Replace other channel access via string to ChannelID.
class ChannelID
{
public:
    friend class ChannelDataMap;

    ChannelID();

    const std::string& GetName() const;

protected:
    ChannelID( int index, const std::string& channelName );

    int m_Index;
    std::string m_ChannelName;
};


// ChannelDataMap provides an object to manage vectors of floats or channels.
// Each channel is accessed via name and all channels within the map are managed
// to be the same length.
class ChannelDataMap
{
public:
    typedef float channel_data_element_t;
    typedef std::vector<channel_data_element_t> channel_data_t;

    ChannelDataMap();
    virtual ~ChannelDataMap();

    void ClearData();
    void IncreaseChannelLength( int numElements );
    void IncreaseChannelLength( const std::string& channel_name, int numElements );
    bool IsEmpty() const ;
    int GetNumChannels() const ;
    int GetChannelLength() const ;
    std::vector<std::string> GetChannelNames() const ;
    const channel_data_t& GetChannel( const std::string& channel_name );
    ChannelID AddChannel( const std::string& channel_name );
    void RemoveChannel( const std::string& channel_name );
    void SetChannelData( const std::string& channel_name, const channel_data_t& rChannelData );
    void SetLastValue( const std::string& channel_name, channel_data_element_t value );

    void Accumulate( const ChannelID& rID,                       channel_data_element_t value );
    void Accumulate( const std::string& channel_name,            channel_data_element_t value );
    void Accumulate( const std::string& channel_name, int index, channel_data_element_t value );

    void ExponentialValues( const std::string& channel_name );

    virtual bool HasChannel(std::string) const;

    // intentionally non-virtual; everyone that accumulates to channelDataMap can do this the same
    // user may call multiple times, each time the reduce will only apply to time steps accumulated since the last call
    void Reduce();

    virtual void WriteOutput( const std::string& filename, std::map<std::string, std::string>& units_map );

    void normalizeChannel( const std::string &channel_name, const std::string& normalization_channel );
    void normalizeChannel( const std::string &channel_name, float normalization_value );

    virtual void addDerivedLogScaleSummaryChannel(   const std::string& channel_name, const std::string& channel_log_name        );
    virtual void addDerivedCumulativeSummaryChannel( const std::string& channel_name, const std::string& channel_cumulative_name );

    void SetAugmentor( IChannelDataMapOutputAugmentor* pAugmentor ) { p_output_augmentor = pAugmentor; }

    void SetStartStopYears( float start, float stop );

    typedef std::map<  std::string, channel_data_t > channel_data_map_t;
    typedef std::pair< std::string, channel_data_t > channel_data_map_element_t;

    channel_data_map_t channel_data_map;
    std::vector<channel_data_t*> channel_index_to_map;

private:
    int timesteps_reduced; // number of time steps reduced so far
    IChannelDataMapOutputAugmentor* p_output_augmentor ;
    float start_year;
    float stop_year;
};
