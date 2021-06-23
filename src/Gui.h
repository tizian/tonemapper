#pragma once

#include <Global.h>
#include <nanogui/screen.h>

namespace tonemapper {

class Image;

class TonemapperGui : public nanogui::Screen {
public:
    TonemapperGui();
    virtual ~TonemapperGui();

    void setImage(const std::string &filename);

    bool keyboard_event(int key, int scancode, int action, int modifiers) override;
    bool mouse_motion_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override;
    bool scroll_event(const nanogui::Vector2i &p, const nanogui::Vector2f &rel) override;


    void draw_contents() override;

private:
    // Data
    Image *m_image = nullptr;

    int m_imageDisplayWidth,
        m_imageDisplayHeight,
        m_imageDisplayOffsetX,
        m_imageDisplayOffsetY;
    float  m_imageDisplayScale;

    // Gui
    nanogui::ref<nanogui::Window> m_window;

    nanogui::ref<nanogui::Button> m_saveButton;

    // Display
    nanogui::ref<nanogui::Shader>     m_shader;     // TODO: should live somewhere else
    nanogui::ref<nanogui::RenderPass> m_renderPass;
    nanogui::ref<nanogui::Texture>    m_texture;
};

} // Namespace tonemapper
