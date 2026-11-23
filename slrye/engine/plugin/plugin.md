# Engine Plugin 模块 (`slrye/engine/plugin`)

## 1. 模块定位与职责
`plugin` 是引擎的模块化可扩展插件子系统。支持以动态链接库（Windows `.dll`、Linux `.so`）的形式，在不重新编译引擎主干代码的情况下，为引擎和编辑器动态注入新的功能特性（如自定义菜单、附加视图、扩展命令、专属资产导入器或自定义渲染通道）。

---

## 2. 文件清单与深度解析

### 2.1 `plugin_api.h`
* **定位**：插件系统的标准 C 语言兼容 ABI（应用二进制接口）契约头文件。
* **主要内容**：
  * **版本控制宏**：定义 `SLRYE_PLUGIN_API_VERSION` 与 `SLRYE_PLUGIN_ABI_VERSION`，用于防止插件与宿主程序版本不一致导致内存布局混乱；
  * **平台导出修饰符**：提供跨平台的 `SLRYE_PLUGIN_EXPORT`（Windows 下为 `__declspec(dllexport)`，POSIX 下为 `visibility("default")`）；
  * **错误状态码 (`SlryePluginResult`)**：定义成功、参数错误、版本不匹配、加载失败、初始化失败等标准返回码；
  * **能力位图 (`SlryePluginCapabilityBits`)**：标记插件所支持的功能类别，例如：
    * `SLRYE_PLUGIN_CAPABILITY_MENU`：扩展编辑器菜单项；
    * `SLRYE_PLUGIN_CAPABILITY_VIEW`：注入自定义渲染/预览视图；
    * `SLRYE_PLUGIN_CAPABILITY_COMMAND`：注册自定义控制台/快捷命令；
    * `SLRYE_PLUGIN_CAPABILITY_ASSET_IMPORTER`：提供特殊文件资产的导入器；
    * `SLRYE_PLUGIN_CAPABILITY_RENDER_PASS`：提供额外渲染管线 Pass。
  * **宿主通信句柄 (`SlryePluginHostApiV1`)**：宿主向插件提供的回调函数指针集（如宿主安全日志打印等）。

### 2.2 `plugin_manifest.h` / `plugin_manifest.cpp`
* **定位**：插件描述清单（Manifest）解析器。
* **主要内容**：
  * `PluginManifest` 结构体：记录插件唯一标识符 `id`、名称 `name`、版本 `version`、作者 `author`、描述 `description`、目标动态库路径 `library` 及启用状态 `enabled`；
  * `load_plugin_manifest(path, error)`：解析插件根目录下的描述清单配置文件（如 `plugin.yaml` / `manifest.json`），将元数据反序列化为结构体。

### 2.3 `plugin_manager.h` / `plugin_manager.cpp`
* **定位**：插件全生命周期管理器（采用 PIMPL 模式设计）。
* **主要内容**：
  * `LoadedPlugin`：记录已成功加载到内存中的插件实体（含其运行时动态库句柄、实际具备的 Capabilities 以及物理目录）；
  * `PluginManager` 单例：
    * `load_all(plugins_root)`：扫描指定根目录下的所有子文件夹，批量发现、校验并加载合法插件；
    * `load_plugin(plugin_directory)`：加载单个插件，执行操作系统动态链接库载入（`LoadLibraryA` / `dlopen`），校验版本，获取入口点函数并完成握手；
    * `shutdown()`：统一卸载所有已加载插件，调用插件的析构清理钩子，最后释放动态库句柄（`FreeLibrary` / `dlclose`）；
    * `set_logger(...)`：为插件子系统配置诊断输出回调。
