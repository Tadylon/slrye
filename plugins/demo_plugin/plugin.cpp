#include <services/plugin/plugin_api.h>

#include <cstddef>

namespace
{

SlryePluginLogFn g_log = nullptr;
void* g_log_user_data = nullptr;

void log(const char* message)
{
    if (g_log != nullptr && message != nullptr)
    {
        g_log(message, g_log_user_data);
    }
}

}

extern "C" SLRYE_PLUGIN_EXPORT SlryePluginResult slrye_plugin_get_info(SlryePluginInfoV1* out_info)
{
    if (out_info == nullptr)
    {
        return SLRYE_PLUGIN_ERROR_INVALID_ARGUMENT;
    }

    out_info->struct_size = sizeof(SlryePluginInfoV1);
    out_info->api_version = SLRYE_PLUGIN_API_VERSION;
    out_info->id = "com.slrye.demo";
    out_info->name = "Demo Plugin";
    out_info->version = "0.1.0";
    out_info->author = "slrye";
    out_info->description = "Validates plugin discovery, initialization, logging, and shutdown.";
    out_info->capabilities = SLRYE_PLUGIN_CAPABILITY_VIEW | SLRYE_PLUGIN_CAPABILITY_COMMAND;
    return SLRYE_PLUGIN_OK;
}

extern "C" SLRYE_PLUGIN_EXPORT SlryePluginResult slrye_plugin_initialize(const SlryePluginHostApiV1* host_api)
{
    if (host_api == nullptr || host_api->struct_size < sizeof(SlryePluginHostApiV1))
    {
        return SLRYE_PLUGIN_ERROR_INVALID_ARGUMENT;
    }

    if (host_api->api_version > SLRYE_PLUGIN_API_VERSION)
    {
        return SLRYE_PLUGIN_ERROR_VERSION_MISMATCH;
    }

    g_log = host_api->log;
    g_log_user_data = host_api->user_data;
    log("[demo_plugin] initialized");
    return SLRYE_PLUGIN_OK;
}

extern "C" SLRYE_PLUGIN_EXPORT void slrye_plugin_shutdown()
{
    log("[demo_plugin] shutdown");
    g_log = nullptr;
    g_log_user_data = nullptr;
}

extern "C" SLRYE_PLUGIN_EXPORT SlryePluginCapabilities slrye_plugin_get_capabilities()
{
    return SLRYE_PLUGIN_CAPABILITY_VIEW | SLRYE_PLUGIN_CAPABILITY_COMMAND;
}
