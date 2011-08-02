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
#include "names.hpp"

namespace OCLT
{

#define iMAP_ELEM(X) std::map<cl_device_type, std::string>::value_type(X , #X)

static const std::map<cl_device_type, std::string>::value_type iDeviceTypeNames[] =
{
	iMAP_ELEM(CL_DEVICE_TYPE_CPU),
	iMAP_ELEM(CL_DEVICE_TYPE_GPU),
	iMAP_ELEM(CL_DEVICE_TYPE_ACCELERATOR),
	iMAP_ELEM(CL_DEVICE_TYPE_ALL),
	iMAP_ELEM(CL_DEVICE_TYPE_DEFAULT),
};

static const size_t iDeviceTypeMapSize =
	sizeof(iDeviceTypeNames) / sizeof(iDeviceTypeNames[0]);
const std::map<cl_device_type, std::string> DeviceTypeNameMap(
	iDeviceTypeNames, iDeviceTypeNames + iDeviceTypeMapSize);

}


