#include "SpirvFunctions.h"

#include "Converter.h"

#include <glm/glm.hpp>

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
		{"_Matrix_4_4_F32_Col_Mult_Vector_4_F32", reinterpret_cast<FunctionPointer>(MultiplicationMV<glm::mat4x4, true, glm::vec4>)},
		{"_Matrix_4_4_F32_Col_Mult_Matrix_4_4_F32_Col", reinterpret_cast<FunctionPointer>(MultiplicationMM<glm::mat4x4, true, glm::mat4x4, true>)},
	};
	return functions;
}