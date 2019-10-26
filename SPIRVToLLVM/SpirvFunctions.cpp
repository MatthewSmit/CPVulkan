#include "SpirvFunctions.h"

#include "Converter.h"

#include <glm/glm.hpp>

#define FATAL_ERROR() if (1) { __debugbreak(); abort(); } else (void)0

template<typename Matrix, bool MatrixColumn, typename Vector>
static void MultiplicationMV(typename Matrix::col_type* result, const Matrix* matrix, Vector* vector)
{
	if (MatrixColumn)
	{
		*result = *matrix * *vector;
	}
	else
	{
		throw std::exception{};
	}
}

template<typename LeftMatrix, bool LeftMatrixColumn, typename RightMatrix, bool RightMatrixColumn>
static void MultiplicationMM(LeftMatrix* result, const LeftMatrix* left, RightMatrix* right)
{
	if (LeftMatrixColumn && RightMatrixColumn)
	{
		*result = *left * *right;
	}
	else
	{
		throw std::exception{};
	}
}

std::unordered_map<std::string, FunctionPointer>& getSpirvFunctions()
{
	static std::unordered_map<std::string, FunctionPointer> functions
	{
		{"@Matrix.Mult.F32[3,3].F32[3].col", reinterpret_cast<FunctionPointer>(MultiplicationMV<glm::mat3x3, true, glm::vec3>)},
		{"@Matrix.Mult.F32[4,4].F32[4].col", reinterpret_cast<FunctionPointer>(MultiplicationMV<glm::mat4x4, true, glm::vec4>)},
		{"@Matrix.Mult.F32[4,4].F32[4,4].col", reinterpret_cast<FunctionPointer>(MultiplicationMM<glm::mat4x4, true, glm::mat4x4, true>)},
	};
	return functions;
}