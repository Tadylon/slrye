#include <services/plugin/plugin_manager.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <yaml-cpp/yaml.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace slrye
{
namespace
{

#if defined(_WIN32)
constexpr const char* kPluginLibrarySuffix = ".dll";
#elif defined(__APPLE__)
constexpr const char* kPluginLibrarySuffix = ".dylib";
#else
constexpr const char* kPluginLibrarySuffix = ".so";
#endif

class NativeLibrary
{
public:
    NativeLibrary() = default;
    ~NativeLibrary()
    {
        close();
    }

    NativeLibrary(const NativeLibrary&) = delete;
    NativeLibrary& operator=(const NativeLibrary&) = delete;

    NativeLibrary(NativeLibrary&& other) noexcept
    {
        *this = std::move(other);
    }

    NativeLibrary& operator=(NativeLibrary&& other) noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        close();
        handle_ = other.handle_;
        other.handle_ = nullptr;
        return *this;
    }

    bool open(const std::filesystem::path& path)
    {
        close();

#if defined(_WIN32)
        handle_ = static_cast<void*>(LoadLibraryW(path.c_str()));
#else
        handle_ = dlopen(path.c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
        return handle_ != nullptr;
    }

    void close()
    {
        if (handle_ == nullptr)
        {
            return;
        }

#if defined(_WIN32)
        FreeLibrary(static_cast<HMODULE>(handle_));
#else
        dlclose(handle_);
#endif
        handle_ = nullptr;
    }

    [[nodiscard]] bool is_open() const noexcept
    {
        return handle_ != nullptr;
    }

    [[nodiscard]] void* symbol(const char* name) const noexcept
    {
        if (handle_ == nullptr || name == nullptr)
        {
            return nullptr;
        }

#if defined(_WIN32)
        return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle_), name));
#else
        return dlsym(handle_, name);
#endif
    }

private:
    void* handle_ = nullptr;
};

struct NativePlugin
{
    NativeLibrary library;
    SlryePluginGetInfoFn get_info = nullptr;
    SlryePluginInitializeFn initialize = nullptr;
    SlryePluginShutdownFn shutdown = nullptr;
    SlryePluginGetCapabilitiesFn get_capabilities = nullptr;
    LoadedPlugin metadata;
    bool initialized = false;
};

std::string read_yaml_string(const YAML::Node& node, const char* key)
{
    const YAML::Node value = node[key];
    if (!value)
    {
        return {};
    }

    return value.as<std::string>();
}

bool read_plugin_manifest(const std::filesystem::path& path, PluginManifest& manifest)
{
    try
    {
        const YAML::Node root = YAML::LoadFile(path.string());
        if (!root)
        {
            return false;
        }

        manifest.id = read_yaml_string(root, "id");
        manifest.name = read_yaml_string(root, "name");
        manifest.version = read_yaml_string(root, "version");
        manifest.author = read_yaml_string(root, "author");
        manifest.description = read_yaml_string(root, "description");
        manifest.library = read_yaml_string(root, "library");

        const YAML::Node enabled = root["enabled"];
        if (enabled)
        {
            manifest.enabled = enabled.as<bool>();
        }

        if (manifest.id.empty())
        {
            return false;
        }

        if (manifest.library.empty())
        {
            manifest.library = manifest.id;
        }

        return true;
    }
    catch (const YAML::Exception&)
    {
        return false;
    }
}

}

class PluginManager::Impl
{
public:
    explicit Impl(LogFn log)
        : log_(std::move(log))
    {
    }

    ~Impl()
    {
        shutdown();
    }

    void log(std::string message)
    {
        if (log_)
        {
            log_(std::move(message));
        }
    }

    static void plugin_host_log(const char* message, void* user_data)
    {
        if (message == nullptr || user_data == nullptr)
        {
            return;
        }

        auto* impl = static_cast<Impl*>(user_data);
        impl->log(message);
    }

    std::size_t load_all(const std::filesystem::path& plugins_root)
    {
        std::error_code error;
        if (!std::filesystem::exists(plugins_root, error))
        {
            log("Plugin root does not exist: " + plugins_root.string());
            return loaded_.size();
        }

        std::vector<std::filesystem::path> plugin_directories;
        for (std::filesystem::directory_iterator it(plugins_root, error), end; !error && it != end; it.increment(error))
        {
            if (it->is_directory(error))
            {
                plugin_directories.push_back(it->path());
            }
        }

        std::sort(plugin_directories.begin(), plugin_directories.end());
        for (const auto& directory : plugin_directories)
        {
            load_plugin(directory);
        }

        return loaded_.size();
    }

