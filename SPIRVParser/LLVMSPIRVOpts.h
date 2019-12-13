//===- LLVMSPIRVOpts.h - Specify options for translation --------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
// Copyright (c) 2019 Intel Corporation. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal with the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimers.
// Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimers in the documentation
// and/or other materials provided with the distribution.
// Neither the names of Advanced Micro Devices, Inc., nor the names of its
// contributors may be used to endorse or promote products derived from this
// Software without specific prior written permission.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH
// THE SOFTWARE.
//
//===----------------------------------------------------------------------===//
/// \file LLVMSPIRVOpts.h
///
/// This files declares helper classes to handle SPIR-V versions and extensions.
///
//===----------------------------------------------------------------------===//
#pragma once

#if defined(OS_WINDOWS)
#if defined(SPIRV_INTERNAL)
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT __declspec(dllimport)
#endif
#else
#define DLL_EXPORT
#endif

#include "spirv.hpp"

#include <cstdint>
#include <map>

namespace SPIRV
{
	enum class VersionNumber : uint32_t
	{
		// See section 2.3 of SPIR-V spec: Physical Layout of a SPIR_V Module and Instruction
		SPIRV_1_0 = 0x00010000,
		SPIRV_1_1 = 0x00010100,
		SPIRV_1_2 = 0x00010200,
		SPIRV_1_3 = 0x00010300,
		SPIRV_1_4 = 0x00010400,
		SPIRV_1_5 = 0x00010500,
		MinimumVersion = SPIRV_1_0,
		MaximumVersion = SPIRV_1_5
	};

	enum class ExtensionID : uint32_t
	{
		First,
#define EXT(X) X,
#include "LLVMSPIRVExtensions.inc"
#undef EXT
		Last,
	};

	/// \brief Helper class to manage SPIR-V translation
	class DLL_EXPORT TranslatorOptions
	{
	public:
		using ExtensionsStatusMap = std::map<ExtensionID, bool>;

		TranslatorOptions() = default;

		TranslatorOptions(VersionNumber maxVersion, spv::ExecutionModel executionModel, const ExtensionsStatusMap& extensionMap = {}) :
			maxVersion(maxVersion),
			executionModel(executionModel),
			extentionStatusMap(extensionMap)
		{
		}

		void EnableAllExtensions()
		{
#define EXT(X) extentionStatusMap[ExtensionID::X] = true;
#include "LLVMSPIRVExtensions.inc"
#undef EXT
		}

		[[nodiscard]] bool isAllowedToUseVersion(VersionNumber requestedVersion) const
		{
			return requestedVersion <= maxVersion;
		}

		[[nodiscard]] bool isAllowedToUseExtension(ExtensionID extension) const
		{
			const auto iterator = extentionStatusMap.find(extension);
			if (extentionStatusMap.end() == iterator)
			{
				return false;
			}

			return iterator->second;
		}

		[[nodiscard]] VersionNumber getMaxVersion() const { return maxVersion; }
		[[nodiscard]] spv::ExecutionModel getExecutionModel() const { return executionModel; }

	private:
		VersionNumber maxVersion = VersionNumber::MaximumVersion;
		spv::ExecutionModel executionModel;
		ExtensionsStatusMap extentionStatusMap;
	};
}