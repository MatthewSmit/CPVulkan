#include "SpirvFunctions.h"

#include <glm/glm.hpp>

template<typename Vector>
static void Dot(typename Vector::value_type* result, const Vector* x, const Vector* y)
{
	*result = glm::dot(*x, *y);
}

template<typename Matrix, bool MatrixColumn, typename Scalar>
static void MultiplicationMS(Matrix* result, const Matrix* matrix, Scalar scalar)
{
	if (MatrixColumn)
	{
		*result = *matrix * scalar;
	}
	else
	{
		throw std::exception{};
	}
}

template<typename Vector, typename Matrix, bool MatrixColumn>
static void MultiplicationVM(typename Matrix::row_type* result, Vector* vector, const Matrix* matrix)
{
	if (MatrixColumn)
	{
		*result = *vector * *matrix;
	}
	else
	{
		throw std::exception{};
	}
}

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
		{"@Vector.Dot.F32.F32[2]", reinterpret_cast<FunctionPointer>(Dot<glm::fvec2>)},
		{"@Vector.Dot.F32.F32[3]", reinterpret_cast<FunctionPointer>(Dot<glm::fvec3>)},
		{"@Vector.Dot.F32.F32[4]", reinterpret_cast<FunctionPointer>(Dot<glm::fvec4>)},
		
		{"@Matrix.Mult.F32[2,2,col].F32[2,2,col].F32", reinterpret_cast<FunctionPointer>(MultiplicationMS<glm::mat2x2, true, float>)},
		{"@Matrix.Mult.F32[3,3,col].F32[3,3,col].F32", reinterpret_cast<FunctionPointer>(MultiplicationMS<glm::mat3x3, true, float>)},
		{"@Matrix.Mult.F32[4,4,col].F32[4,4,col].F32", reinterpret_cast<FunctionPointer>(MultiplicationMS<glm::mat4x4, true, float>)},
		
		{"@Matrix.Mult.F32[4].F32[4].F32[4,4,col]", reinterpret_cast<FunctionPointer>(MultiplicationVM<glm::vec4, glm::mat4x4, true>)},
		
		{"@Matrix.Mult.F32[3].F32[3,3,col].F32[3]", reinterpret_cast<FunctionPointer>(MultiplicationMV<glm::mat3x3, true, glm::vec3>)},
		{"@Matrix.Mult.F32[4].F32[4,4,col].F32[4]", reinterpret_cast<FunctionPointer>(MultiplicationMV<glm::mat4x4, true, glm::vec4>)},
		
		{"@Matrix.Mult.F32[4,4,col].F32[4,4,col].F32[4,4,col]", reinterpret_cast<FunctionPointer>(MultiplicationMM<glm::mat4x4, true, glm::mat4x4, true>)},
	};
	return functions;
}
