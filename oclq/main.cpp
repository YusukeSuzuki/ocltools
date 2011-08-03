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
#include "errors.hpp"
#include "names.hpp"

#if !APPLE
	#include <CL/cl.h>
#else
	#include <OpenCL/opencl.h>
#endif

#include <iostream>
#include <string>
#include <vector>
#include <getopt.h>

static const int gVersionMajor = 1;
static const int gVersionMinor = 0;

static void IfErrorThenExit(int error);

static std::string GetPlatformInfo(cl_platform_id id, cl_platform_info info);

static std::string GetDeviceType(cl_device_id device_id);
static std::vector<size_t> GetDeviceMaxWorkItemSizes(
	cl_device_id device_id, cl_uint max_work_item_dimensions);

template<typename T>
static T GetDeviceInfo(cl_device_id device_id, cl_device_info info);
static std::string GetDeviceInfo(cl_device_id device_id, cl_device_info info);

static void PrintPlatform(cl_platform_id platform_id, bool verbose);
static void PrintDevice(cl_platform_id platform_id, cl_device_id device_id, bool verbose);

static void GetOpts(
	int argc, char* argv[], bool& verbose, bool& version, bool& help);

int
main(int argc, char* argv[])
{
	using namespace std;
	using namespace OCLT;

	bool verbose = false;
	bool version = false;
	bool help = false;

	GetOpts(argc, argv, verbose, version, help);

	if(help)
	{
		cout << "usage: " << argv[0] << " [options]" << endl <<
			"  -v --verbose print detail" << endl <<
			"  -h --help    print help" << endl <<
			"  -V --version print version information" << endl;
		exit(EXIT_SUCCESS);
	}

	if(version)
	{
		cout << "oclq version " << gVersionMajor << "." << gVersionMinor << endl;
		exit(EXIT_SUCCESS);
	}

	cl_uint num_platforms;

	IfErrorThenExit( clGetPlatformIDs(0, NULL, &num_platforms) );

	if(!num_platforms)
	{
		cerr << "there is no OpenCL platform" << endl;
		exit(EXIT_FAILURE);
	}

	cl_platform_id* platforms = new cl_platform_id[num_platforms];

	IfErrorThenExit( clGetPlatformIDs(num_platforms, platforms, &num_platforms) );

	for(cl_uint i = 0; i < num_platforms; ++i)
	{
		PrintPlatform(platforms[i], verbose);

		cl_uint device_num = 0;

		IfErrorThenExit( clGetDeviceIDs(
			platforms[i], CL_DEVICE_TYPE_ALL, device_num, NULL, &device_num) );

		if(!device_num)
		{
			continue;
		}

		cl_device_id* devices = new cl_device_id[device_num];

		IfErrorThenExit( clGetDeviceIDs(
			platforms[i], CL_DEVICE_TYPE_ALL, device_num, devices, &device_num) );

		for(size_t j = 0; j < device_num; ++j)
		{
			PrintDevice(platforms[i], devices[j], verbose);
		}

		delete[] devices;
	}

	delete[] platforms;

	return 0;
}

static void PrintPlatform(cl_platform_id platform_id, bool verbose)
{
	using namespace std;

	cout << "---- platform" << endl;
	cout << "ID: " << platform_id << endl;

	cout << "CL_PLATFORM_PROFILE: " <<
		GetPlatformInfo(platform_id, CL_PLATFORM_PROFILE) << endl;

	cout << "CL_PLATFORM_VERSION: " <<
		GetPlatformInfo(platform_id, CL_PLATFORM_VERSION) << endl;

	cout << "CL_PLATFORM_NAME: " <<
		GetPlatformInfo(platform_id, CL_PLATFORM_NAME) << endl;

	cout << "CL_PLATFORM_VENDOR: " <<
		GetPlatformInfo(platform_id, CL_PLATFORM_VENDOR) << endl;


	cout << "CL_PLATFORM_EXTENSIONS: " <<
		GetPlatformInfo(platform_id, CL_PLATFORM_EXTENSIONS) << endl;
}

