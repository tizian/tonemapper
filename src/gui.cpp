#include <gui.h>

#include <image.h>
#include <tonemap.h>

#include <operators/clamping.h>
#include <operators/drago.h>
#include <operators/exponential.h>
#include <operators/exponentiation.h>
#include <operators/ferwerda.h>
#include <operators/filmic.h>
#include <operators/insomniac.h>
#include <operators/uncharted.h>
#include <operators/linear.h>
#include <operators/logarithmic.h>
#include <operators/maxdivision.h>
#include <operators/meanvalue.h>
#include <operators/reinhard.h>
#include <operators/reinhard_devlin.h>
#include <operators/reinhard_extended.h>
#include <operators/schlick.h>
#include <operators/srgb.h>
#include <operators/tumblin_rushmeier.h>
#include <operators/ward.h>

TonemapperScreen::TonemapperScreen() : nanogui::Screen(Eigen::Vector2i(800, 600), "Tone Mapper", true, false) {
	using namespace nanogui;

	m_tonemapIndex = 0;
	m_tonemapOperators = std::vector<TonemapOperator *>();
	m_tonemapOperators.push_back(new LinearOperator());
	m_tonemapOperators.push_back(new SRGBOperator());
	m_tonemapOperators.push_back(new ReinhardOperator());
	m_tonemapOperators.push_back(new ExtendedReinhardOperator());
	m_tonemapOperators.push_back(new WardOperator());
	m_tonemapOperators.push_back(new FerwerdaOperator());
	m_tonemapOperators.push_back(new SchlickOperator());
	m_tonemapOperators.push_back(new TumblinRushmeierOperator());
	m_tonemapOperators.push_back(new DragoOperator());
	m_tonemapOperators.push_back(new ReinhardDevlinOperator());
	m_tonemapOperators.push_back(new FilmicOperator());
	m_tonemapOperators.push_back(new Uncharted2Operator());
	m_tonemapOperators.push_back(new InsomniacOperator());
	m_tonemapOperators.push_back(new MaximumDivisionOperator());
	m_tonemapOperators.push_back(new MeanValueOperator());
	m_tonemapOperators.push_back(new ClampingOperator());
	m_tonemapOperators.push_back(new LogarithmicOperator());
	m_tonemapOperators.push_back(new ExponentialOperator());
	m_tonemapOperators.push_back(new ExponentiationOperator());

	auto ctx = nvgContext();

	glfwSetWindowPos(glfwWindow(), 20, 40);
	setBackground(Vector3f(0.8f, 0.8f, 0.8f));

	auto layout = new GroupLayout();
	layout->setSpacing(10);
	layout->setGroupSpacing(20);
	layout->setGroupIndent(14);

	m_window = new Window(this, "Tone Mapper");
	m_window->setPosition(Vector2i(15, 15));
	m_window->setLayout(layout);

	auto about = new Button(m_window->buttonPanel(), "", ENTYPO_ICON_INFO);
	about->setCallback([&, ctx] {
		auto dlg = new MessageDialog(this, MessageDialog::Type::Information, "About Tone Mapper", "Info and description.\n\n(c) 2015 Tizian Zeltner");
		dlg->messageLabel()->setFixedWidth(300);
		dlg->messageLabel()->setFontSize(20);
		performLayout(ctx);
		dlg->center();
	});

	new Label(m_window, "Image I/O", "sans-bold");

	auto *openButton = new Button(m_window, "Open HDR image");
	openButton->setBackgroundColor(nanogui::Color(0, 255, 0, 25));
	openButton->setIcon(ENTYPO_ICON_FOLDER);
	openButton->setCallback([&] {
		std::string filename = file_dialog({ {"exr", "OpenEXR"} }, false);
		setImage(filename);
	});

	m_saveButton = new Button(m_window, "Save LDR image");
	m_saveButton->setBackgroundColor(nanogui::Color(0, 255, 0, 25));
	m_saveButton->setIcon(ENTYPO_ICON_SAVE);
	m_saveButton->setCallback([&] {
		std::string filename = file_dialog({ { "png", "Portable Network Graphics" } }, true);
		if (m_image && filename != "") {
			m_saveWindow = new Window(this, "Saving tonemapped image..");
			m_saveWindow->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Middle, 10, 10));
			m_saveWindow->setModal(true);
			m_saveWindow->setFixedWidth(300);
			m_saveWindow->setVisible(true);

			auto panel = new Widget(m_saveWindow);
			panel->setLayout(new BoxLayout(Orientation::Horizontal, Alignment::Middle, 10, 15));

			m_saveWindow->center();
			m_saveWindow->requestFocus();
			m_progressBar = new ProgressBar(panel);
			m_progressBar->setFixedWidth(200);

			performLayout(nvgContext());

			m_saveThread = new std::thread([&, filename]{
				m_image->saveAsPNG(filename, m_tonemapOperators[m_tonemapIndex], m_exposure, &m_progress);
			});
		}
	});

	setExposureMode(0);

	m_saveButton->setEnabled(false);
	m_exposureSelection->setEnabled(false);
	setEnabledRecursive(m_exposureWidget, false);
	m_tonemapPopupButton->setEnabled(false);
	setEnabledRecursive(m_tonemapWidget, false);

	performLayout(mNVGContext);

	drawAll();
	setVisible(true);
}

