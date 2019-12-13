//===- SPIRVUtil.h - SPIR-V Utility Functions -------------------*- C++ -*-===//
//
//                     The LLVM/SPIRV Translator
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
// Copyright (c) 2014 Advanced Micro Devices, Inc. All rights reserved.
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
/// \file
///
/// This file defines SPIR-V utility functions.
///
//===----------------------------------------------------------------------===//

#pragma once

#include <ostream>
#define spv_ostream std::ostream

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace SPIRV
{
#define SPIRV_DEF_NAMEMAP(Type, MapType)                                       \
  typedef SPIRVMap<Type, std::string>(MapType);                                \
  inline MapType getNameMap(Type) {                                            \
    MapType MT;                                                                \
    return MT;                                                                 \
  }
	
	// A bi-way map
	template<class Ty1, class Ty2, class Identifier = void>
	struct SPIRVMap
	{
	public:
		using KeyTy = Ty1;
		using ValueTy = Ty2;
		// Initialize map entries
		void init();
		
		static Ty2 map(Ty1 Key)
		{
			Ty2 Val;
			bool Found = find(Key, &Val);
			(void)Found;
			assert(Found && "Invalid key");
			return Val;
		}
		
		static Ty1 rmap(Ty2 Key)
		{
			Ty1 Val;
			bool Found = rfind(Key, &Val);
			(void)Found;
			assert(Found && "Invalid key");
			return Val;
		}
		
		static const SPIRVMap& getMap()
		{
			static const SPIRVMap Map(false);
			return Map;
		}
		
		static const SPIRVMap& getRMap()
		{
			static const SPIRVMap Map(true);
			return Map;
		}
		
		static void foreach(std::function<void(Ty1, Ty2)> F)
		{
			for (auto& I : getMap().Map)
				F(I.first, I.second);
		}
		
		// For each key/value in the map executes function \p F.
		// If \p F returns false break the iteration.
		static void foreachConditional(std::function<bool(const Ty1&, Ty2)> F)
		{
			for (auto& I : getMap().Map)
			{
				if (!F(I.first, I.second))
					break;
			}
		}
		
		static bool find(Ty1 Key, Ty2* Val = nullptr)
		{
			const SPIRVMap& Map = getMap();
			typename MapTy::const_iterator Loc = Map.Map.find(Key);
			if (Loc == Map.Map.end())
				return false;
			if (Val)
				*Val = Loc->second;
			return true;
		}
		
		static bool rfind(Ty2 Key, Ty1* Val = nullptr)
		{
			const SPIRVMap& Map = getRMap();
			typename RevMapTy::const_iterator Loc = Map.RevMap.find(Key);
			if (Loc == Map.RevMap.end())
				return false;
			if (Val)
				*Val = Loc->second;
			return true;
		}
		
		SPIRVMap() : IsReverse(false)
		{
		}
		
	protected:
		explicit SPIRVMap(bool Reverse) : IsReverse(Reverse)
		{
			init();
		}
		
		using MapTy = std::map<Ty1, Ty2>;
		using RevMapTy = std::map<Ty2, Ty1>;
		
		void add(Ty1 V1, Ty2 V2)
		{
			if (IsReverse)
			{
				RevMap[V2] = V1;
				return;
			}
			Map[V1] = V2;
		}

		MapTy Map;
		RevMapTy RevMap;
		bool IsReverse;
	};
	
	inline std::vector<std::string> getVec(const std::string& S, char Delim)
	{
		std::vector<std::string> Strs;
		std::stringstream SS(S);
		std::string Item;
		while (std::getline(SS, Item, Delim))
		{
			Strs.push_back(Item);
		}
		return Strs;
	}
	
	inline std::set<std::string> getSet(const std::string& S, char Delim = ' ')
	{
		std::set<std::string> Strs;
		std::stringstream SS(S);
		std::string Item;
		while (std::getline(SS, Item, Delim))
		{
			Strs.insert(Item);
		}
		return Strs;
	}
	
	// Get the number of words used for encoding a string literal in SPIRV
	inline unsigned getSizeInWords(const std::string& Str)
	{
		assert(Str.length() / 4 + 1 <= std::numeric_limits<unsigned>::max());
		return static_cast<unsigned>(Str.length() / 4 + 1);
	}
	
	inline std::string getString(std::vector<uint32_t>::const_iterator Begin,
	                             std::vector<uint32_t>::const_iterator End)
	{
		auto Str = std::string();
		for (auto I = Begin; I != End; ++I)
		{
			const auto Word = *I;
			for (auto J = 0u; J < 32u; J += 8u)
			{
				const auto Char = static_cast<char>((Word >> J) & 0xff);
				if (Char == '\0')
				{
					return Str;
				}
				Str += Char;
			}
		}
		return Str;
	}
	
	inline std::string getString(const std::vector<uint32_t>& V)
	{
		return getString(V.cbegin(), V.cend());
	}

	// if vector of Literals is expected to contain more than one Literal String
	inline std::vector<std::string> getVecString(const std::vector<uint32_t>& V)
	{
		std::vector<std::string> Result;
		std::string Str;
		for (auto It = V.cbegin(); It < V.cend(); It += getSizeInWords(Str))
		{
			Str.clear();
			Str = getString(It, V.cend());
			Result.push_back(Str);
		}
		return Result;
	}
	
	inline std::vector<uint32_t> getVec(const std::string& Str)
	{
		std::vector<uint32_t> V;
		const auto StrSize = Str.size();
		auto CurrentWord = 0u;
		for (auto I = 0u; I < StrSize; ++I)
		{
			if (I % 4u == 0u && I != 0u)
			{
				V.push_back(CurrentWord);
				CurrentWord = 0u;
			}
			assert(Str[I] && "0 is not allowed in string");
			CurrentWord += static_cast<uint32_t>(Str[I]) << ((I % 4u) * 8u);
		}
		if (CurrentWord != 0u)
		{
			V.push_back(CurrentWord);
		}
		if (StrSize % 4 == 0)
		{
			V.push_back(0);
		}
		return V;
	}
	
	template<typename T>
	std::set<T> getSet(T Op1)
	{
		std::set<T> S;
		S.insert(Op1);
		return S;
	}
	
	template<typename T>
	std::vector<T> getVec(T Op1)
	{
		std::vector<T> V;
		V.push_back(Op1);
		return V;
	}
	
	template<typename T>
	inline std::vector<T> getVec(T Op1, T Op2)
	{
		std::vector<T> V;
		V.push_back(Op1);
		V.push_back(Op2);
		return V;
	}
	
	template<typename T>
	std::vector<T> getVec(T Op1, T Op2, T Op3)
	{
		std::vector<T> V;
		V.push_back(Op1);
		V.push_back(Op2);
		V.push_back(Op3);
		return V;
	}
	
	template<typename T>
	std::vector<T> getVec(T Op1, const std::vector<T>& Ops2)
	{
		std::vector<T> V;
		V.push_back(Op1);
		V.insert(V.end(), Ops2.begin(), Ops2.end());
		return V;
	}
}