static void PrintDevice(cl_platform_id platform_id, cl_device_id device_id, bool verbose)
{
	using namespace std;

	cout << "-- device" << endl;

	cout << "ID: " << device_id << endl;
	cout << "CL_DEVICE_TYPE: " << GetDeviceType(device_id) << endl;
	cout << "CL_DEVICE_VENDOR_ID: " <<
		"0x" << hex << GetDeviceInfo<cl_uint>(device_id, CL_DEVICE_VENDOR_ID) << dec << endl;
	cout << "CL_DEVICE_MAX_COMPUTE_UNITS: " <<
		GetDeviceInfo<cl_uint>(device_id, CL_DEVICE_MAX_COMPUTE_UNITS) << endl;
	
	cout << "CL_DEVICE_PLATFORM: " << "0x" << hex <<
		GetDeviceInfo<cl_platform_id>(device_id, CL_DEVICE_PLATFORM) << dec << endl;

	cout << "CL_DEVICE_NAME: " <<
		GetDeviceInfo(device_id, CL_DEVICE_NAME) << endl;

	cout << "CL_DEVICE_VENDOR: " <<
		GetDeviceInfo(device_id, CL_DEVICE_VENDOR) << endl;

	cout << "CL_DEVICE_VERSION: " <<
		GetDeviceInfo(device_id, CL_DEVICE_VERSION) << endl;

	cout << "CL_DEVICE_PROFILE: " <<
		GetDeviceInfo(device_id, CL_DEVICE_PROFILE) << endl;

	cout << "CL_DEVICE_OPENCL_C_VERSION: " <<
		GetDeviceInfo(device_id, CL_DEVICE_OPENCL_C_VERSION) << endl;

	cout << "CL_DRIVER_VERSION: " <<
		GetDeviceInfo(device_id, CL_DRIVER_VERSION) << endl;

	cout << "CL_DEVICE_EXTENSIONS: " <<
		GetDeviceInfo(device_id, CL_DEVICE_EXTENSIONS) << endl;

	if(!verbose) return;

	cl_uint max_work_item_dimensions = 0;
	cout << "CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS: " <<
		( max_work_item_dimensions =
			GetDeviceInfo<cl_uint>(device_id, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS) ) <<
			endl;

	std::vector<size_t> work_item_sizes =
		GetDeviceMaxWorkItemSizes(device_id, max_work_item_dimensions);

	cout << "CL_DEVICE_MAX_WORK_ITEM_SIZES:";

	for(vector<size_t>::const_iterator itr = work_item_sizes.begin();
		itr != work_item_sizes.end(); ++itr) cout << " " << (*itr);

	cout << endl;

	cout << "CL_DEVICE_MAX_WORK_GROUP_SIZE: " <<
		GetDeviceInfo<size_t>(device_id, CL_DEVICE_MAX_WORK_GROUP_SIZE) << endl;

	cout << "CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR: " <<
		GetDeviceInfo<cl_uint>(device_id, CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR) << endl;
	cout << "CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT: " <<
		GetDeviceInfo<cl_uint>(device_id, CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT) << endl;
	cout << "CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT: " <<
		GetDeviceInfo<cl_uint>(device_id, CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT) << endl;
	cout << "CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG: " <<
		GetDeviceInfo<cl_uint>(device_id, CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG) << endl;
	cout << "CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT: " <<
		GetDeviceInfo<cl_uint>(device_id, CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT) << endl;
	cout << "CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE: " <<
		GetDeviceInfo<cl_uint>(device_id, CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE) << endl;
	cout << "CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF: " <<
		GetDeviceInfo<cl_uint>(device_id, CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF) << endl;

	cout << "CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR: " <<
		GetDeviceInfo<cl_uint>(device_id, CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR) << endl;
	cout << "CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT: " <<
		GetDeviceInfo<cl_uint>(device_id, CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT) << endl;
	cout << "CL_DEVICE_NATIVE_VECTOR_WIDTH_INT: " <<
		GetDeviceInfo<cl_uint>(device_id, CL_DEVICE_NATIVE_VECTOR_WIDTH_INT) << endl;
	cout << "CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG: " <<
		GetDeviceInfo<cl_uint>(device_id, CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG) << endl;
	cout << "CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT: " <<
		GetDeviceInfo<cl_uint>(device_id, CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT) << endl;
	cout << "CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE: " <<
		GetDeviceInfo<cl_uint>(device_id, CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE) << endl;
	cout << "CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF: " <<
		GetDeviceInfo<cl_uint>(device_id, CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF) << endl;

	cout << "CL_DEVICE_MAX_CLOCK_FREQUENCY: " <<
		GetDeviceInfo<cl_uint>(device_id, CL_DEVICE_MAX_CLOCK_FREQUENCY) << endl;
	cout << "CL_DEVICE_ADDRESS_BITS: " <<
		GetDeviceInfo<cl_uint>(device_id, CL_DEVICE_ADDRESS_BITS) << endl;
	cout << "CL_DEVICE_MAX_MEM_ALLOC_SIZE: " <<
		GetDeviceInfo<cl_ulong>(device_id, CL_DEVICE_MAX_MEM_ALLOC_SIZE) << endl;
	cout << "CL_DEVICE_IMAGE_SUPPORT: " <<
		GetDeviceInfo<cl_bool>(device_id, CL_DEVICE_IMAGE_SUPPORT) << endl;
	cout << "CL_DEVICE_MAX_READ_IMAGE_ARGS: " <<
		GetDeviceInfo<cl_uint>(device_id, CL_DEVICE_MAX_READ_IMAGE_ARGS) << endl;
	cout << "CL_DEVICE_IMAGE2D_MAX_WIDTH: " <<
		GetDeviceInfo<size_t>(device_id, CL_DEVICE_IMAGE2D_MAX_WIDTH) << endl;
	cout << "CL_DEVICE_IMAGE2D_MAX_HEIGHT: " <<
		GetDeviceInfo<size_t>(device_id, CL_DEVICE_IMAGE2D_MAX_HEIGHT) << endl;
	cout << "CL_DEVICE_IMAGE3D_MAX_WIDTH: " <<
		GetDeviceInfo<size_t>(device_id, CL_DEVICE_IMAGE3D_MAX_WIDTH) << endl;
	cout << "CL_DEVICE_IMAGE3D_MAX_HEIGHT: " <<
		GetDeviceInfo<size_t>(device_id, CL_DEVICE_IMAGE3D_MAX_HEIGHT) << endl;
	cout << "CL_DEVICE_IMAGE3D_MAX_DEPTH: " <<
		GetDeviceInfo<size_t>(device_id, CL_DEVICE_IMAGE3D_MAX_DEPTH) << endl;
	cout << "CL_DEVICE_MAX_SAMPLERS: " <<
		GetDeviceInfo<cl_uint>(device_id, CL_DEVICE_MAX_SAMPLERS) << endl;
	cout << "CL_DEVICE_MAX_PARAMETER_SIZE: " <<
		GetDeviceInfo<size_t>(device_id, CL_DEVICE_MAX_PARAMETER_SIZE) << endl;
	cout << "CL_DEVICE_MEM_BASE_ADDR_ALIGN: " <<
		GetDeviceInfo<cl_uint>(device_id, CL_DEVICE_MEM_BASE_ADDR_ALIGN) << endl;
	cout << "CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE: " <<
		GetDeviceInfo<cl_uint>(device_id, CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE) << endl;

	cl_device_fp_config fp_config = GetDeviceInfo<cl_device_fp_config>(
		device_id, CL_DEVICE_SINGLE_FP_CONFIG);

	cout << "CL_DEVICE_SINGLE_FP_CONFIG:" <<
		(fp_config & CL_FP_DENORM ? "CL_DEVICE_ CL_FP_DENORM" : "") <<
		(fp_config & CL_FP_INF_NAN ? "CL_DEVICE_ CL_FP_INF_NAN" : "") <<
		(fp_config & CL_FP_ROUND_TO_NEAREST? "CL_DEVICE_ CL_FP_ROUND_TO_NEAREST" : "") <<
		(fp_config & CL_FP_ROUND_TO_ZERO? "CL_DEVICE_ CL_FP_ROUND_TO_ZERO" : "") <<
		(fp_config & CL_FP_ROUND_TO_INF? "CL_DEVICE_ CL_FP_ROUND_TO_INF" : "") <<
		(fp_config & CL_FP_FMA ? "CL_DEVICE_ CL_FP_FMA" : "") <<
		(fp_config & CL_FP_SOFT_FLOAT ? "CL_DEVICE_ CL_FP_SOFT_FLOAT" : "") << endl;
	
	cl_device_mem_cache_type mem_cache_type = GetDeviceInfo<cl_device_mem_cache_type>(
		device_id, CL_DEVICE_GLOBAL_MEM_CACHE_TYPE);
	
	cout << "CL_DEVICE_GLOBAL_MEM_CACHE_TYPE: " <<
		(mem_cache_type == CL_NONE ? "CL_DEVICE_CL_NONE" :
			mem_cache_type == CL_READ_ONLY_CACHE ? "CL_DEVICE_CL_READ_ONLY_CACHE" :
			mem_cache_type == CL_READ_WRITE_CACHE ? "CL_DEVICE_CL_READ_WRITE_CACHE" : "unknown") << endl;

	cout << "CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE: " <<
		GetDeviceInfo<cl_uint>(device_id, CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE) << endl;
	cout << "CL_DEVICE_GLOBAL_MEM_CACHE_SIZE: " <<
		GetDeviceInfo<cl_ulong>(device_id, CL_DEVICE_GLOBAL_MEM_CACHE_SIZE) << endl;
	cout << "CL_DEVICE_GLOBAL_MEM_SIZE: " <<
		GetDeviceInfo<cl_ulong>(device_id, CL_DEVICE_GLOBAL_MEM_SIZE) << endl;

	cout << "CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE: " <<
		GetDeviceInfo<cl_ulong>(device_id, CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE) << endl;
	cout << "CL_DEVICE_MAX_CONSTANT_ARGS: " <<
		GetDeviceInfo<cl_uint>(device_id, CL_DEVICE_MAX_CONSTANT_ARGS) << endl;

	cl_device_local_mem_type local_mem_type = GetDeviceInfo<cl_device_local_mem_type>(
		device_id, CL_DEVICE_LOCAL_MEM_TYPE);
	
	cout << "CL_DEVICE_LOCAL_MEM_TYPE: " <<
		(local_mem_type == CL_LOCAL ? "CL_DEVICE_CL_LOCAL" :
			local_mem_type == CL_GLOBAL? "CL_DEVICE_CL_GLOBAL" : "unknown") << endl;

	cout << "CL_DEVICE_LOCAL_MEM_SIZE: " <<
		GetDeviceInfo<cl_ulong>(device_id, CL_DEVICE_LOCAL_MEM_SIZE) << endl;

	cout << "CL_DEVICE_ERROR_CORRECTION_SUPPORT: " <<
		GetDeviceInfo<cl_bool>(device_id, CL_DEVICE_ERROR_CORRECTION_SUPPORT) << endl;
	cout << "CL_DEVICE_HOST_UNIFIED_MEMORY: " <<
		GetDeviceInfo<cl_bool>(device_id, CL_DEVICE_HOST_UNIFIED_MEMORY) << endl;
	cout << "CL_DEVICE_PROFILING_TIMER_RESOLUTION: " <<
		GetDeviceInfo<size_t>(device_id, CL_DEVICE_PROFILING_TIMER_RESOLUTION) << endl;
	cout << "CL_DEVICE_ENDIAN_LITTLE: " <<
		GetDeviceInfo<cl_bool>(device_id, CL_DEVICE_ENDIAN_LITTLE) << endl;
	cout << "CL_DEVICE_AVAILABLE: " <<
		GetDeviceInfo<cl_bool>(device_id, CL_DEVICE_AVAILABLE) << endl;
	cout << "CL_DEVICE_COMPILER_AVAILABLE: " <<
		GetDeviceInfo<cl_bool>(device_id, CL_DEVICE_COMPILER_AVAILABLE) << endl;

	cl_device_exec_capabilities exec_capabilities =
		GetDeviceInfo<cl_device_exec_capabilities>(
			device_id, CL_DEVICE_EXECUTION_CAPABILITIES);
	
	cout << "CL_DEVICE_EXECUTION_CAPABILITIES:" <<
		(exec_capabilities & CL_EXEC_KERNEL ? "CL_DEVICE_ CL_EXEC_KERNEL" : "") <<
		(exec_capabilities & CL_EXEC_NATIVE_KERNEL ? "CL_DEVICE_ CL_EXEC_NATIVE_KERNEL" : "") << endl;
	
	cl_command_queue_properties queue_properties =
		GetDeviceInfo<cl_command_queue_properties>(device_id, CL_DEVICE_QUEUE_PROPERTIES);

	cout << "CL_DEVICE_QUEUE_PROPERTIES:" <<
		(queue_properties & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE ?
			"CL_DEVICE_ CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE" : "") <<
		(queue_properties & CL_QUEUE_PROFILING_ENABLE ?
			"CL_DEVICE_ CL_QUEUE_PROFILING_ENABLE" : "") << endl;
}

