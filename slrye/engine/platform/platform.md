# Engine Platform 模块 (`slrye/engine/platform`)

## 1. 模块定位与职责
`platform` 是引擎的跨平台操作系统抽象层（OS Abstraction Layer）。负责屏蔽 Windows、Linux、macOS 等底层操作系统在文件读写、原子持久化、路径字符编码转换（UTF-8 与宽字符）、特殊系统环境变量（如 `%LOCALAPPDATA%`）等方面的平台差异，为引擎和编辑器提供健壮、统一的 I/O 接口。

---

## 2. 文件清单与深度解析

### 2.1 `file_io.h` / `file_io.cpp`
* **定位**：安全文件输入输出组件。
* **主要内容**：
  * `read_text_file(path, contents, error)`：将指定路径的文本文件内容一次性读入 `std::string`。包含详细的错误原因诊断（若打开失败输出底层文件系统原因）。
  * `write_text_file_atomic(path, contents, error)`：**原子文件写入**。先将内容写入一个带随机后缀的临时文件（`.tmp`），写入并刷盘成功后再使用文件系统重命名（`filesystem::rename`）直接覆盖目标文件。有效防止程序异常崩溃导致工程文件或场景文件损坏半截。

### 2.2 `path_encoding.h` / `path_encoding.cpp`
* **定位**：跨平台路径字符集编码互转。
* **主要内容**：
  * `path_to_utf8(path)`：将 `std::filesystem::path` 安全无损转换为标准的 UTF-8 编码 `std::string`，供 YAML 序列化、UI 渲染展示与日志打印使用。
  * `path_from_utf8(value)`：将 UTF-8 编码的文本还原为适应宿主系统的 `std::filesystem::path`（在 Windows 上自动转换为 wchar_t Unicode 路径，在 POSIX 系统转换为原生路径）。

### 2.3 `paths.h` / `paths.cpp`
* **定位**：核心运行时目录自适应解析器。
* **主要内容**：
  * `resolve_executable_path(argc, argv)`：从启动参数或当前运行工作目录中精准解析出当前可执行文件的绝对物理路径。
  * `plugins_directory(executable_path)`：解析动态插件存放目录（默认位于可执行文件同级的 `plugins/` 目录）。
  * `assets_directory(executable_path)`：**带多层自适应回退的资产目录定位**：
    1. 优先探测 `executable.parent_path() / "assets"`（安装包/打包目录）；
    2. 次优探测 `executable.parent_path().parent_path() / "assets"`（应对 Visual Studio / CMake 多配置生成的 `build/bin/Debug/` 目录）；
    3. 兼容单配置输出与就近资产。
  * `user_config_directory()`：根据平台规范获取当前登录用户的应用专属持久化配置目录（在 Windows 下优先使用 `LOCALAPPDATA/Slrye`，失败回退到 `APPDATA/Slrye` 或用户主目录）。
