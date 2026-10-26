# Plugin System Decision

Status: Accepted

## Context

Slrye is a desktop game engine editor built with C++20, GLFW, Vulkan, and Dear ImGui. The editor should support independently distributed native extensions for views, commands, asset importers, and later render passes.

The host and plugins are currently built from the same repository, but the plugin boundary must remain stable enough that a plugin can outlive a host rebuild.

## Options Considered

### Option A: Native dynamic library with a C ABI and YAML manifest

- The host discovers directories under `plugins/`.
- Each directory contains `plugin.yaml` and a platform shared library.
- Plugins export C-linkage functions for information, initialization, shutdown, and capabilities.
- The host passes a versioned C API struct with a logging callback.

Why this is suitable:

- Stable across MSVC versions and different C++ compilers.
- Explicit plugin lifecycle and version checks.
- No shared C++ runtime or STL layout crossing the boundary.
- The manifest is human-readable and can later carry dependencies, permissions, and supported host versions.

Cost:

- More manual function pointer resolution.
- C ABI limits rich C++ extension interfaces until wrapper layers are added.

### Option B: C++ abstract interface shared library

Plugins link against host C++ interfaces and return `std::unique_ptr<IPlugin>`.

Why rejected as the primary boundary:

- Tightly couples compiler, standard library, and runtime settings.
- Requires plugins and host to use the same C++ toolchain.
- Debug/release and iterator-debug-level mismatches become fragile.

### Option C: Scripted plugin layer only

Use Lua, Python, or another embedded language for editor extensions.

Why rejected as the only mechanism:

- Good for UI scripting later, but insufficient for native asset importers, render extensions, and engine-level integrations.
- Adds an interpreter dependency before the native editor core is stable.

## Decision

Use Option A as the native plugin boundary. The first implementation exposes discovery, lifecycle, logging, capabilities, and a single sample plugin. C++ wrapper interfaces can be added later without changing the binary contract.

## Consequences

- `slrye_plugin_api.h` must remain backward compatible.
- New fields must be appended to versioned structs.
- Plugins must not allocate memory in the host and free it across the ABI boundary; future ownership APIs will use host-provided allocators or explicit release functions.
- The host currently accepts plugins whose API version is less than or equal to the host API version.
- Plugins are unloaded in reverse load order before Vulkan teardown.
