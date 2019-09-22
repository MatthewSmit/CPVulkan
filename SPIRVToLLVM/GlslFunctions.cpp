#include "GlslFunctions.h"

#include <glm/glm.hpp>

template<typename Matrix, typename Vector>
static void ColumnMultiplication(const Matrix* matrix, Vector* vector, typename Matrix::col_type* result)
{
	*result = *matrix * *vector;
}

const std::unordered_map<std::string, void*>& getGlslFunctions()
{
	static std::unordered_map<std::string, void*> functions
	{
		{"_Matrix_Vector_Mult_4_4_F32_Col", ColumnMultiplication<glm::mat4x4, glm::vec4>},
		// {"_Matrix_Vector_Mult_4_4_F32_Col", _Matrix_Vector_Mult_4_4_F32_Col},
	};
	return functions;
}