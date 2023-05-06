/*
    Copyright (c) 2022 Tizian Zeltner

    tonemapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#include <Gui.h>

#include <Image.h>
#include <Tonemap.h>

#include <nanogui/button.h>
#include <nanogui/graph.h>
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
#include <nanogui/toolbutton.h>
#include <nanogui/window.h>
#include <nanogui/vscrollpanel.h>

#include <thread>
#include <filesystem>

const size_t SCREEN_WIDTH_DEFAULT  = 1280;
const size_t SCREEN_HEIGHT_DEFAULT = 720;

const size_t MAIN_WINDOW_WIDTH   = 280;
const size_t GRAPH_WINDOW_WIDTH  = 280;
const size_t GRAPH_WINDOW_HEIGHT = 200;

namespace tonemapper {

using namespace nanogui;

TonemapperGui::TonemapperGui()
    : nanogui::Screen(Vector2i(SCREEN_WIDTH_DEFAULT, SCREEN_HEIGHT_DEFAULT), "", true, false) {
    m_screenSize = Vector2i(SCREEN_WIDTH_DEFAULT, SCREEN_HEIGHT_DEFAULT);
    m_exposureModeIndex = 0;
    m_tonemapOperatorIndex = 0;

    // Get ordered list of names
    std::vector<std::string> operatorNames = TonemapOperator::orderedNames();

    // Instantiate all operators
    m_operators = std::vector<TonemapOperator *>(operatorNames.size());
    for (size_t i = 0; i < operatorNames.size(); ++i) {
        if (operatorNames[i].compare("") == 0) {
            m_operators[i] = nullptr;
        } else {
            m_operators[i] = TonemapOperator::create(operatorNames[i]);
        }
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

    m_mainWindow = new Window(this, "Options");
    m_mainWindow->set_position(Vector2i(25, 25));
    m_mainWindow->set_fixed_width(MAIN_WINDOW_WIDTH);
    m_mainWindow->set_layout(layout);
    m_mainWindow->set_visible(true);

    auto about = new Button(m_mainWindow->button_panel(), "", FA_INFO);
    about->set_callback([&, ctx] {
        std::ostringstream oss;
        oss << "tonemapper v" << VERSION << std::endl
            << std::endl
            << "© " << YEAR << " Tizian Zeltner" << std::endl
            << std::endl
            << "More information can be found at" << std::endl
            << "http://github.com/tizian/tonemapper" << std::endl;

        auto dlg = new MessageDialog(this, MessageDialog::Type::Information, "About", oss.str());
        dlg->message_label()->set_fixed_width(380);
        dlg->message_label()->set_font_size(20);
        perform_layout(ctx);
        dlg->center();
    });

    new Label(m_mainWindow, "Image I/O", "sans-bold");

    auto *openButton = new Button(m_mainWindow, "Open HDR image", FA_FOLDER_OPEN);
    openButton->set_background_color(Color(0, 255, 0, 25));
    openButton->set_tooltip("Open HDR image (.exr or .hdr)");
    openButton->set_callback([&] {
        std::string filename = file_dialog({ {"exr", "OpenEXR"}, {"hdr", "Radiance RGBE"} }, false);
        if (filename != "") {
            setImage(filename);
        }
    });

    m_saveButton = new Button(m_mainWindow, "Save LDR image", FA_SAVE);
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

    m_graphWindow = new Window(this, "Graph");
    m_graphWindow->set_position(Vector2i(SCREEN_WIDTH_DEFAULT  - 25 - GRAPH_WINDOW_WIDTH,
                                         SCREEN_HEIGHT_DEFAULT - 25 - GRAPH_WINDOW_HEIGHT));
    m_graphWindow->set_fixed_size(Vector2i(GRAPH_WINDOW_WIDTH, GRAPH_WINDOW_HEIGHT));
    m_graphWindow->set_layout(layout);
    m_graphWindow->set_visible(false);

    perform_layout(ctx);
    draw_all();
    set_visible(true);

    std::ostringstream oss;
    oss << "tonemapper v" << VERSION;
    std::string title = oss.str();
    set_caption(title);
    PRINT("");
}

TonemapperGui::~TonemapperGui() {}

void TonemapperGui::setImage(const std::string &filename) {
    if (m_image) {
        delete m_image;
        m_image = nullptr;
    }

    PRINT_("Read image \"%s\" ..", filename);
    m_image = Image::load(filename);

    if (!m_image) {
        return;
    }

    if (m_image) {
        m_saveButton->set_enabled(true);
    }

    m_imageDisplayHeight = SCREEN_HEIGHT_DEFAULT;
    m_imageDisplayWidth  = SCREEN_HEIGHT_DEFAULT * int(m_image->getWidth() / m_image->getHeight());
    m_screenSize = Vector2i(std::max(int(SCREEN_WIDTH_DEFAULT), m_imageDisplayWidth), m_imageDisplayHeight);
    set_size(Vector2i(m_screenSize));

    m_imageDisplayScale = 0.f;
    m_imageDisplayOffsetX = 0;
    m_imageDisplayOffsetY = 0;

    m_graphWindow->set_position(Vector2i(std::max(1280, m_imageDisplayWidth)  - 25 - GRAPH_WINDOW_WIDTH,
                                         m_imageDisplayHeight - 25 - GRAPH_WINDOW_HEIGHT));

    m_texture = new nanogui::Texture(
        nanogui::Texture::PixelFormat::RGB,
        nanogui::Texture::ComponentFormat::Float32,
        Vector2i(int(m_image->getWidth()), int(m_image->getHeight())),
        nanogui::Texture::InterpolationMode::Nearest,
        nanogui::Texture::InterpolationMode::Nearest
    );

    setExposureMode(m_exposureModeIndex);
    PRINT(" done.");
}

void TonemapperGui::setExposureMode(int index) {
    if (!m_image) {
        return;
    }

    m_exposureModeIndex = index;

    if (m_exposureLabel) {
        m_mainWindow->remove_child(m_exposureLabel);
    }

    m_exposureLabel = new Label(m_mainWindow, "Exposure mode", "sans-bold");

    if (m_exposurePopupButton) {
        m_mainWindow->remove_child(m_exposurePopupButton);
    }

    std::vector<std::string> exposureNames{"Manual", "Key Value", "Auto"};
    std::vector<std::string> exposureDescriptions{
        "Manual Mode\n\nScale the input image with a factor of 2^Exposure.",
        "Key Value mode\n\nScale the input image with a key value as described in \"Photographic Tone Reproduction for Digital Images\" by Reinhard et al. 2002.",
        "Auto mode\n\nAuto adjust the input image exposure as proposed in \"Perceptual Effects in Real-time Tone Mapping\" by Krawczyk et al. 2005."
    };

    m_exposurePopupButton = new PopupButton(m_mainWindow);
    m_exposurePopup = m_exposurePopupButton->popup();
    m_exposurePopup->set_width(130);
    m_exposurePopup->set_height(130);

    auto tmpWidget = new Widget(m_exposurePopup);
    tmpWidget->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Fill));
    auto popupPanel = new Widget(tmpWidget);
    popupPanel->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Fill, 10, 10));

    for (int i = 0; i < 3; ++i) {
        auto button = new Button(popupPanel, exposureNames[i]);
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
        m_mainWindow->remove_child(m_exposureWidget);
    }

    m_exposureWidget = new Widget(m_mainWindow);
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
        m_exposure = 0.18f / m_image->getLogMeanLuminance();

        textBox->set_callback([&, slider, textBox](float v) {
            textBox->set_value(v);
            m_exposure = v / m_image->getLogMeanLuminance();
            slider->set_value(v);
        });

        slider->set_callback([&, textBox](float t) {
            m_exposure = t / m_image->getLogMeanLuminance();
            textBox->set_value(t);
        });

        button->set_callback([&, slider, textBox] {
            m_exposure = 0.18f / m_image->getLogMeanLuminance();
            slider->set_value(0.18f);
            textBox->set_value(0.18f);
        });
    } else if (index == 2) {
        /* See Eqs. (1) and (11) in "Perceptual Effects in Real-time Tone Mapping"
           by Krawczyk et al. 2005. */
        float key = 1.03f - 2.f / (2.f + std::log10(m_image->getLogMeanLuminance() + 1.f));
        m_exposure = key / m_image->getLogMeanLuminance();
    }

    setTonemapOperator(m_tonemapOperatorIndex);
}

