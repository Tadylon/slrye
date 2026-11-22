# Slrye Architecture

```mermaid
flowchart TB
    MAIN[editor/app/main.cpp] --> APP[Application]

    subgraph EditorLayer[Editor 层: editor/]
        APP --> UI[EditorUI Shell]
        APP --> IMGUI_BK[ImGuiBackend]
        
        UI *-- THEME[ThemeManager / Styles]
        UI *-- PANELS[Editor Panels]
        
        subgraph PanelsGroup[panels/]
            VIEWPORT[Viewport Panel]
            HIERARCHY[SceneHierarchy Panel]
            INSPECTOR[Inspector Panel]
            CONSOLE[Console Panel]
            CAMCTRL[CameraController]
            VIEWPORT *-- CAMCTRL
        end
        
        PANELS --> VIEWPORT
        PANELS --> HIERARCHY
        PANELS --> INSPECTOR
        PANELS --> CONSOLE
    end

    subgraph EngineLayer[Engine 运行时层: engine/]
        subgraph RHIGroup[rhi/]
            VK[VulkanBackend]
            CTX[VulkanContext]
            VMA[VMA Allocator / vma_resources]
            VK --> CTX
            VK --> VMA
        end

        subgraph RendererGroup[renderer/]
            RENDERER[SceneRenderer]
            CAMERA[PerspectiveCamera]
            GEOMETRY[SceneGeometry]
            RENDERER --> GEOMETRY
            RENDERER --> CTX
            RENDERER --> VMA
        end

        subgraph SceneGroup[scene/]
            SCENE[Scene]
            ENTITY[Entity]
            ID[EntityId]
            COMPONENTS[Components / EnTT Registry]
            TRANSFORM[SceneTransform]
            PICKING[Scene Query / Picking]
            
            SCENE *-- COMPONENTS
            SCENE *-- ENTITY
            SCENE *-- TRANSFORM
            PICKING ..-> SCENE
        end

        subgraph ServicesGroup[services/]
            PROJECT[ProjectFile]
            RECENT[RecentProjects]
            SERIALIZER[SceneSerializer]
            SETTINGS_SVC[EditorSettings YAML]
        end

        subgraph CorePlatform[core / platform / plugin]
            LOG[Log / spdlog]
            PLATFORM[Paths / FileIO]
            PLUGIN[PluginManager]
        end
    end

    %% Editor 依赖 Engine，Engine 绝不依赖 Editor
    APP --> VK
    APP *-- SCENE
    APP ..-> PROJECT
    APP ..-> SERIALIZER
    
    VIEWPORT --> RENDERER
    VIEWPORT ..-> SCENE
    HIERARCHY ..-> SCENE
    INSPECTOR ..-> SCENE
    RENDERER ..-> SCENE
    RENDERER ..-> CAMERA
    IMGUI_BK --> CTX
```
