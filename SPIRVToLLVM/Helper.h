#pragma once

#if defined(_MSC_VER)
#define STL_DLL_EXPORT __declspec(dllexport)
#else
#define STL_DLL_EXPORT
#endif