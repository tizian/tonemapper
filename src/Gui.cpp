#include <Gui.h>

#include <Image.h>
#include <Tonemap.h>

#include <nanogui/button.h>
#include <nanogui/icons.h>
#include <nanogui/label.h>
#include <nanogui/layout.h>
#include <nanogui/messagedialog.h>
#include <nanogui/opengl.h>
#include <nanogui/progressbar.h>
#include <nanogui/popupbutton.h>
#include <nanogui/renderpass.h>
#include <nanogui/shader.h>
#include <nanogui/slider.h>
#include <nanogui/textbox.h>
#include <nanogui/window.h>
#include <nanogui/vscrollpanel.h>

#include <thread>

namespace tonemapper {

using namespace nanogui;

TonemapperGui::TonemapperGui()
    : nanogui::Screen(Vector2i(1280, 720), "", true, false) {
    m_exposureModeIndex = 0;
    m_tonemapOperatorIndex = 0;

    // Instantiate all operators
    std::vector<std::string> operatorNames;
    for (auto const& kv: *TonemapOperator::constructors) {
        operatorNames.push_back(kv.first);
    }
    std::sort(operatorNames.begin(), operatorNames.end());

    m_operators = std::vector<TonemapOperator *>(operatorNames.size());
    for (size_t i = 0; i < operatorNames.size(); ++i) {
        m_operators[i] = TonemapOperator::create(operatorNames[i]);
    }

    // Setup display
    m_renderPass = new RenderPass({ this });
    m_renderPass->set_cull_mode(RenderPass::CullMode::Disabled);
    m_renderPass->set_clear_color(0, Color(50, 50, 60, 255));

    glfwSetWindowPos(glfw_window(), 50, 100);

    // Setup GUI
    auto ctx = nvg_context();

    auto layout = new GroupLayout();
    layout->set_spacing(10);
    layout->set_group_spacing(20);
    layout->set_group_indent(14);

    m_window = new Window(this, "Options");
    m_window->set_position(Vector2i(25, 15));
    m_window->set_fixed_width(280);
    m_window->set_layout(layout);

    auto about = new Button(m_window->button_panel(), "", FA_INFO);
    about->set_callback([&, ctx] {
        std::ostringstream oss;
        oss << "Tone Mapper v" << VERSION << std::endl
            << std::endl
            << "Copyright (c) " << YEAR << " Tizian Zeltner" << std::endl
            << std::endl
            << "More information can be found under" << std::endl
            << "http://github.com/tizian/tonemapper" << std::endl;

        auto dlg = new MessageDialog(this, MessageDialog::Type::Information, "About", oss.str());
        dlg->message_label()->set_fixed_width(380);
        dlg->message_label()->set_font_size(20);
        perform_layout(ctx);
        dlg->center();
    });

    new Label(m_window, "Image I/O", "sans-bold");

    auto *openButton = new Button(m_window, "Open HDR image", FA_FOLDER_OPEN);
    openButton->set_background_color(Color(0, 255, 0, 25));
    openButton->set_tooltip("Open HDR image (.exr or .hdr)");
    openButton->set_callback([&] {
        std::string filename = file_dialog({ {"exr", "OpenEXR"}, {"hdr", "Radiance RGBE"} }, false);
        if (filename != "") {
            PRINT_("Read image \"%s\" ..", filename);
            setImage(filename);
            PRINT(" done.");
        }
    });

    m_saveButton = new Button(m_window, "Save LDR image", FA_SAVE);
    m_saveButton->set_background_color(Color(0, 0, 255, 25));
    m_saveButton->set_tooltip("Save LDR image (.png or .jpg)");
    m_saveButton->set_callback([&] {
        std::string filename = file_dialog({ {"jpg", "JPEG"}, {"png", "Portable Network Graphics"} }, true);
        if (m_image && filename != "") {
            m_saveWindow = new Window(this, "Saving tonemapped image ..");
            m_saveWindow->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Middle, 10, 10));
            m_saveWindow->set_modal(true);
            m_saveWindow->set_fixed_width(300);
            m_saveWindow->set_visible(true);

            auto panel = new Widget(m_saveWindow);
            panel->set_layout(new BoxLayout(Orientation::Horizontal, Alignment::Middle, 10, 15));

            m_saveWindow->center();
            m_saveWindow->request_focus();
            m_saveProgressBar = new ProgressBar(panel);
            m_saveProgressBar->set_fixed_width(200);

            m_saveThread = new std::thread([&, filename]{
                PRINT_("Save image \"%s\" ..", filename);
                Image *out = new Image(m_image->getWidth(), m_image->getHeight());
                m_saveProgress = 0.f;
                m_operators[m_tonemapOperatorIndex]->process(m_image, out, m_exposure, &m_saveProgress);
                out->save(filename);
                m_saveProgress = -1.f;
                delete out;
                PRINT(" done.");
            });
        }
    });
    m_saveButton->set_enabled(false);

    setExposureMode(m_exposureModeIndex);

    perform_layout(ctx);

    draw_all();
    set_visible(true);

    std::ostringstream oss;
    oss << "Tone Mapper v" << VERSION;
    std::string title = oss.str();
    set_caption(title);
}

