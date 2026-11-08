

```
flowchart TB
    MAIN[main] --> APP[Application 单例]

    subgraph Infrastructure[基础设施层 core]
        VK[VulkanBackend 单例]
        IMGUI[ImGuiBackend 单例]
        CTX[VulkanContext]
        VK --> IMGUI
        VK --> CTX
    end

    subgraph Editor[编辑器层 ui / views]
        UI[EditorUI 单例]
        THEME[ThemeManager]
        STATE[EditorViewState]
        VIEWPORT[Viewport]
        CAMCTRL[CameraController]
        CAMERA[PerspectiveCamera]
        RENDERER[SceneRenderer]

        UI *-- THEME
        UI *-- STATE
        UI *-- VIEWPORT
        VIEWPORT *-- CAMCTRL
        VIEWPORT *-- CAMERA
        VIEWPORT *-- RENDERER
    end

    subgraph Domain[领域层 domain]
        SCENE[Scene]
        ENTITY[Entity]
        ID[EntityId]
        COMPONENTS[Components / EntT Registry]
        SCENE *-- ENTITY
        SCENE *-- ID
        SCENE *-- COMPONENTS
        ENTITY --> SCENE
    end

    subgraph Services[服务层 services]
        PLUGIN[PluginManager 单例]
        SERIALIZER[SceneSerializer]
        PROJECT[ProjectFile]
        SETTINGS[EditorSettings]
        RECENT[RecentProjects]
    end

    APP --> VK
    APP --> UI
    APP --> PLUGIN
    APP *-- SCENE
    APP *-- SETTINGS
    APP *-- RECENT
    APP ..-> PROJECT
    APP ..-> SERIALIZER

    UI --> VK
    VIEWPORT --> SCENE
    RENDERER --> CTX
    RENDERER ..-> SCENE
    RENDERER ..-> CAMERA
```