    bool load_plugin(const std::filesystem::path& plugin_directory)
    {
        const std::filesystem::path manifest_path = plugin_directory / "plugin.yaml";
        PluginManifest manifest;
        if (!read_plugin_manifest(manifest_path, manifest))
        {
            log("Ignoring plugin without a valid plugin.yaml: " + plugin_directory.string());
            return false;
        }

        if (!manifest.enabled)
        {
            log("Skipping disabled plugin: " + manifest.id);
            return false;
        }

        const auto duplicate = std::find_if(
            loaded_.begin(),
            loaded_.end(),
            [&manifest](const LoadedPlugin& loaded)
            {
                return loaded.manifest.id == manifest.id;
            });

        if (duplicate != loaded_.end())
        {
            log("Duplicate plugin id ignored: " + manifest.id);
            return false;
        }

        std::filesystem::path library_name = manifest.library;
        if (!library_name.has_extension())
        {
            library_name += kPluginLibrarySuffix;
        }

        auto native = std::make_unique<NativePlugin>();
        if (!native->library.open(plugin_directory / library_name))
        {
            log("Failed to load plugin library: " + (plugin_directory / library_name).string());
            return false;
        }

        native->get_info = reinterpret_cast<SlryePluginGetInfoFn>(native->library.symbol("slrye_plugin_get_info"));
        native->initialize = reinterpret_cast<SlryePluginInitializeFn>(native->library.symbol("slrye_plugin_initialize"));
        native->shutdown = reinterpret_cast<SlryePluginShutdownFn>(native->library.symbol("slrye_plugin_shutdown"));
        native->get_capabilities = reinterpret_cast<SlryePluginGetCapabilitiesFn>(native->library.symbol("slrye_plugin_get_capabilities"));

        if (native->get_info == nullptr || native->initialize == nullptr || native->shutdown == nullptr)
        {
            log("Plugin is missing required exports: " + manifest.id);
            return false;
        }

        SlryePluginInfoV1 info{};
        info.struct_size = sizeof(info);
        const SlryePluginResult info_result = native->get_info(&info);
        if (info_result != SLRYE_PLUGIN_OK)
        {
            log("Plugin rejected get_info: " + manifest.id);
            return false;
        }

        if (info.struct_size < sizeof(SlryePluginInfoV1) || info.api_version > SLRYE_PLUGIN_API_VERSION)
        {
            log("Plugin API version is not supported: " + manifest.id);
            return false;
        }

        SlryePluginHostApiV1 host_api{};
        host_api.struct_size = sizeof(host_api);
        host_api.api_version = SLRYE_PLUGIN_API_VERSION;
        host_api.log = &Impl::plugin_host_log;
        host_api.user_data = this;

        const SlryePluginResult initialize_result = native->initialize(&host_api);
        if (initialize_result != SLRYE_PLUGIN_OK)
        {
            log("Plugin initialization failed: " + manifest.id);
            return false;
        }

        native->initialized = true;
        native->metadata.manifest = manifest;
        native->metadata.directory = plugin_directory;
        native->metadata.library_path = plugin_directory / library_name;

        if (native->get_capabilities != nullptr)
        {
            native->metadata.capabilities = native->get_capabilities();
        }
        else
        {
            native->metadata.capabilities = info.capabilities;
        }

        loaded_.push_back(native->metadata);
        plugins_.push_back(std::move(native));
        log("Loaded plugin: " + manifest.id + " (" + manifest.name + ")");
        return true;
    }

    void shutdown()
    {
        for (auto it = plugins_.rbegin(); it != plugins_.rend(); ++it)
        {
            NativePlugin& plugin = **it;
            if (plugin.initialized && plugin.shutdown != nullptr)
            {
                plugin.shutdown();
                plugin.initialized = false;
            }
            plugin.library.close();
        }

        plugins_.clear();
        loaded_.clear();
    }

    [[nodiscard]] const std::vector<LoadedPlugin>& plugins() const noexcept
    {
        return loaded_;
    }

private:
    LogFn log_;
    std::vector<std::unique_ptr<NativePlugin>> plugins_;
    std::vector<LoadedPlugin> loaded_;
};

PluginManager::PluginManager(LogFn log)
    : impl_(std::make_unique<Impl>(std::move(log)))
{
}

PluginManager::~PluginManager() = default;

std::size_t PluginManager::load_all(const std::filesystem::path& plugins_root)
{
    return impl_->load_all(plugins_root);
}

bool PluginManager::load_plugin(const std::filesystem::path& plugin_directory)
{
    return impl_->load_plugin(plugin_directory);
}

void PluginManager::shutdown()
{
    impl_->shutdown();
}

const std::vector<LoadedPlugin>& PluginManager::plugins() const noexcept
{
    return impl_->plugins();
}

}
