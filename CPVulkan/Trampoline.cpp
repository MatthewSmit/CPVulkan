#include "Trampoline.h"

// ReSharper disable CppUnusedIncludeDirective

#include "CommandBuffer.h"
#include "Device.h"
#include "Instance.h"
#include "PhysicalDevice.h"
#include "Queue.h"

#define CONCAT_IMPL(x, y) x##y
#define MACRO_CONCAT(x, y) CONCAT_IMPL(x, y)

#ifdef _MSC_VER
#   define GET_ARG_COUNT(_, ...)  INTERNAL_EXPAND_ARGS_PRIVATE(INTERNAL_ARGS_AUGMENTER(__VA_ARGS__))

#   define INTERNAL_ARGS_AUGMENTER(...) unused, __VA_ARGS__
#   define INTERNAL_EXPAND(x) x
#   define INTERNAL_EXPAND_ARGS_PRIVATE(...) INTERNAL_EXPAND(INTERNAL_GET_ARG_COUNT_PRIVATE(__VA_ARGS__, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#   define INTERNAL_GET_ARG_COUNT_PRIVATE(_1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, _17_, _18_, _19_, _20_, _21_, _22_, _23_, _24_, _25_, _26_, _27_, _28_, _29_, _30_, _31_, _32_, _33_, _34_, _35_, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, count, ...) count
#else
#   define GET_ARG_COUNT(...) INTERNAL_GET_ARG_COUNT_PRIVATE(__VA_ARGS__, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#   define INTERNAL_GET_ARG_COUNT_PRIVATE(_0, _1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, _17_, _18_, _19_, _20_, _21_, _22_, _23_, _24_, _25_, _26_, _27_, _28_, _29_, _30_, _31_, _32_, _33_, _34_, _35_, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, count, ...) count
#endif

#define GET_NAMES0
#define GET_NAMES1 , a
#define GET_NAMES2 , a, b
#define GET_NAMES3 , a, b, c
#define GET_NAMES4 , a, b, c, d
#define GET_NAMES5 , a, b, c, d, e
#define GET_NAMES6 , a, b, c, d, e, f
#define GET_NAMES7 , a, b, c, d, e, f, g
#define GET_NAMES8 , a, b, c, d, e, f, g, h
#define GET_NAMES9 , a, b, c, d, e, f, g, h, i
#define GET_NAMES10 , a, b, c, d, e, f, g, h, i, j
#define GET_NAMES11 , a, b, c, d, e, f, g, h, i, j, k
#define GET_NAMES12 , a, b, c, d, e, f, g, h, i, j, k, l
#define GET_NAMES13 , a, b, c, d, e, f, g, h, i, j, k, l, m
#define GET_NAMES14 , a, b, c, d, e, f, g, h, i, j, k, l, m, n
#define GET_NAMES(...) MACRO_CONCAT(GET_NAMES, GET_ARG_COUNT(__VA_ARGS__))

#define GET_ARGS0(_)
#define GET_ARGS1(_, T0) , T0 a
#define GET_ARGS2(_, T0, T1) , T0 a, T1 b
#define GET_ARGS3(_, T0, T1, T2) , T0 a, T1 b, T2 c
#define GET_ARGS4(_, T0, T1, T2, T3) , T0 a, T1 b, T2 c, T3 d
#define GET_ARGS5(_, T0, T1, T2, T3, T4) , T0 a, T1 b, T2 c, T3 d, T4 e
#define GET_ARGS6(_, T0, T1, T2, T3, T4, T5) , T0 a, T1 b, T2 c, T3 d, T4 e, T5 f
#define GET_ARGS7(_, T0, T1, T2, T3, T4, T5, T6) , T0 a, T1 b, T2 c, T3 d, T4 e, T5 f, T6 g
#define GET_ARGS8(_, T0, T1, T2, T3, T4, T5, T6, T7) , T0 a, T1 b, T2 c, T3 d, T4 e, T5 f, T6 g, T7 h
#define GET_ARGS9(_, T0, T1, T2, T3, T4, T5, T6, T7, T8) , T0 a, T1 b, T2 c, T3 d, T4 e, T5 f, T6 g, T7 h, T8 i
#define GET_ARGS10(_, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9) , T0 a, T1 b, T2 c, T3 d, T4 e, T5 f, T6 g, T7 h, T8 i, T9 j
#define GET_ARGS11(_, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10) , T0 a, T1 b, T2 c, T3 d, T4 e, T5 f, T6 g, T7 h, T8 i, T9 j, T10 k
#define GET_ARGS12(_, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11) , T0 a, T1 b, T2 c, T3 d, T4 e, T5 f, T6 g, T7 h, T8 i, T9 j, T10 k, T11 l
#define GET_ARGS13(_, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12) , T0 a, T1 b, T2 c, T3 d, T4 e, T5 f, T6 g, T7 h, T8 i, T9 j, T10 k, T11 l, T12 m
#define GET_ARGS14(_, T0, T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13) , T0 a, T1 b, T2 c, T3 d, T4 e, T5 f, T6 g, T7 h, T8 i, T9 j, T10 k, T11 l, T12 m, T13 n
#define GET_ARGS(...) MACRO_CONCAT(GET_ARGS, GET_ARG_COUNT(__VA_ARGS__))(__VA_ARGS__)

#define VULKAN_FUNCTION(method, returnType, clazz, ...) VKAPI_ATTR returnType VKAPI_PTR __##clazz##_##method(void* ptr GET_ARGS(__VA_ARGS__)) noexcept\
	{\
		typedef returnType (VKAPI_PTR* FunctionArg)(__VA_ARGS__);\
		const auto clazzMethod = &clazz::method;\
		const auto redirect = *(FunctionArg*)&clazzMethod;\
		return redirect(static_cast<uint8_t*>(ptr) + 16 GET_NAMES(__VA_ARGS__));\
	}
#include "VulkanFunctions.h"
#undef VULKAN_FUNCTION