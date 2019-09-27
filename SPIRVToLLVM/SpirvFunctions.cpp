#include "SpirvFunctions.h"

#include <glm/glm.hpp>

template<typename Matrix, bool MatrixColumn, typename Vector>
static void Multiplication(typename Matrix::col_type* result, const Matrix* matrix, Vector* vector)
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
static void Multiplication(LeftMatrix* result, const LeftMatrix* left, RightMatrix* right)
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

std::unordered_map<std::string, void*>& getSpirvFunctions()
{
	static std::unordered_map<std::string, void*> functions
	{
		{"_Matrix_4_4_F32_Col_Mult_Vector_4_F32", Multiplication<glm::mat4x4, true, glm::vec4>},
		{"_Matrix_4_4_F32_Col_Mult_Matrix_4_4_F32_Col", Multiplication<glm::mat4x4, true, glm::mat4x4, true>},
	};
	return functions;
}