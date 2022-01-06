#pragma once

#include <Global.h>
#include <nanogui/screen.h>

namespace tonemapper {

class Image;
class TonemapOperator;
class RgbGraph;

class TonemapperGui : public nanogui::Screen {
public:
    TonemapperGui();
    virtual ~TonemapperGui();

    void setImage(const std::string &filename);
    void setTonemapOperator(const std::string &filename);

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
    nanogui::ref<nanogui::Label>       m_rfLabel;

    nanogui::ref<nanogui::Window>      m_graphWindow;
    nanogui::ref<RgbGraph>             m_graph;

    // Display
    nanogui::ref<nanogui::Shader>      m_shader;
    nanogui::ref<nanogui::RenderPass>  m_renderPass;
    nanogui::ref<nanogui::Texture>     m_texture;
    nanogui::ref<nanogui::Texture>     m_rfTextureR;
    nanogui::ref<nanogui::Texture>     m_rfTextureG;
    nanogui::ref<nanogui::Texture>     m_rfTextureB;
};

class RgbGraph : public nanogui::Widget {
public:
    RgbGraph(Widget *parent, const std::string &caption = "Untitled");

    const std::string &caption() const { return m_caption; }
    void set_caption(const std::string &caption) { m_caption = caption; }

    const std::string &header() const { return m_header; }
    void set_header(const std::string &header) { m_header = header; }

    const std::string &footer() const { return m_footer; }
    void set_footer(const std::string &footer) { m_footer = footer; }

    const std::vector<float> &valuesR() const { return m_values[0]; }
    std::vector<float> &valuesR() { return m_values[0]; }
    void set_valuesR(const std::vector<float> &values) { m_values[0] = values; }

    const std::vector<float> &valuesG() const { return m_values[1]; }
    std::vector<float> &valuesG() { return m_values[1]; }
    void set_valuesG(const std::vector<float> &values) { m_values[1] = values; }

    const std::vector<float> &valuesB() const { return m_values[2]; }
    std::vector<float> &valuesB() { return m_values[2]; }
    void set_valuesB(const std::vector<float> &values) { m_values[2] = values; }

    virtual nanogui::Vector2i preferred_size(NVGcontext *ctx) const override;
    virtual void draw(NVGcontext *ctx) override;
protected:
    std::string m_caption, m_header, m_footer;
    std::vector<float> m_values[3];
};

} // Namespace tonemapper
