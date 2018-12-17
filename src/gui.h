/*
    src/gui.h -- Graphical user interface

    Copyright (c) 2016 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#pragma once

#include <global.h>

#include <thread>

#include <nanogui/glutil.h>
#include <nanogui/nanogui.h>

class Image;
class TonemapOperator;

class TonemapperScreen : public nanogui::Screen {
public:
    TonemapperScreen();
    ~TonemapperScreen();

    void setImage(const std::string &filename);
    void setTonemapMode(int index);
    void setExposureMode(int index);

    void refreshGraph();

    virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) override;
    virtual bool mouseMotionEvent(const Eigen::Vector2i &p, const Eigen::Vector2i &rel, int button, int modifiers) override;
    virtual bool scrollEvent(const Eigen::Vector2i &p, const Eigen::Vector2f &rel) override;
    virtual bool dropEvent(const std::vector<std::string> & filenames) override;
    virtual void drawContents() override;
    virtual void draw(NVGcontext *ctx) override;

private:
    void setEnabledRecursive(nanogui::Widget *widget, bool enabled);

    std::vector<TonemapOperator *> m_tonemapOperators;
    int m_tonemapIndex;
    int m_exposureIndex;

    Image                   *m_image = nullptr;
    uint32_t                 m_texture = 0;

    float                    m_exposure = 1.f;

    std::thread             *m_saveThread = nullptr;
    float                    m_progress = 0.f;

    nanogui::Button         *m_saveButton = nullptr;
    nanogui::Window         *m_saveWindow = nullptr;
    nanogui::ProgressBar    *m_progressBar = nullptr;
    nanogui::Window         *m_window = nullptr;
    nanogui::Label          *m_tonemapLabel = nullptr;
    nanogui::PopupButton    *m_tonemapPopupButton = nullptr;
    nanogui::Popup          *m_tonemapPopup = nullptr;
    nanogui::Widget         *m_tonemapWidget = nullptr;
    nanogui::Label          *m_exposureLabel = nullptr;
    nanogui::PopupButton    *m_exposurePopupButton = nullptr;
    nanogui::Popup          *m_exposurePopup = nullptr;
    nanogui::Widget         *m_exposureWidget = nullptr;
    nanogui::Graph          *m_graph = nullptr;

    const int                MAIN_WIDTH = 960;
    Eigen::Vector2i          m_windowSize;
    Eigen::Vector2i          m_scaledImageSize;
    Eigen::Vector2i          m_imageOffset;
    float                    m_imageScale;

    bool                     m_shiftDown;
};