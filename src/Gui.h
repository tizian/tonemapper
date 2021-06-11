#pragma once

#include <Global.h>
#include <nanogui/screen.h>

namespace tonemapper {

class TonemapperGui : public nanogui::Screen {
public:
    TonemapperGui();
    virtual ~TonemapperGui();

private:
    nanogui::Window *m_window = nullptr;

    nanogui::Button *m_saveButton = nullptr;
};

} // Namespace tonemapper