TonemapperScreen::~TonemapperScreen() {
	glDeleteTextures(1, &m_texture);
	for (size_t i = 0; i < m_tonemapOperators.size(); ++i) {
		delete m_tonemapOperators[i];
	}
}

void TonemapperScreen::setImage(const std::string &filename) {
	using namespace nanogui;
	if (m_image) {
		delete m_image;
		m_image = nullptr;
	}

	m_image = new Image(filename);

	if (m_image->getWidth() <= 0 || m_image->getHeight() <= 0) {
		delete m_image;
		m_image = nullptr;

		m_saveButton->setEnabled(false);
		m_exposureSelection->setEnabled(false);
		setEnabledRecursive(m_exposureWidget, false);
		m_tonemapPopupButton->setEnabled(false);
		setEnabledRecursive(m_tonemapWidget, false);

		return;
	}

	for (auto tm : m_tonemapOperators) {
		tm->setParameters(m_image);
	}

	m_saveButton->setEnabled(true);
	m_exposureSelection->setEnabled(true);
	setEnabledRecursive(m_exposureWidget, true);
	m_tonemapPopupButton->setEnabled(true);
	setEnabledRecursive(m_tonemapWidget, true);

	m_window->setPosition(Vector2i(15, 15));

	m_scaledImageSize = Vector2i(MAIN_WIDTH, (MAIN_WIDTH * m_image->getHeight()) / m_image->getWidth());
	m_windowSize = Vector2i(m_scaledImageSize.x(), m_scaledImageSize.y());

	setSize(m_windowSize);
	glfwSetWindowPos(glfwWindow(), 20, 40);

	glGenTextures(1, &m_texture);
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void TonemapperScreen::setTonemapMode(int index) {
	using namespace nanogui;

	m_tonemapIndex = index;

	MatrixXu indices(3, 2);
	indices.col(0) << 0, 1, 2;
	indices.col(1) << 2, 3, 0;

	MatrixXf positions(2, 4);
	positions.col(0) << 0, 0;
	positions.col(1) << 1, 0;
	positions.col(2) << 1, 1;
	positions.col(3) << 0, 1;

	m_tonemapOperators[m_tonemapIndex]->shader->bind();
	m_tonemapOperators[m_tonemapIndex]->shader->uploadIndices(indices);
	m_tonemapOperators[m_tonemapIndex]->shader->uploadAttrib("position", positions);

	if (m_tonemapLabel) {
		m_window->removeChild(m_tonemapLabel);
	}

	m_tonemapLabel = new Label(m_window, "Tonemapping operator", "sans-bold");

	if (m_tonemapPopupButton) {
		m_window->removeChild(m_tonemapPopupButton);
	}

	m_tonemapPopupButton = new PopupButton(m_window);
	m_popup = m_tonemapPopupButton->popup();
	m_popup->setWidth(220);
	auto scroll = new VScrollPanel(m_popup);
	scroll->setLayout(new BoxLayout(Orientation::Vertical));
	auto panel = new Widget(scroll);
	
	panel->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Fill, 10, 10));
	int newIndex = 0;
	for (auto tm: m_tonemapOperators) {
		auto *button = new Button(panel, tm->name);
		button->setFlags(Button::RadioButton);
		button->setCallback([&, newIndex] {
			m_popup->setVisible(false);
			setTonemapMode(newIndex);
		});
		newIndex++;
	}
	m_tonemapPopupButton->setCaption(m_tonemapOperators[m_tonemapIndex]->name);

	if (m_tonemapWidget) {
		m_window->removeChild(m_tonemapWidget);
	}

	m_tonemapWidget = new Widget(m_window);
	m_tonemapWidget->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Minimum, 0, 8));

	for (auto &parameter : m_tonemapOperators[m_tonemapIndex]->parameters) {
		auto &p = parameter.second;
		if (p.constant) continue;

		auto *windowPanel = new Widget(m_tonemapWidget);
		windowPanel->setLayout(new BoxLayout(Orientation::Horizontal, Alignment::Middle, 0, 20));

		auto button = new Button(windowPanel, parameter.first);
		button->setFixedSize(Vector2i(50, 22));
		button->setFontSize(15);

		auto *slider = new Slider(windowPanel);
		slider->setValue(inverseLerp(p.value, p.minValue, p.maxValue));

		auto textBox = new FloatBox<float>(windowPanel);
		textBox->setFixedSize(Vector2i(50, 22));
		textBox->numberFormat("%.2f");
		textBox->setFontSize(15);
		textBox->setValue(p.value);
		textBox->setAlignment(TextBox::Alignment::Right);
		textBox->setEditable(true);

		textBox->setCallback([&, slider, textBox](float v) {
			p.value = v;
			textBox->setValue(p.value);
			refreshGraph();
		});

		slider->setCallback([&, textBox](float t) {
			p.value = lerp(t, p.minValue, p.maxValue);
			textBox->setValue(p.value);
			refreshGraph();
		});

		button->setCallback([&, slider, textBox] {
			p.value = p.defaultValue;
			slider->setValue(inverseLerp(p.value, p.minValue, p.maxValue));
			textBox->setValue(p.defaultValue);
			refreshGraph();
		});
	}

	refreshGraph();

	performLayout(mNVGContext);
}