void TonemapperGui::setTonemapOperator(const std::string &name) {
    std::vector<std::string> operatorNames = TonemapOperator::orderedNames();
    auto it = std::find(operatorNames.begin(), operatorNames.end(), name);
    if (it != operatorNames.end()) {
        setTonemapOperator(int(std::distance(operatorNames.begin(), it)));
    }
}

void TonemapperGui::setTonemapOperator(int index) {
    if (!m_image) {
        return;
    }
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
    op->preprocess(m_image);
    m_shader = new Shader(m_renderPass, op->name, op->vertexShader, op->fragmentShader);
    m_shader->set_uniform("exposure", 1.f);
    for (auto &parameter : op->parameters) {
        Parameter &p = parameter.second;
        m_shader->set_uniform(p.uniform, p.defaultValue);
    }

    m_shader->set_buffer("indices", VariableType::UInt32, {3*2}, indices);
    m_shader->set_buffer("position", VariableType::Float32, {4, 2}, positions);
    m_shader->set_texture("source", m_texture);

    if (m_tonemapLabel) {
        m_mainWindow->remove_child(m_tonemapLabel);
    }

    m_tonemapLabel = new Label(m_mainWindow, "Tonemapping operator", "sans-bold");

    if (m_tonemapPopupButton) {
        m_mainWindow->remove_child(m_tonemapPopupButton);
    }

    m_tonemapPopupButton = new PopupButton(m_mainWindow);
    m_tonemapPopupButton->set_tooltip(op->description);

    m_tonemapPopup = m_tonemapPopupButton->popup();
    m_tonemapPopup->set_width(300);
    m_tonemapPopup->set_height(300);

    auto scroll = new VScrollPanel(m_tonemapPopup);
    scroll->set_layout(new BoxLayout(Orientation::Vertical));
    auto popupPanel = new Widget(scroll);

    popupPanel->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Fill, 10, 10));
    for (size_t i = 0; i < m_operators.size(); ++i) {
        TonemapOperator *opi = m_operators[i];
        if (opi == nullptr) {
            new Label(popupPanel, " ", "sans-bold", 6);
            continue;
        }

        auto button = new Button(popupPanel, opi->name);
        button->set_tooltip(opi->description);
        button->set_flags(Button::RadioButton);
        button->set_callback([&, i] {
            m_tonemapPopup->set_visible(false);
            setTonemapOperator(int(i));
        });
    }
    m_tonemapPopupButton->set_caption(op->name);

    if (m_tonemapWidget) {
        m_mainWindow->remove_child(m_tonemapWidget);
    }

    m_tonemapWidget = new Widget(m_mainWindow);
    m_tonemapWidget->set_layout(new BoxLayout(Orientation::Vertical, Alignment::Fill, 0, 10));

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
            refreshGraph();
        });

        slider->set_callback([&, textBox](float t) {
            p.value = lerp(t, p.minValue, p.maxValue);
            textBox->set_value(p.value);
            refreshGraph();
        });

        button->set_callback([&, slider, textBox] {
            p.value = p.defaultValue;
            slider->set_value(inverseLerp(p.value, p.minValue, p.maxValue));
            textBox->set_value(p.defaultValue);
            refreshGraph();
        });
    }

    if (op->dataDriven) {
        auto *rfOpenButton = new Button(m_tonemapWidget, "Open RF file", FA_FOLDER_OPEN);
        m_rfLabel = new Label(m_tonemapWidget, "RF: none", "sans-bold");
        rfOpenButton->set_background_color(Color(0, 255, 0, 25));
        rfOpenButton->set_tooltip("Open camera response function file.");
        rfOpenButton->set_callback([&] {
            std::string filename = file_dialog({ {"rf", "Response function"}, {"txt", "Text file"} }, false);
            if (filename != "") {
                TonemapOperator *op = m_operators[m_tonemapOperatorIndex];
                op->fromFile(filename);
                std::filesystem::path path(filename);

                m_tonemapWidget->remove_child(m_rfLabel);
                std::string label = "RF: " + path.stem().string();
                m_rfLabel = new Label(m_tonemapWidget, label, "sans-bold");
                refreshGraph();

                m_rfTextureR = new nanogui::Texture(
                    nanogui::Texture::PixelFormat::R,
                    nanogui::Texture::ComponentFormat::Float32,
                    Vector2i(int(op->irradiance.size()), 1),
                    nanogui::Texture::InterpolationMode::Bilinear,
                    nanogui::Texture::InterpolationMode::Bilinear
                );
                m_rfTextureG = new nanogui::Texture(
                    nanogui::Texture::PixelFormat::R,
                    nanogui::Texture::ComponentFormat::Float32,
                    Vector2i(int(op->irradiance.size()), 1),
                    nanogui::Texture::InterpolationMode::Bilinear,
                    nanogui::Texture::InterpolationMode::Bilinear
                );
                m_rfTextureB = new nanogui::Texture(
                    nanogui::Texture::PixelFormat::R,
                    nanogui::Texture::ComponentFormat::Float32,
                    Vector2i(int(op->irradiance.size()), 1),
                    nanogui::Texture::InterpolationMode::Bilinear,
                    nanogui::Texture::InterpolationMode::Bilinear
                );
                m_rfTextureR->upload((const uint8_t *) &op->values[0][0]);
                m_rfTextureG->upload((const uint8_t *) &op->values[1][0]);
                m_rfTextureB->upload((const uint8_t *) &op->values[2][0]);

                m_shader->set_texture("dataR", m_rfTextureR);
                m_shader->set_texture("dataG", m_rfTextureG);
                m_shader->set_texture("dataB", m_rfTextureB);
            }
        });
    }

    refreshGraph();

    auto ctx = nvg_context();
    perform_layout(ctx);
}

