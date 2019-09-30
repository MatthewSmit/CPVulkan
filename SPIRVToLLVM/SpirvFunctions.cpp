#include "SpirvFunctions.h"

#include "Converter.h"

#include <glm/glm.hpp>

template<typename Matrix, typename Vector>
static void ColumnMultiplication(typename Matrix::col_type* result, const Matrix* matrix, Vector* vector)
{
	*result = *matrix * *vector;
}

std::unordered_map<std::string, FunctionPointer>& getSpirvFunctions()
{
    static std::unordered_map<std::string, FunctionPointer> functions
    {
        {"_Matrix_Vector_Mult_4_4_F32_Col", reinterpret_cast<FunctionPointer>(ColumnMultiplication<glm::mat4x4, glm::vec4>)},
    };
	return functions;
}