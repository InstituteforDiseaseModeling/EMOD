function [nodeIDList chNames channelTimeSeries] = readDTKSpatialReports(varargin)


% options = struct('outputfiledir', 'C:\svn_TB\Regression\1001_TB_multinode_SEAsia_HLfit\output', 'channelnames', {{'Population', 'Prevalence', 'Disease_Deaths', 'Active_TB_Prevalence'}}, 'returnasstruct', false);
options = struct('outputfiledir', '\\diamonds-hn\EMOD\home\ghuynh\output\TB\simulations\2013_03_21_05_56_03_AM_56_3679\output', 'channelnames', {{'Population', 'Prevalence', 'Disease_Deaths', 'Active_TB_Prevalence', 'Latent_TB_Prevalence'}}, 'returnasstruct', false);

%read optional parameters
for pair = reshape(varargin,2,[])
    inpName = lower(pair{1}); %make case insensitive
    if any(strcmp(inpName, fieldnames(options)))
        options.(inpName) = pair{2};
    else   error('MATLAB:readDTKReportFiles', '%s is not a recognized parameter name.\n', inpName);
    end;
end;

%{
There is now a separate file for each channel for which spatial-reporting is enabled, 
called SpatialReport_<Channel_Name>.bin (e.g. SpatialReport_New_Infections.bin).
The format of each files is:

<4-byte int representing number of nodes in the file>
<4-byte int representing number of timesteps in the file>
<array of 4-byte ints representing node-ids in order they will appear in the file>
<array of 4-byte floats representing output-values for timestep 1; in same order as node-ids>
<array of 4-byte floats representing output-values for timestep 2; in same order as node-ids>
...
<array of 4-byte floats representing output-values for timestep n; in same order as node-ids>
%}

channelTimeSeries	= {};
chNames				= {};
nodeIDList			= [];

if(isempty(options.channelnames))
	
	return;
	if(~exist( options.outputfiledir, 'dir' ))
		return;
	end;
	
	% can detect the report files here
end;

chNames = options.channelnames;
if(~iscell(chNames))
	chNames = {chNames};
end;

channelTimeSeries = cell(1, length(chNames));
if(options.returnasstruct)
	channelTimeSeries = struct();
end;

for i = 1:length(chNames)
	
	file_path = fullfile( options.outputfiledir, sprintf('SpatialReport_%s.bin', chNames{i}) );
	
	[tmp_data  nodeIDList] = read_channel(file_path);
	
	if(options.returnasstruct)
		channelTimeSeries.(chNames{i}) = tmp_data;
	else
		channelTimeSeries{i} = tmp_data;
	end;	
end;
end


function [timeSpaceData nodeIDs] = read_channel(file_path)

fid	= fopen(file_path);

nNodes			= fread(fid, 1, 'int32=>int32');
nTimeSteps		= fread(fid, 1, 'int32=>int32');
nodeIDs			= fread(fid, nNodes, 'int32=>int32');

timeSpaceData	= fread(fid, [nNodes, nTimeSteps], 'float32=>float32'); % file is written by DTK in time-major order, so rows are nodes and columns are time steps. (all nodes are written out following each timestep)

fclose(fid);
end