void TonemapperGui::refreshGraph() {
    m_graphWindow->set_visible(true);

    if (m_graph) {
        m_graphWindow->remove_child(m_graph);
    }

    m_graph = new RgbGraph(m_graphWindow, "Luminance [0, 1]");
    m_graph->set_footer("Log luminance [-5, 2]");
    m_graph->set_fixed_height(150);

    size_t res = 100;
    std::vector<float> valuesR(res),
                       valuesG(res),
                       valuesB(res);
    for (size_t i = 0; i < res; ++i) {
        float t = float(i) / (res - 1),
              v = std::pow(2.f, 7.f*t - 5.f);

        Color3f c = m_operators[m_tonemapOperatorIndex]->map(Color3f(v), 1.f);
        for (size_t ch = 0; ch < 3; ++ch) {
            if (!std::isfinite(c[i])) {
                c[i] = 0.f;
            }
        }
        valuesR[i] = c.r();
        valuesG[i] = c.g();
        valuesB[i] = c.b();

    }
    m_graph->set_valuesR(valuesR);
    m_graph->set_valuesG(valuesG);
    m_graph->set_valuesB(valuesB);

    auto ctx = nvg_context();
    perform_layout(ctx);
}

bool TonemapperGui::keyboard_event(int key, int scancode, int action, int modifiers) {
    if (Screen::keyboard_event(key, scancode, action, modifiers)) {
        return true;
    }
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        set_visible(false);
        return true;
    }

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) {
        m_mainWindow->set_visible(!m_mainWindow->visible());
        m_graphWindow->set_visible(!m_graphWindow->visible());
        return true;
    }

    bool center     = false;
    bool fullscreen = false;

    if (key == GLFW_KEY_C && action == GLFW_PRESS) {
        center = true;
    }

    if (key == GLFW_KEY_F && action == GLFW_PRESS) {
        center = true;
        fullscreen = true;
    }

    if (center) {
        m_imageDisplayOffsetX = 0;
        m_imageDisplayOffsetY = 0;
    }
    if (fullscreen) {
        float scale = std::pow(1.1f, m_imageDisplayScale);
        Vector2f screenSize = Vector2f(m_screenSize),
                 imageSize  = scale*Vector2f(float(m_imageDisplayWidth),
                                             float(m_imageDisplayHeight));

        Vector2f fullScale = screenSize / imageSize;
        scale *= fullScale.x() < fullScale.y() ? fullScale.x() : fullScale.y();
        m_imageDisplayScale = std::log(scale) / std::log(1.1f);
    }


    if (fullscreen || center) {
        return true;
    }

    return false;
}