void TonemapperScreen::setExposureMode(int index) {
	using namespace nanogui;

	if (m_exposureLabel) {
		m_window->removeChild(m_exposureLabel);
	}

	m_exposureLabel = new Label(m_window, "Exposure mode", "sans-bold");

	if (m_exposureSelection) {
		m_window->removeChild(m_exposureSelection);
	}

	m_exposureSelection = new ComboBox(m_window, { "log2 Exposure", "Key value Exposure", "Auto Exposure" });
	m_exposureSelection->setCallback([&](int index) {
		setExposureMode(index);
	});
	m_exposureSelection->setSelectedIndex(index);

	if (m_exposureWidget) {
		m_window->removeChild(m_exposureWidget);
	}

	m_exposureWidget = new Widget(m_window);
	m_exposureWidget->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Minimum, 0, 10));

	auto *panel = new Widget(m_exposureWidget);
	panel->setLayout(new BoxLayout(Orientation::Horizontal, Alignment::Middle, 0, 20));
	
	Button *button;
	Slider *slider;
	FloatBox<float> *textBox;

	if (index == 0 || index == 1) {

		button = new Button(panel, "alpha");
		button->setFixedSize(Vector2i(50, 22));
		button->setFontSize(15);

		slider = new Slider(panel);

		textBox = new FloatBox<float>(panel);
		textBox->setFixedSize(Vector2i(50, 22));
		textBox->numberFormat("%.1f");
		textBox->setFontSize(15);
		textBox->setAlignment(TextBox::Alignment::Right);
		textBox->setEditable(true);
	}
	
	if (index == 0) {
		m_exposure = 1.f;
		slider->setValue(0.5f);
		textBox->setValue(0.f);

		textBox->setCallback([&, slider, textBox](float v) {
			textBox->setValue(v);
			m_exposure = std::pow(2.f, v);
		});

		slider->setCallback([&, textBox](float t) {
			float tmp = (t - 0.5f) * 20.f;
			m_exposure = std::pow(2.f, tmp);
			textBox->setValue(tmp);
		});

		button->setCallback([&, slider, textBox] {
			m_exposure = 1.f;
			slider->setValue(0.5f);
			textBox->setValue(0.f);
		});
	}
	else if (index == 1) {
		slider->setValue(0.18f);
		textBox->setValue(0.18f);
		m_exposure = 0.18f / m_image->getLogAverageLuminance();

		textBox->setCallback([&, slider, textBox](float v) {
			textBox->setValue(v);
			m_exposure = v / m_image->getLogAverageLuminance();
		});

		slider->setCallback([&, textBox](float t) {
			m_exposure = t / m_image->getLogAverageLuminance();
			textBox->setValue(t);
		});

		button->setCallback([&, slider, textBox] {
			m_exposure = 0.18f / m_image->getLogAverageLuminance();
			slider->setValue(0.18f);
		textBox->setValue(0.18f);
		});
	}
	else if (index == 2) {
		m_exposure = m_image->getAutoKeyValue() / m_image->getLogAverageLuminance();
	}

	setTonemapMode(m_tonemapIndex);
}