TonemapperGui::~TonemapperGui() {}

void TonemapperGui::setImage(const std::string &filename) {
    if (m_image) {
        delete m_image;
        m_image = nullptr;
    }

    m_image = Image::load(filename);

    if (m_image) {
        m_saveButton->set_enabled(true);
    }

    const int DEFAULT_WIDTH = 1280;
    m_imageDisplayWidth  = DEFAULT_WIDTH;
    m_imageDisplayHeight = DEFAULT_WIDTH * m_image->getHeight() / m_image->getWidth();
    set_size(Vector2i(m_imageDisplayWidth, m_imageDisplayHeight));

    m_imageDisplayScale = 0.f;
    m_imageDisplayOffsetX = 0;
    m_imageDisplayOffsetY = 0;

    m_texture = new nanogui::Texture(
        nanogui::Texture::PixelFormat::RGB,
        nanogui::Texture::ComponentFormat::Float32,
        Vector2i(m_image->getWidth(), m_image->getHeight()),
        nanogui::Texture::InterpolationMode::Nearest,
        nanogui::Texture::InterpolationMode::Nearest);

    m_shader->set_texture("source", m_texture);

    setExposureMode(m_exposureModeIndex);
}

void TonemapperGui::setExposureMode(int index) {
    m_exposureModeIndex = index;

    if (m_exposureLabel) {
        m_window->remove_child(m_exposureLabel);
    }

    m_exposureLabel = new Label(m_window, "Exposure mode", "sans-bold");

    if (m_exposurePopupButton) {
        m_window->remove_child(m_exposurePopupButton);
    }

    std::vector<std::string> exposureNames{"Manual", "Key Value", "Auto"};
    std::vector<std::string> exposureDescriptions{
        "Manual Mode\n\nScale the input image with a factor of 2^Exposure.",
        "Key Value mode\n\nScale the input image with a key value as described in \"Photographic Tone Reproduction for Digital Images\" by Reinhard et al. 2002.",
        "Auto mode\n\nAuto adjust the input image exposure as proposed in \"Perceptual Effects in Real-time Tone Mapping\" by Krawczyk et al. 2005."
    };

    m_exposurePopupButton = new PopupButton(m_window);
    m_exposurePopup = m_exposurePopupButton->popup();
    m_exposurePopup->set_width(130);
    m_exposurePopup->set_height(130);

    auto tmp = new Widget(m_exposurePopup);
    tmp->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Fill));
    auto popopPanel = new Widget(tmp);
    popopPanel->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Fill, 10, 10));

    for (int i = 0; i < 3; ++i) {
        auto button = new Button(popopPanel, exposureNames[i]);
        button->set_tooltip(exposureDescriptions[i]);
        button->set_flags(Button::RadioButton);
        button->set_callback([&, i] {
            m_exposurePopup->set_visible(false);
            setExposureMode(i);
        });
    }
    m_exposurePopupButton->set_caption(exposureNames[index]);
    m_exposurePopupButton->set_tooltip(exposureDescriptions[index]);

    if (m_exposureWidget) {
        m_window->remove_child(m_exposureWidget);
    }

    m_exposureWidget = new Widget(m_window);
    m_exposureWidget->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Minimum, 0, 10));

    auto *panel = new Widget(m_exposureWidget);
    panel->set_layout(new BoxLayout(Orientation::Horizontal, Alignment::Middle, 0, 0));

    Button *button = nullptr;
    Slider *slider = nullptr;
    FloatBox<float> *textBox = nullptr;

    if (index == 0 || index == 1) {
        button = new Button(panel, "alpha");
        button->set_fixed_size(Vector2i(50, 22));
        button->set_font_size(15);

        if (index == 0) {
            button->set_tooltip("Exponential scale factor 2^alpha");
        }
        else if (index == 1) {
            button->set_tooltip("Key value exposure adjustment parameter");
        }

        slider = new Slider(panel);
        slider->set_fixed_size(Vector2i(140, 22));

        textBox = new FloatBox<float>(panel);
        textBox->set_fixed_size(Vector2i(50, 22));
        textBox->number_format("%.2f");
        textBox->set_font_size(15);
        textBox->set_alignment(TextBox::Alignment::Right);
        textBox->set_editable(true);
    }

    if (index == 0) {
        m_exposure = 1.f;
        slider->set_value(0.5f);
        textBox->set_value(0.f);

        textBox->set_callback([&, slider, textBox](float v) {
            textBox->set_value(v);
            m_exposure = std::pow(2.f, v);
            slider->set_value(v / 20.f + 0.5f);
        });

        slider->set_callback([&, textBox](float t) {
            float tmp = (t - 0.5f) * 20.f;
            m_exposure = std::pow(2.f, tmp);
            textBox->set_value(tmp);
        });

        button->set_callback([&, slider, textBox] {
            m_exposure = 1.f;
            slider->set_value(0.5f);
            textBox->set_value(0.f);
        });
    } else if (index == 1) {
        /* See Eq. (1) in "Photographic Tone Reproduction for Digital Images"
           by Reinhard et al. 2002. */
        slider->set_value(0.18f);
        textBox->set_value(0.18f);
        if (m_image) {
            m_exposure = 0.18f / m_image->getLogMeanLuminance();
        }

        textBox->set_callback([&, slider, textBox](float v) {
            textBox->set_value(v);
            m_exposure = v / m_image->getLogMeanLuminance();
            slider->set_value(v);
        });

        slider->set_callback([&, textBox](float t) {
            if (m_image) {
                m_exposure = t / m_image->getLogMeanLuminance();
            }
            textBox->set_value(t);
        });

        button->set_callback([&, slider, textBox] {
            if (m_image) {
                m_exposure = 0.18f / m_image->getLogMeanLuminance();
            }
            slider->set_value(0.18f);
            textBox->set_value(0.18f);
        });
    } else if (index == 2) {
        /* See Eqs. (1) and (11) in "Perceptual Effects in Real-time Tone Mapping"
           by Krawczyk et al. 2005. */
        if (m_image) {
            float tmp = 1.03f - 2.f / (2.f + std::log10(m_image->getLogMeanLuminance() + 1.f));
            m_exposure = tmp / m_image->getLogMeanLuminance();
        }
    }

    setTonemapOperator(m_tonemapOperatorIndex);
}

