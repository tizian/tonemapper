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
	virtual bool dropEvent(const std::vector<std::string> & filenames) override;
	virtual void drawContents() override;
	virtual void draw(NVGcontext *ctx) override;

private:
	void setEnabledRecursive(nanogui::Widget *widget, bool enabled);

	std::vector<TonemapOperator *> m_tonemapOperators;
	int m_tonemapIndex;

	Image 					*m_image = nullptr;
    
	float 			 		m_exposure = 1.f;

	std::thread				*m_saveThread = nullptr;
	float					m_progress = 0.f;
	
	nanogui::Window			*m_saveWindow = nullptr;
	nanogui::ProgressBar	*m_progressBar = nullptr;
	nanogui::Window			*m_window = nullptr;
    nanogui::Label 			*m_tonemapLabel = nullptr;
	nanogui::PopupButton 	*m_tonemapPopupButton = nullptr;
	nanogui::Popup			*m_popup = nullptr;
	nanogui::Widget 		*m_tonemapWidget = nullptr;
	nanogui::Label 			*m_exposureLabel = nullptr;
	nanogui::ComboBox 		*m_exposureSelection = nullptr;
	nanogui::Widget 		*m_exposureWidget = nullptr;
	nanogui::Graph 			*m_graph = nullptr;

	const int 				MAIN_WIDTH = 960;
	Eigen::Vector2i 		m_windowSize;
	Eigen::Vector2i 		m_scaledImageSize;
	uint32_t 				m_texture = 0;
};