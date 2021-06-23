#include <Gui.h>

#include <Image.h>

#include <nanogui/button.h>
#include <nanogui/icons.h>
#include <nanogui/label.h>
#include <nanogui/layout.h>
#include <nanogui/messagedialog.h>
#include <nanogui/opengl.h>
#include <nanogui/renderpass.h>
#include <nanogui/shader.h>
#include <nanogui/window.h>

namespace tonemapper {

using namespace nanogui;

TonemapperGui::TonemapperGui()
    : nanogui::Screen(Vector2i(800, 600), "", true, false) {
    using namespace nanogui;

    // Setup display
    m_renderPass = new RenderPass({ this });
    m_renderPass->set_cull_mode(RenderPass::CullMode::Disabled);
    m_renderPass->set_clear_color(0, Color(50, 50, 60, 255));

    glfwSetWindowPos(glfw_window(), 50, 100);

    m_shader = new Shader(
        m_renderPass,

        "Image Preview",

        /* Vertex shader */
        R"(
        #version 330

        in vec2 position;
        out vec2 uv;

        void main() {
            gl_Position = vec4(position.x*2-1, position.y*2-1, 0.0, 1.0);
            uv = vec2(position.x, 1-position.y);
        })",

        /* Fragment shader */
        R"(

        #version 330
        out vec4 out_color;
        uniform sampler2D source;
        in vec2 uv;

        float toSRGB(float value) {
            if (value < 0.0031308)
                return 12.92 * value;
            return 1.055 * pow(value, 0.41666) - 0.055;
        }

        void main() {
            float scale = 1.0;
            vec4 color = texture(source, uv);
            //vec4 color = vec4(1.0, 1.0, 1.0, 1.0);
            color *= scale;
            out_color = vec4(toSRGB(color.r), toSRGB(color.g), toSRGB(color.b), 1);
        })"
    );

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

    m_shader->set_buffer("indices", VariableType::UInt32, {3*2}, indices);
    m_shader->set_buffer("position", VariableType::Float32, {4, 2}, positions);







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
            VARLOG(filename);
            setImage(filename);
        }
    });

    m_saveButton = new Button(m_window, "Save LDR image", FA_SAVE);
    m_saveButton->set_background_color(Color(0, 0, 255, 25));
    m_saveButton->set_tooltip("Save LDR image (.png or .jpg)");
    m_saveButton->set_callback([&] {
        std::string filename = file_dialog({ {"jpg", "JPEG"}, {"png", "Portable Network Graphics"} }, true);
        if (m_image && filename != "") {
        //     m_saveWindow = new Window(this, "Saving tonemapped image..");
        //     m_saveWindow->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Middle, 10, 10));
        //     m_saveWindow->setModal(true);
        //     m_saveWindow->setFixedWidth(300);
        //     m_saveWindow->setVisible(true);

        //     auto panel = new Widget(m_saveWindow);
        //     panel->setLayout(new BoxLayout(Orientation::Horizontal, Alignment::Middle, 10, 15));

        //     m_saveWindow->center();
        //     m_saveWindow->requestFocus();
        //     m_progressBar = new ProgressBar(panel);
        //     m_progressBar->setFixedWidth(200);

        //     performLayout(nvgContext());

        //     m_saveThread = new std::thread([&, filename]{
        //         m_image->saveAsPNG(filename, m_tonemapOperators[m_tonemapIndex], m_exposure, &m_progress);
        //     });
        }
    });
    m_saveButton->set_enabled(false);

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
        m_shader->end();
    }

    m_renderPass->end();
}



} // Namespace tonemapper