void TonemapperGui::setTonemapOperator(int index) {
    m_tonemapOperatorIndex = index;

    uint32_t indices[3*2] = {
        0, 1, 2,
        2, 3, 0
    };

    float positions[2*4] = {
        0.f, 0.f,
        1.f, 0.f,
        1.f, 1.f,
        0.f, 1.f
    };

    TonemapOperator *op = m_operators[m_tonemapOperatorIndex];
    m_shader = new Shader(m_renderPass, op->name, op->vertexShader, op->fragmentShader);
    m_shader->set_uniform("exposure", 1.f);
    for (auto &parameter : op->parameters) {
        Parameter &p = parameter.second;
        m_shader->set_uniform(p.uniform, p.defaultValue);
    }

    m_shader->set_buffer("indices", VariableType::UInt32, {3*2}, indices);
    m_shader->set_buffer("position", VariableType::Float32, {4, 2}, positions);
    if (m_image) {
        m_shader->set_texture("source", m_texture);
    }

    if (m_tonemapLabel) {
        m_window->remove_child(m_tonemapLabel);
    }

    m_tonemapLabel = new Label(m_window, "Tonemapping operator", "sans-bold");

    if (m_tonemapPopupButton) {
        m_window->remove_child(m_tonemapPopupButton);
    }

    m_tonemapPopupButton = new PopupButton(m_window);
    m_tonemapPopupButton->set_tooltip(op->description);
    m_tonemapPopup = m_tonemapPopupButton->popup();
    m_tonemapPopup->set_width(220);
    // m_tonemapPopup->set_height(130);

    auto scroll = new VScrollPanel(m_tonemapPopup);
    scroll->set_layout(new BoxLayout(Orientation::Vertical));
    auto popopPanel = new Widget(scroll);

    popopPanel->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Fill, 10, 10));
    // int newIndex = 0;
    for (size_t i = 0; i < m_operators.size(); ++i) {
        TonemapOperator *opi = m_operators[i];
        auto button = new Button(popopPanel, opi->name);
        button->set_tooltip(opi->description);
        button->set_flags(Button::RadioButton);
        button->set_callback([&, i] {
            m_tonemapPopup->set_visible(false);
            setTonemapOperator(i);
        });
    }
    m_tonemapPopupButton->set_caption(op->name);

    if (m_tonemapWidget) {
        m_window->remove_child(m_tonemapWidget);
    }

    m_tonemapWidget = new Widget(m_window);
    m_tonemapWidget->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Minimum, 0, 10));

    for (auto &parameter : op->parameters) {
        auto &p = parameter.second;
        if (p.constant) continue;

        auto *windowPanel = new Widget(m_tonemapWidget);
        windowPanel->set_layout(new BoxLayout(Orientation::Horizontal, Alignment::Middle, 0, 0));

        auto button = new Button(windowPanel, parameter.first);
        button->set_fixed_size(Vector2i(50, 22));
        button->set_font_size(15);
        button->set_tooltip(p.description);

        auto *slider = new Slider(windowPanel);
        slider->set_fixed_size(Vector2i(140, 22));
        slider->set_value(inverseLerp(p.value, p.minValue, p.maxValue));

        auto textBox = new FloatBox<float>(windowPanel);
        textBox->set_fixed_size(Vector2i(50, 22));
        textBox->number_format("%.2f");
        textBox->set_font_size(15);
        textBox->set_value(p.value);
        textBox->set_alignment(TextBox::Alignment::Right);
        textBox->set_editable(true);

        textBox->set_callback([&, slider, textBox](float v) {
            p.value = v;
            textBox->set_value(v);
            slider->set_value(inverseLerp(p.value, p.minValue, p.maxValue));
            // refreshGraph();
        });

        slider->set_callback([&, textBox](float t) {
            p.value = lerp(t, p.minValue, p.maxValue);
            textBox->set_value(p.value);
            // refreshGraph();
        });

        button->set_callback([&, slider, textBox] {
            p.value = p.defaultValue;
            slider->set_value(inverseLerp(p.value, p.minValue, p.maxValue));
            textBox->set_value(p.defaultValue);
            // refreshGraph();
        });
    }

    // refreshGraph();

    auto ctx = nvg_context();
    perform_layout(ctx);
}

