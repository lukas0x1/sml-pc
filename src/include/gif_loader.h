#include <cstdint>
#include <vulkan/vulkan.h>
#include <imgui.h>

class GifLoader {
public:
    GifLoader();
    ~GifLoader();
    bool LoadGif(const char* filename);
    void RenderFrame(float x = 150, float y = 150);
private:
    void UpdateFrame();
    void CreateTextures(VkFormat format, VkExtent2D extent, uint32_t frameCount);
    void CreateImageView(VkImageView* imageView, VkFormat format);
    void AllocateMemory(VkDeviceMemory* memory, VkImageCreateInfo* imageInfo, VkMemoryPropertyFlags properties);
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    void DestroyTextures();

    VkImage m_Image;
    VkDeviceMemory m_ImageMemory;
    VkImageView m_ImageView;
    std::vector<ImTextureID> m_Frames;
    int m_CurrentFrame;
    int m_TotalFrames;
    float m_FrameDelay;
    float m_LastFrameTime;
};
