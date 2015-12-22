#pragma once

#include <global.h>

#include <nanogui/glutil.h>
#include <nanogui/nanogui.h>

#include <operators/gamma.h>
#include <operators/srgb.h>
#include <operators/reinhard.h>

class Image;
class TonemapOperator;

class TonemapperScreen : public nanogui::Screen {
public:
	TonemapperScreen();
	~TonemapperScreen();

	void setImage(const std::string &filename);
	void setTonemap(TonemapOperator *tonemap);

	void refreshGraph();

	virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) override;
	virtual bool dropEvent(const std::vector<std::string> & filenames) override;
	virtual void drawContents() override;

private:
	Image 				*m_image = nullptr;
	TonemapOperator 	*m_tonemap = nullptr;
    
	float 			 	 m_exposure = 1.f;

    nanogui::Window 	*m_window = nullptr;
	nanogui::ComboBox 	*m_tonemapSelection = nullptr;
	nanogui::Widget 	*m_paramWidget = nullptr;
	nanogui::Graph 		*m_graph = nullptr;

	const int 			 MAIN_WIDTH = 960;
	Eigen::Vector2i 	 m_windowSize;
	Eigen::Vector2i 	 m_scaledImageSize;
	uint32_t 			 m_texture = 0;
};