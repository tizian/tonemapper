#include <Gui.h>

#include <nanogui/button.h>
#include <nanogui/icons.h>
#include <nanogui/label.h>
#include <nanogui/layout.h>
#include <nanogui/messagedialog.h>
#include <nanogui/window.h>

namespace tonemapper {

using namespace nanogui;

TonemapperGui::TonemapperGui()
    : nanogui::Screen(Vector2i(1280, 720), "", false) {

    auto ctx = nvg_context();

    auto layout = new nanogui::GroupLayout();
    layout->set_spacing(10);
    layout->set_group_spacing(20);
    layout->set_group_indent(14);

    m_window = new nanogui::Window(this, "Options");
    m_window->set_position(Vector2i(25, 15));
    m_window->set_fixed_width(280);
    m_window->set_layout(layout);

    auto about = new nanogui::Button(m_window->button_panel(), "", FA_INFO);
    about->set_callback([&, ctx] {
        std::ostringstream oss;
        oss << "Tone Mapper v" << VERSION << std::endl
            << std::endl
            << "Copyright (c) " << YEAR << " Tizian Zeltner" << std::endl
            << std::endl
            << "More information can be found under" << std::endl
            << "http://github.com/tizian/tonemapper" << std::endl;

        auto dlg = new nanogui::MessageDialog(this, nanogui::MessageDialog::Type::Information, "About", oss.str());
        dlg->message_label()->set_fixed_width(380);
        dlg->message_label()->set_font_size(20);
        perform_layout(ctx);
        dlg->center();
    });

    new nanogui::Label(m_window, "Image I/O", "sans-bold");

    auto *openButton = new nanogui::Button(m_window, "Open HDR image", FA_FOLDER_OPEN);
    openButton->set_background_color(nanogui::Color(0, 255, 0, 25));
    openButton->set_tooltip("Open HDR image (.exr or .hdr)");
    openButton->set_callback([&] {
        std::string filename = file_dialog({ {"exr", "OpenEXR"}, {"hdr", "Radiance RGBE"} }, false);
        if (filename != "") {
            VARLOG(filename);
            // setImage(filename);
        }
    });

    m_saveButton = new nanogui::Button(m_window, "Save .png image", FA_SAVE);
    m_saveButton->set_background_color(nanogui::Color(0, 0, 255, 25));
    m_saveButton->set_tooltip("Save LDR image (.png, .jpg)");
    m_saveButton->set_callback([&] {
        // std::string filename = file_dialog({ { "png", "Portable Network Graphics" } }, true);
        // if (m_image && filename != "") {
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
        // }
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

} // Namespace tonemapper
