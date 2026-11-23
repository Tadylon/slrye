#pragma once


#if defined(_WIN32) && defined(SLRYE_SHARED_LIBRARY)
// 在编译SLRYE库本身的时候，通过__declspec(dllexport)，告诉编译器，把符号导出到DLL
#if defined(SLRYE_BUILDING_LIBRARY)
#define SLRYE_API __declspec(dllexport)
#else
// 其它项目使用SLRYE动态库的时候，__declspec(dllimport)告诉编译器，符号来自于DLL
#define SLRYE_API __declspec(dllimport)
#endif
// 通过__attribute__((visibility("default")))，确保符号对外可见
#elif defined(__GNUC__) && defined(SLRYE_SHARED_LIBRARY)
#define SLRYE_API __attribute__((visibility("default")))
#else
#define SLRYE_API
#endif