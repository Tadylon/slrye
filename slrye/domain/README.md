# Domain

这里放不依赖 ImGui、GLFW、Vulkan 的业务模型。

当前包含：

- `Scene`：EnTT registry 的拥有者，也是实体创建、销毁和层级关系的唯一入口。
- `Entity`：基于稳定 `EntityId` 的轻量句柄，失效后不会被 EnTT 句柄复用误判为有效。
- `components.h`：Name、Transform、Hierarchy、Camera、Light 和 Collider 数据组件。
- `entity_id.h`：稳定实体标识。

后续的 CommandStack、序列化模型和运行时系统继续放在这一层。
