#pragma once

#include <cstdint>

#define SLRYE_PLUGIN_API_VERSION 1u
#define SLRYE_PLUGIN_ABI_VERSION 1u

#if defined(_WIN32)
    #if defined(SLRYE_PLUGIN_BUILD)
        #define SLRYE_PLUGIN_EXPORT __declspec(dllexport)
    #else
        #define SLRYE_PLUGIN_EXPORT __declspec(dllimport)
    #endif
#else
    #define SLRYE_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum SlryePluginResult
{
    SLRYE_PLUGIN_OK = 0,
    SLRYE_PLUGIN_ERROR_INVALID_ARGUMENT = 1,
    SLRYE_PLUGIN_ERROR_VERSION_MISMATCH = 2,
    SLRYE_PLUGIN_ERROR_LOAD_FAILED = 3,
    SLRYE_PLUGIN_ERROR_INITIALIZATION_FAILED = 4,
    SLRYE_PLUGIN_ERROR_NOT_FOUND = 5
} SlryePluginResult;

typedef uint32_t SlryePluginCapabilities;

enum SlryePluginCapabilityBits
{
    SLRYE_PLUGIN_CAPABILITY_NONE = 0,
    SLRYE_PLUGIN_CAPABILITY_MENU = 1u << 0,
    SLRYE_PLUGIN_CAPABILITY_VIEW = 1u << 1,
    SLRYE_PLUGIN_CAPABILITY_COMMAND = 1u << 2,
    SLRYE_PLUGIN_CAPABILITY_ASSET_IMPORTER = 1u << 3,
    SLRYE_PLUGIN_CAPABILITY_RENDER_PASS = 1u << 4
};

typedef void (*SlryePluginLogFn)(const char* message, void* user_data);

typedef struct SlryePluginHostApiV1
{
    uint32_t struct_size;
    uint32_t api_version;
    SlryePluginLogFn log;
    void* user_data;
} SlryePluginHostApiV1;

typedef struct SlryePluginInfoV1
{
    uint32_t struct_size;
    uint32_t api_version;
    const char* id;
    const char* name;
    const char* version;
    const char* author;
    const char* description;
    SlryePluginCapabilities capabilities;
} SlryePluginInfoV1;

typedef SlryePluginResult (*SlryePluginGetInfoFn)(SlryePluginInfoV1* out_info);
typedef SlryePluginResult (*SlryePluginInitializeFn)(const SlryePluginHostApiV1* host_api);
typedef void (*SlryePluginShutdownFn)(void);
typedef SlryePluginCapabilities (*SlryePluginGetCapabilitiesFn)(void);

#ifdef __cplusplus
}
#endif
