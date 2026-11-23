# Engine Core 模块 (`slrye/engine/core`)

## 1. 模块定位与职责
`core` 是整个引擎的最底层基础模块。它不依赖任何上层业务逻辑、场景或渲染管线，专门提供引擎通用的系统基础设施支持，包括高性能结构化日志系统、断言检查与基础宏定义，为整个引擎（Engine）和编辑器（Editor）提供统一的基础运行时服务。

---

## 2. 文件清单与深度解析

### 2.1 `log.h`
* **定位**：核心日志系统头文件，基于现代高效的 `spdlog` 库进行封装。
* **主要内容**：
  * **双通道设计 (`Log` 类)**：
    * `core_logger()`：引擎底层内核专用日志通道（标记为 `[SLRYE]`），供 RHI、渲染管线、场景管理、物理与文件系统使用。
    * `client_logger()`：上层客户端/编辑器/游戏逻辑专用日志通道（标记为 `[APP]`），供应用层、编辑器面板与脚本调用。
  * **快捷诊断宏定义**：
    * 内核级：`SLRYE_CORE_TRACE`, `SLRYE_CORE_INFO`, `SLRYE_CORE_WARN`, `SLRYE_CORE_ERROR`
    * 客户端级：`SLRYE_TRACE`, `SLRYE_INFO`, `SLRYE_WARN`, `SLRYE_ERROR`
  * **零拷贝内联访问**：通过 `noexcept` 内联函数直接返回 `std::shared_ptr<spdlog::logger>&`，避免无谓的原子引用计数开销。

### 2.2 `log.cpp`
* **定位**：日志系统实现文件。
* **主要内容**：
  * `Log::init()`：初始化标准输出与控制台颜色汇聚器（`spdlog::sinks::stdout_color_sink_mt`），配置自定义高可读性格式串（包含时间戳、线程号、日志器名与颜色等级）：
    ```text
    [%T] [%n] [%^%l%$] %v
    ```
  * 静态成员变量的实例化与生命周期持有。

---

## 3. 设计考量与协作关系
* **无侵入性**：仅依赖第三方经过工业验证的高速日志库 `spdlog`，对外暴露最符合 C++ 开发者直觉的变参宏接口。
* **向下基础**：后续如果增加全局内存分配统计器（`MemoryTracker`）、性能剖析器（`Profiler / Tracy`）或断言机制（`Assert`），均属于本 `core` 模块范畴。