bool TonemapperGui::keyboard_event(int key, int scancode, int action, int modifiers) {
    if (Screen::keyboard_event(key, scancode, action, modifiers))
        return true;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        set_visible(false);
        return true;
    }
    return false;
}

bool TonemapperGui::mouse_motion_event(const Vector2i &p, const Vector2i &rel, int button, int modifiers) {
    if (Screen::mouse_motion_event(p, rel, button, modifiers))
        return true;

    if (button == 1) {
        m_imageDisplayOffsetX += rel[0];
        m_imageDisplayOffsetY += rel[1];
        return true;
    }
    return false;
}

bool TonemapperGui::scroll_event(const Vector2i &p, const Vector2f &rel) {
    if (Screen::scroll_event(p, rel))
        return true;

    m_imageDisplayScale += rel[1];
    float minmax = 15.f;
    m_imageDisplayScale = std::min(minmax, std::max(m_imageDisplayScale, -minmax));
    return true;
}

void TonemapperGui::draw_contents() {
    m_renderPass->resize(framebuffer_size());
    m_renderPass->begin();

    if (m_texture) {
        m_texture->upload((const uint8_t *)m_image->getData());

        float scale = m_pixel_ratio * std::pow(1.1f, m_imageDisplayScale);
        GLint x = (GLint) (m_fbsize[0] - scale*m_imageDisplayWidth)  / 2 + m_pixel_ratio*m_imageDisplayOffsetX;
        GLint y = (GLint) (m_fbsize[1] - scale*m_imageDisplayHeight) / 2 - m_pixel_ratio*m_imageDisplayOffsetY;
        GLsizei width  = (GLsizei) (scale*m_imageDisplayWidth);
        GLsizei height = (GLsizei) (scale*m_imageDisplayHeight);
        glViewport(x, y, width, height);

        m_shader->begin();
        m_shader->draw_array(nanogui::Shader::PrimitiveType::Triangle, 0, 6, true);
        m_shader->set_uniform("exposure", m_exposure);
        TonemapOperator *op = m_operators[m_tonemapOperatorIndex];
        for (auto &parameter : op->parameters) {
            Parameter &p = parameter.second;
            m_shader->set_uniform(p.uniform, p.value);
        }
        m_shader->end();
    }

    m_renderPass->end();
}

void TonemapperGui::draw(NVGcontext *ctx) {
    if (m_saveProgressBar) {
        m_saveProgressBar->set_value(m_saveProgress);
        if (m_saveProgress < 0.f) {
            m_saveThread->join();
            delete m_saveThread;
            m_saveProgressBar = nullptr;
            m_saveWindow->dispose();
        }
    }
    Screen::draw(ctx);
}

} // Namespace tonemapper