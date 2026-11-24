#pragma once

#include <memory>
#include <string>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <engine/resources/texture.h>
#include <engine/rhi/vulkan/vulkan_context.h>

namespace slrye::engine
{

/**
 * @brief 材质透明度混合模式枚举（遵循 glTF 2.0 规范）
 */
enum class AlphaMode
{
    Opaque, ///< 不透明物体（默认模式，直接深度写入与遮挡）
    Mask,   ///< 镂空裁剪模式（依据 alpha_cutoff 阈值进行 discard 测试，如树叶、铁丝网）
    Blend   ///< Alpha 半透明混合模式（依据基础色 alpha 开启 GPU 颜色混合）
};

/**
 * @brief 现代 PBR 物理材质资产（Physically Based Rendering Material）
 * 
 * 职责与层级关系：
 * 1. 位于 Engine Resources 资源层，遵循 glTF 2.0 金属-粗糙度（Metallic-Roughness）工作流规范；
 * 2. 集中管理材质基础色彩因数、金属度因数、粗糙度因数、自发光因数；
 * 3. 关联多通道 2D 纹理资产（BaseColor, Normal, Metallic-Roughness, Ambient Occlusion, Emissive）；
 * 4. 供 Mesh 子网格及 Scene / ECS 的 MaterialComponent 绑定使用；
 * 5. 提供默认材质快捷工厂 create_default()，自动初始化安全的基础颜色贴图。
 */
struct Material
{
    std::string name = "DefaultMaterial"; ///< 材质可读标识名称

    // --- PBR 材质核心因子 (Factors) ---
    glm::vec4 base_color_factor{1.0f, 1.0f, 1.0f, 1.0f}; ///< 基础表面反射色因子 (RGBA)
    float metallic_factor = 1.0f;                         ///< 金属度乘数 [0.0 = 非金属介质, 1.0 = 纯金属]
    float roughness_factor = 1.0f;                        ///< 粗糙度乘数 [0.0 = 镜面光滑, 1.0 = 完全漫反射]
    glm::vec3 emissive_factor{0.0f, 0.0f, 0.0f};          ///< 自发光辐射乘数 (RGB)

    // --- PBR 材质纹理贴图通道 (Textures) ---
    std::shared_ptr<Texture> base_color_texture = nullptr;         ///< 主漫反射 / 固有色贴图 (sRGB)
    std::shared_ptr<Texture> normal_texture = nullptr;             ///< 切线空间法线贴图 (Linear RGB)
    std::shared_ptr<Texture> metallic_roughness_texture = nullptr; ///< 金属粗糙度合成贴图 (Linear: B 通道为金属度, G 通道为粗糙度)
    std::shared_ptr<Texture> occlusion_texture = nullptr;          ///< 环境光遮蔽贴图 (Linear: R 通道为 AO)
    std::shared_ptr<Texture> emissive_texture = nullptr;           ///< 自发光贴图 (sRGB)

    // --- 渲染控制参数 ---
    AlphaMode alpha_mode = AlphaMode::Opaque; ///< 透明度模式
    float alpha_cutoff = 0.5f;                ///< Mask 模式下的 Alpha 剔除门限
    bool double_sided = false;                ///< 是否开启双面渲染（禁用面剔除）

    /**
     * @brief 构建默认的安全标准 PBR 材质
     * @param context 可选的 VulkanContext；若有效，将自动生成 1x1 纯白纹理绑定到 base_color_texture
     */
    [[nodiscard]] static std::shared_ptr<Material> create_default(const VulkanContext* context = nullptr)
    {
        auto mat = std::make_shared<Material>();
        mat->name = "DefaultMaterial";
        if (context != nullptr && context->is_valid())
        {
            mat->base_color_texture = Texture::create_color(*context, 0xFFFFFFFF);
        }
        return mat;
    }
};

}

namespace slrye
{
using engine::AlphaMode;
using engine::Material;
}