static std::string GetPlatformInfo(
	cl_platform_id platform_id, cl_platform_info platform_info)
{
	size_t size = 0;
	cl_int ret = clGetPlatformInfo(
		platform_id, platform_info, 0, NULL, &size);

	if(!size)
	{
		return std::string();
	}

	IfErrorThenExit(ret);

	char* str = new char[size + 1];
	str[0] = '\0';

	ret = clGetPlatformInfo(
		platform_id, platform_info, size, str, &size);

	std::string result(str);

	delete[] str;

	return result;
}

static std::string GetDeviceType(cl_device_id device_id)
{
	cl_device_type device_type;
	size_t ret_size;
	IfErrorThenExit(
		clGetDeviceInfo(device_id, CL_DEVICE_TYPE,
		sizeof(device_type), &device_type, &ret_size) );

	std::map<cl_device_type, std::string>::const_iterator itr =
		OCLT::DeviceTypeNameMap.find(device_type);

	return itr == OCLT::DeviceTypeNameMap.end() ? std::string("unknown") : itr->second;
}

static std::vector<size_t> GetDeviceMaxWorkItemSizes(
	cl_device_id device_id, cl_uint max_work_item_dimensions)
{
	if(max_work_item_dimensions == 0)
	{
		return std::vector<size_t>();
	}

	std::vector<size_t> result(max_work_item_dimensions);

	IfErrorThenExit(
		clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_ITEM_SIZES,
		sizeof(size_t) * max_work_item_dimensions, &result[0], NULL) );
	return result;
}

template<typename T>
static T GetDeviceInfo(cl_device_id device_id, cl_device_info info)
{
	T ret;
	size_t ret_size;
	IfErrorThenExit(
		clGetDeviceInfo(device_id, info, sizeof(T), &ret, &ret_size) );
	return ret;
}

static std::string GetDeviceInfo(cl_device_id device_id, cl_device_info info)
{
	size_t size = 0;
	cl_int ret = clGetDeviceInfo(
		device_id, info, 0, NULL, &size);

	if(!size)
	{
		return std::string();
	}

	IfErrorThenExit(ret);

	char* str = new char[size + 1];
	str[0] = '\0';

	ret = clGetDeviceInfo(
		device_id, info, size, str, &size);

	std::string result(str);

	delete[] str;

	return result;
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
	int argc, char* argv[], bool& verbose, bool& version, bool& help)
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
		int c = getopt_long(argc, argv, "hvV", long_options, &option_index);

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
		default:
			break;
		}
	}
}