void TonemapperScreen::refreshGraph() {
	using namespace nanogui;

	if (m_graph) {
		m_window->removeChild(m_graph);
	}

	m_graph = new Graph(m_window, "luminance [0, 1]");
	m_graph->setFixedHeight(100);
	m_graph->setFooter("log luminance [-5, 5]");
	VectorXf &func = m_graph->values();
	int precision = 50;
	func.resize(precision);
	for (int i = 0; i < precision; ++i) {
		float t = (float)i / precision;
		Color3f color(std::pow(2.f, (t - 0.5f) * 10.f));
		func[i] = clamp(m_tonemapOperators[m_tonemapIndex]->map(color).getLuminance(), 0.f, 1.f);
	}

	performLayout(mNVGContext);
}

bool TonemapperScreen::keyboardEvent(int key, int scancode, int action, int modifiers) {
    if (Screen::keyboardEvent(key, scancode, action, modifiers))
        return true;
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        setVisible(false);
        return true;
    }
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
		m_window->setVisible(!m_window->visible());
		return true;
	}
    return false;
}

bool TonemapperScreen::dropEvent(const std::vector<std::string> & filenames) {
	if (filenames.size() > 0) {
		setImage(filenames[0]);
	}
	return true;
}

void TonemapperScreen::drawContents() {
	using namespace nanogui;

	if (m_image) {
		const Vector2i &imageSize = m_image->getSize();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, m_image->getWidth());
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, imageSize.x(), imageSize.y(), 0, GL_RGB, GL_FLOAT, (uint8_t *)m_image->getData());
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

		GLint x = (GLint) mPixelRatio * (mFBSize[0] - m_scaledImageSize[0]) / 2;
		GLint y = (GLint) mPixelRatio * (mFBSize[1] - m_scaledImageSize[1]) / 2;
		GLsizei width = (GLsizei) mPixelRatio*m_scaledImageSize[0];
		GLsizei height = (GLsizei) mPixelRatio*m_scaledImageSize[1];
		glViewport(x, y, width, height);

		m_tonemapOperators[m_tonemapIndex]->shader->bind();
		m_tonemapOperators[m_tonemapIndex]->shader->setUniform("source", 0);
		m_tonemapOperators[m_tonemapIndex]->shader->setUniform("exposure", m_exposure);

		for (auto &parameter : m_tonemapOperators[m_tonemapIndex]->parameters) {
			Parameter &p = parameter.second;
			m_tonemapOperators[m_tonemapIndex]->shader->setUniform(p.uniform, p.value);
		}

		m_tonemapOperators[m_tonemapIndex]->shader->drawIndexed(GL_TRIANGLES, 0, 2);

		x = (GLint) 0;
		y = (GLint) 0;
		width = (GLsizei) mFBSize[0];
		height = (GLsizei) mFBSize[1];
		glViewport(x, y, width, height);
	}
}

void TonemapperScreen::draw(NVGcontext *ctx) {

	if (m_progressBar) {
		m_progressBar->setValue(m_progress);

		if (m_progress < 0.f) {
			m_saveThread->join();
			delete m_saveThread;
			m_progressBar = nullptr;
			m_saveWindow->dispose();
		}
	}
	
	
	Screen::draw(ctx);
}

void TonemapperScreen::setEnabledRecursive(nanogui::Widget *widget, bool enabled) {
	widget->setEnabled(enabled);
	for (auto c : widget->children()) {
		setEnabledRecursive(c, enabled);
	}
}