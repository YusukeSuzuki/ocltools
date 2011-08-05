/*
 * OpenCL tools
 *
 * Copyright (C) 2011 Yusuke Suzuki 
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */
#if !APPLE
	#include <CL/cl.h>
#else
	#include <OpenCL/opencl.h>
#endif

#include "errors.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <getopt.h>

static const int gVersionMajor = 1;
static const int gVersionMinor = 0;

static void IfErrorThenExit(int error);
static void GetOpts(
	int argc, char* argv[],
	bool& verbose, bool& version, bool& help,
	std::string& outfile, std::vector<std::string>& infiles);
static void LoadSource(
	const std::string& infile, std::vector<char>& source);
static void BuildProgram(
	const std::vector< std::vector<char> >& source,
	std::vector<unsigned char>& binary);
static void SaveBinary(
	const std::string& outfile, const std::vector<unsigned char> binary);

int
main(int argc, char* argv[])
{
	using namespace std;
	using namespace OCLT;

	bool verbose = false;
	bool version = false;
	bool help = false;

	vector<string> infiles;
	string outfile;

	GetOpts(argc, argv,
		verbose, version, help,
		outfile, infiles);

	if(help)
	{
		cout << "usage: " << argv[0] << " [options] kernel.cl" << endl <<
			"  -o file      output file name" << endl <<
			"  -v --verbose print detail" << endl <<
			"  -h --help    print help" << endl <<
			"  -V --version print version information" << endl;
		exit(EXIT_SUCCESS);
	}

	if(infiles.empty())
	{
		cerr << "no input file" << endl;
		exit(EXIT_FAILURE);
	}

	vector< vector<char> > sources(infiles.size());
	vector< vector<char> >::iterator srcItr = sources.begin();

	for(vector<string>::const_iterator itr = infiles.begin();
		itr != infiles.end(); ++itr, ++srcItr)
	{
		vector<char> source;
		LoadSource(*itr, source);
		srcItr->swap(source);
	}

	vector<unsigned char> binary;
	BuildProgram(sources, binary);

	if( outfile.empty() ) outfile = "out.clx";

	SaveBinary(outfile, binary);

	return 0;
}

static void LoadSource(
	const std::string& infile, std::vector<char>& source)
{
	using namespace std;

	FILE* file = fopen(infile.c_str(), "rb");

	if(!file)
	{
		int errorNum = errno;
		cerr << strerror(errorNum) << ": " << infile << endl;
		exit(EXIT_FAILURE);
	}

	source.clear();
	
	while(!feof(file))
	{
		char buf[1024];

		size_t readBytes = fread(buf, 1, 1024, file);

		if(!readBytes)
		{
			break;
		}

		size_t preSize= source.size();
		source.resize(preSize + readBytes);
		memcpy(&source[preSize], buf, readBytes);
	}

	fclose(file);
}

static void BuildProgram(
	const std::vector< std::vector<char> >& sources,
	std::vector<unsigned char>& binary)
{
	using namespace std;

	vector<const char*> srcPtrs(sources.size());
	vector<size_t> srcSizes(sources.size());

	for(size_t i = 0; i < sources.size(); ++i)
	{
		srcPtrs[i] = &sources[i][0];
		srcSizes[i] = sources[i].size();
	}

	cl_platform_id platform_id;
	cl_uint num_platforms;
	IfErrorThenExit( clGetPlatformIDs(1, &platform_id, &num_platforms) );

	if(!num_platforms)
	{
		cerr << "no platform on system" << endl;
		exit(EXIT_FAILURE);
	}

	cl_uint num_devices = 0;
	IfErrorThenExit(
		clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_ALL, 0, NULL, &num_devices) );

	if(!num_devices)
	{
		cerr << "no device on system" << endl;
		exit(EXIT_FAILURE);
	}

	vector<cl_device_id> device_ids(num_devices);
	IfErrorThenExit(
		clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_ALL, num_devices, &device_ids[0], NULL) );

	cl_int errcode_ret;
	cl_context context = clCreateContext(
		NULL, device_ids.size(), &device_ids[0], NULL, NULL, &errcode_ret);
	IfErrorThenExit(errcode_ret);

	cl_program program = clCreateProgramWithSource(
			context, sources.size(), &srcPtrs[0], &srcSizes[0], &errcode_ret);
	IfErrorThenExit(errcode_ret);

	IfErrorThenExit( clBuildProgram(program, 0, NULL, NULL, NULL, NULL) );

	cl_uint num_program_devices;
	IfErrorThenExit(
		clGetProgramInfo(
			program, CL_PROGRAM_NUM_DEVICES, sizeof(num_program_devices),
			&num_program_devices, NULL) );

	if(!num_program_devices)
	{
		cerr << "no device specific program built" << endl;
		exit(EXIT_FAILURE);
	}

	vector<size_t> programSizes(num_program_devices);
	IfErrorThenExit(
		clGetProgramInfo(
			program, CL_PROGRAM_BINARY_SIZES,
			sizeof(size_t) * num_program_devices, &programSizes[0], NULL) );

	vector< vector<unsigned char> > binaries(num_program_devices);
	vector<unsigned char*> binaryPtrs(num_program_devices);

	for(size_t i = 0; i < num_program_devices; ++i)
	{
		binaries[i].resize(programSizes[i]);
		binaryPtrs[i] = &binaries[i][0];
	}

	unsigned char* dataPtr = &binary[0];
	IfErrorThenExit(
		clGetProgramInfo(program, CL_PROGRAM_BINARIES,
			sizeof(unsigned char*) * num_program_devices, &binaryPtrs[0], NULL) );
	binary = binaries[0];
}

static void SaveBinary(
	const std::string& outfile, const std::vector<unsigned char> binary)
{
	using namespace std;

	FILE* file = fopen(outfile.c_str(), "wb");

	if(!file)
	{
		int errorNum = errno;
		cerr << strerror(errorNum) << ": " << outfile << endl;
		exit(EXIT_FAILURE);
	}

	fwrite(&binary[0], 1, binary.size(), file);

	fclose(file);
}

static void IfErrorThenExit(int error)
{
	if(!error)
	{
		return;
	}

	std::map<int, std::string>::const_iterator errMsgPairItr =
		OCLT::ErrorMessageMap.find(error);

	if(errMsgPairItr != OCLT::ErrorMessageMap.end())
	{
		std::cerr << "error : " << errMsgPairItr->second << std::endl;
	}
	else
	{
		std::cerr << "error : unkown error" << std::endl;
	}

	exit(EXIT_FAILURE);
}

static void GetOpts(
	int argc, char* argv[],
	bool& verbose, bool& version, bool& help,
	std::string& outfile, std::vector<std::string>& infiles)
{
	verbose = false;
	version = false;
	help = false;

	for(;;)
	{
		static struct option long_options[] = {
			{"help", 0, 0, 'h'},
			{"verbose", 0, 0, 'v'},
			{"version", 0, 0, 'V'},
			{0,0,0,0}
		};

		int option_index = 0;
		int c = getopt_long(argc, argv, "hvVo:", long_options, &option_index);

		if(c == -1) break;

		switch(c)
		{
		case 'h':
			help = true;
			break;
		case 'v':
			verbose = true;
			break;
		case 'V':
			version = true;
			break;
		case 'o':
			outfile = optarg;
			break;
		default:
			break;
		}
	}

	while(optind < argc)
	{
		infiles.push_back( std::string(argv[optind++]) );
	}
}