bool TonemapperGui::mouse_motion_event(const Vector2i &p, const Vector2i &rel, int button, int modifiers) {
    if (Screen::mouse_motion_event(p, rel, button, modifiers)) {
        return true;
    }

    if (button == 1) {
        m_imageDisplayOffsetX += rel[0];
        m_imageDisplayOffsetY += rel[1];
        return true;
    }
    return false;
}

bool TonemapperGui::scroll_event(const Vector2i &p, const Vector2f &rel) {
    if (Screen::scroll_event(p, rel)) {
        return true;
    }

    m_imageDisplayScale += rel[1];
    float minmax = 15.f;
    m_imageDisplayScale = std::min(minmax, std::max(m_imageDisplayScale, -minmax));
    return true;
}

bool TonemapperGui::resize_event(const Vector2i& screenSizeNew) {
    Vector2i screenSizeOld = m_screenSize;

    Vector2i graphPos    = m_graphWindow->position(),
             graphBR     = graphPos + Vector2i(GRAPH_WINDOW_WIDTH, GRAPH_WINDOW_HEIGHT),
             graphOffset = screenSizeOld - graphBR;

    m_graphWindow->set_position(Vector2i(screenSizeNew.x() - graphOffset.x() - GRAPH_WINDOW_WIDTH,
                                         screenSizeNew.y() - graphOffset.y() - GRAPH_WINDOW_HEIGHT));

    m_screenSize = screenSizeNew;
    Screen::resize_event(screenSizeNew);
    return true;
}

