#pragma once

#include <Global.h>
#include <nanogui/screen.h>
#include <nanogui/graph.h>

namespace tonemapper {

class Image;
class TonemapOperator;

class TonemapperGui : public nanogui::Screen {
public:
    TonemapperGui();
    virtual ~TonemapperGui();

    void setImage(const std::string &filename);

    void setExposureMode(int index);
    void setTonemapOperator(int index);
    void refreshGraph();

    bool keyboard_event(int key, int scancode, int action, int modifiers) override;
    bool mouse_motion_event(const nanogui::Vector2i &p, const nanogui::Vector2i &rel, int button, int modifiers) override;
    bool scroll_event(const nanogui::Vector2i &p, const nanogui::Vector2f &rel) override;
    bool resize_event(const nanogui::Vector2i& size) override;

    void draw_contents() override;
    void draw(NVGcontext *ctx) override;

private:
    Image *m_image = nullptr;
    float  m_exposure = 1.f;

    int m_exposureModeIndex;
    int m_tonemapOperatorIndex;

    std::vector<TonemapOperator *> m_operators;

    // Gui
    int m_imageDisplayWidth,
        m_imageDisplayHeight,
        m_imageDisplayOffsetX,
        m_imageDisplayOffsetY;
    float m_imageDisplayScale;
    nanogui::Vector2i m_screenSize;

    nanogui::ref<nanogui::Window>      m_mainWindow;

    nanogui::ref<nanogui::Button>      m_saveButton;
    nanogui::ref<nanogui::Window>      m_saveWindow;
    nanogui::ref<nanogui::ProgressBar> m_saveProgressBar;
    float                              m_saveProgress = 0.f;
    std::thread                       *m_saveThread = nullptr;

    nanogui::ref<nanogui::Label>       m_exposureLabel;
    nanogui::ref<nanogui::PopupButton> m_exposurePopupButton;
    nanogui::ref<nanogui::Popup>       m_exposurePopup;
    nanogui::ref<nanogui::Widget>      m_exposureWidget;

    nanogui::ref<nanogui::Label>       m_tonemapLabel;
    nanogui::ref<nanogui::PopupButton> m_tonemapPopupButton;
    nanogui::ref<nanogui::Popup>       m_tonemapPopup;
    nanogui::ref<nanogui::Widget>      m_tonemapWidget;

    nanogui::ref<nanogui::Window>      m_graphWindow;
    nanogui::ref<nanogui::Graph>       m_graph;

    // Display
    nanogui::ref<nanogui::Shader>      m_shader;
    nanogui::ref<nanogui::RenderPass>  m_renderPass;
    nanogui::ref<nanogui::Texture>     m_texture;
};

} // Namespace tonemapper