bool TonemapperGui::drop_event(const std::vector<std::string> &filenames) {
    if (filenames.size() > 0) {
        std::string extension = std::filesystem::path(filenames[0]).extension().string();
        if (extension != ".exr" && extension != ".hdr") {
            auto dlg = new MessageDialog(this, MessageDialog::Type::Warning, "Unsupported image format", "Only EXR (.exr) and HDR (.hdr) image formats are supported currently.");
            dlg->center();
        } else {
            setImage(filenames[0]);
        }
    }
    return true;
}

void TonemapperGui::draw_contents() {
    m_renderPass->resize(framebuffer_size());
    m_renderPass->begin();

    if (m_image && m_texture) {
        m_texture->upload((const uint8_t *)m_image->getData());

        float scale = m_pixel_ratio * std::pow(1.1f, m_imageDisplayScale);
        GLint x = GLint((m_fbsize[0] - scale*m_imageDisplayWidth)  / 2 + m_pixel_ratio*m_imageDisplayOffsetX);
        GLint y = GLint((m_fbsize[1] - scale*m_imageDisplayHeight) / 2 - m_pixel_ratio*m_imageDisplayOffsetY);
        GLsizei width  = GLsizei(scale*m_imageDisplayWidth);
        GLsizei height = GLsizei(scale*m_imageDisplayHeight);
        glViewport(x, y, width, height);

        TonemapOperator *op = m_operators[m_tonemapOperatorIndex];
        if (!op || !op->dataDriven || op->irradiance.size() > 0) {
            m_shader->begin();
            m_shader->draw_array(nanogui::Shader::PrimitiveType::Triangle, 0, 6, true);
            m_shader->set_uniform("exposure", m_exposure);

            if (op) {
                for (auto& parameter : op->parameters) {
                    Parameter& p = parameter.second;
                    m_shader->set_uniform(p.uniform, p.value);
                }
            }
            m_shader->end();
        }
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

RgbGraph::RgbGraph(Widget *parent, const std::string &caption)
    : Widget(parent), m_caption(caption) {}

Vector2i RgbGraph::preferred_size(NVGcontext *) const {
    return Vector2i(180, 45);
}

void RgbGraph::draw(NVGcontext *ctx) {
    Color backgroundColor(20, 128);
    Color fillColors[3] = {
        Color(255, 0, 0, 64),
        Color(0, 255, 0, 64),
        Color(0, 0, 255, 64)
    };


    Color strokeColors[3] = {
        Color(255, 0, 0, 255),
        Color(0, 255, 0, 255),
        Color(0, 0, 255, 255)
    };
    Color textColor(240, 192);

    Widget::draw(ctx);

    nvgBeginPath(ctx);
    nvgRect(ctx, float(m_pos.x()), float(m_pos.y()), float(m_size.x()), float(m_size.y()));
    nvgFillColor(ctx, backgroundColor);
    nvgFill(ctx);

    for (size_t ch = 0; ch < 3; ++ch) {
        if (m_values[ch].size() < 2) {
            return;
        }

        nvgBeginPath(ctx);
        nvgMoveTo(ctx, float(m_pos.x()), float(m_pos.y()+m_size.y()));
        for (size_t i = 0; i < (size_t) m_values[ch].size(); i++) {
            float value = m_values[ch][i];
            float vx = m_pos.x() + i * m_size.x() / (float) (m_values[ch].size() - 1);
            float vy = m_pos.y() + (1-value) * m_size.y();
            nvgLineTo(ctx, vx, vy);
        }

        nvgLineTo(ctx, float(m_pos.x() + m_size.x()), float(m_pos.y() + m_size.y()));
        nvgStrokeColor(ctx, strokeColors[ch]);
        nvgStrokeWidth(ctx, 2);
        nvgStroke(ctx);
        if (fillColors[ch].w() > 0) {
            nvgFillColor(ctx, fillColors[ch]);
            nvgFill(ctx);
        }
    }

    nvgFontFace(ctx, "sans");

    if (!m_caption.empty()) {
        nvgFontSize(ctx, 14.0f);
        nvgTextAlign(ctx, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
        nvgFillColor(ctx, textColor);
        nvgText(ctx, float(m_pos.x() + 3), float(m_pos.y() + 1), m_caption.c_str(), NULL);
    }

    if (!m_header.empty()) {
        nvgFontSize(ctx, 18.0f);
        nvgTextAlign(ctx, NVG_ALIGN_RIGHT | NVG_ALIGN_TOP);
        nvgFillColor(ctx, textColor);
        nvgText(ctx, float(m_pos.x() + m_size.x() - 3), float(m_pos.y() + 1), m_header.c_str(), NULL);
    }

    if (!m_footer.empty()) {
        nvgFontSize(ctx, 15.0f);
        nvgTextAlign(ctx, NVG_ALIGN_RIGHT | NVG_ALIGN_BOTTOM);
        nvgFillColor(ctx, textColor);
        nvgText(ctx, float(m_pos.x() + m_size.x() - 3), float(m_pos.y() + m_size.y() - 1), m_footer.c_str(), NULL);
    }

    nvgBeginPath(ctx);
    nvgRect(ctx, float(m_pos.x()), float(m_pos.y()), float(m_size.x()), float(m_size.y()));
    nvgStrokeColor(ctx, Color(100, 255));
    nvgStroke(ctx);
}

} // Namespace tonemapper
