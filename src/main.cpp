#include <global.h>

#include <nanogui/glutil.h>
#include <nanogui/nanogui.h>
#include <filesystem/resolver.h>

#include <image.h>
#include <operator.h>
#include <operators/gamma.h>
#include <operators/reinhard.h>

using namespace std;

class TonemapperScreen : public nanogui::Screen {
public:
	TonemapperScreen()
		: nanogui::Screen(Eigen::Vector2i(1280, 960), "Tone Mapper", true, false) {

		using namespace nanogui;

		auto ctx = nvgContext();

		glfwSetWindowPos(glfwWindow(), 20, 60);
		setBackground(Vector3f(0.8f, 0.8f, 0.8f));

		m_window = new Window(this, "Tone Mapper");
		m_window->setPosition(Vector2i(15, 15));
		m_window->setLayout(new GroupLayout());

		auto about = new Button(m_window->buttonPanel(), "", ENTYPO_ICON_INFO);
		about->setCallback([&, ctx] {
			auto dlg = new MessageDialog(this, MessageDialog::Type::Information, "About Tone Mapper", "Info and description.\n\n(c) 2015 Tizian Zeltner");
			dlg->messageLabel()->setFixedWidth(300);
			dlg->messageLabel()->setFontSize(20);
			performLayout(ctx);
			dlg->center();
		});

		new Label(m_window, "Image I/O", "sans-bold");

		auto *openBtn = new Button(m_window, "Open HDR image");
		openBtn->setBackgroundColor(nanogui::Color(0, 255, 0, 25));
		openBtn->setIcon(ENTYPO_ICON_FOLDER);
		openBtn->setCallback([&] {
			std::string filename = file_dialog({ {"exr", "OpenEXR"} }, false);
			setImage(filename);
		});

		auto *saveBtn = new Button(m_window, "Save LDR image");
		saveBtn->setBackgroundColor(nanogui::Color(0, 255, 0, 25));
		saveBtn->setIcon(ENTYPO_ICON_SAVE);
		saveBtn->setCallback([&] {
			std::string filename = file_dialog({ { "png", "Portable Network Graphics" } }, true);
		});

		new Label(m_window, "Tonemapping operator", "sans-bold");

		m_tonemapSelection = new ComboBox(m_window, { "Gamma", "Reinhard" });
		setTonemapper(new GammaOperator());
		m_tonemapSelection->setCallback([&](int index) {
			ToneMappingOperator *op = nullptr;
			if (index == 0) {
				op = new GammaOperator();
			}
			else if (index == 1) {
				op = new ReinhardOperator();
			}

			if (op) {
				setTonemapper(op);
			}
		});

		drawAll();
		setVisible(true);
	}

	void setImage(const std::string &filename) {
		using namespace nanogui;
		if (m_image) {
			delete m_image;
			m_image = nullptr;
		}

		m_image = new Image(filename);

		if (m_image->getWidth() <= 0 || m_image->getHeight() <= 0) {
			delete m_image;
			m_image = nullptr;
			return;
		}

		m_window->setPosition(Vector2i(15, 15));

		m_scaledImageSize = Vector2i(MAIN_WIDTH, (MAIN_WIDTH * m_image->getHeight()) / m_image->getWidth());
		m_windowSize = Vector2i(m_scaledImageSize.x(), m_scaledImageSize.y());

		setSize(m_windowSize);
		glfwSetWindowPos(glfwWindow(), 20, 100);

		glGenTextures(1, &m_texture);
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}

	void setTonemapper(ToneMappingOperator *tonemap) {
		using namespace nanogui;

		if (m_tonemap) {
			delete m_tonemap;
			m_tonemap = nullptr;
		}

		m_tonemap = tonemap;

		MatrixXu indices(3, 2);
		indices.col(0) << 0, 1, 2;
		indices.col(1) << 2, 3, 0;

		MatrixXf positions(2, 4);
		positions.col(0) << 0, 0;
		positions.col(1) << 1, 0;
		positions.col(2) << 1, 1;
		positions.col(3) << 0, 1;

		m_tonemap->shader->bind();
		m_tonemap->shader->uploadIndices(indices);
		m_tonemap->shader->uploadAttrib("position", positions);

		if (m_paramWidget) {
			m_window->removeChild(m_paramWidget);
		}

		m_paramWidget = new Widget(m_window);
		m_paramWidget->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Minimum, 0, 10));

		for (auto &parameter : m_tonemap->parameters) {
			auto &p = parameter.second;

			new Label(m_paramWidget, parameter.first, "sans-bold");

			auto *panel = new Widget(m_paramWidget);
			panel->setLayout(new BoxLayout(Orientation::Horizontal, Alignment::Middle, 0, 20));

			auto toolButton = new ToolButton(panel, ENTYPO_ICON_CYCLE);
			toolButton->setFlags(Button::NormalButton);

			auto *slider = new Slider(panel);
			slider->setValue(inverseLerp(p.defaultValue, p.minValue, p.maxValue));
			// slider->setFixedWidth(SIDE_WIDTH - 160);

			auto textBox = new FloatBox<float>(panel);
			textBox->setFixedSize(Vector2i(60, 25));
			textBox->setValue(p.defaultValue);
			textBox->setAlignment(TextBox::Alignment::Right);
			textBox->setEditable(true);
			textBox->numberFormat("%.1f");

			textBox->setCallback([&](float v) {
				p.value = v;
			});

			slider->setCallback([&, textBox](float t) {
				p.value = lerp(t, p.minValue, p.maxValue);
				textBox->setValue(p.value);
			});

			toolButton->setCallback([&, slider, textBox] {
				p.value = p.defaultValue;
				slider->setValue(inverseLerp(p.value, p.minValue, p.maxValue));
				textBox->setValue(p.defaultValue);
			});
		}

		// sidebar->setSize(Vector2i(SIDE_WIDTH, m_scaledImageSize.y()));
		performLayout(mNVGContext);
	}

	virtual bool keyboardEvent(int key, int scancode, int action, int modifiers) {
        if (Screen::keyboardEvent(key, scancode, action, modifiers))
            return true;
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
            setVisible(false);
            return true;
        }
        return false;
    }

	~TonemapperScreen() {
		glDeleteTextures(1, &m_texture);
		delete m_tonemap;
	}

	virtual void drawContents() {
		using namespace nanogui;

		if (m_image) {
			const Vector2i &imageSize = m_image->getSize();

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, m_texture);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, m_image->getWidth());
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, imageSize.x(), imageSize.y(), 0, GL_RGB, GL_FLOAT, (uint8_t *)m_image->getData());
			glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
			glViewport(mPixelRatio * (mFBSize[0]-m_scaledImageSize[0])/2, mPixelRatio * (mFBSize[1] - m_scaledImageSize[1]) / 2, mPixelRatio*m_scaledImageSize[0], mPixelRatio*m_scaledImageSize[1]);
			m_tonemap->shader->bind();
			m_tonemap->shader->setUniform("source", 0);

			for (auto &parameter : m_tonemap->parameters) {
				Parameter &p = parameter.second;
				m_tonemap->shader->setUniform(p.uniform, p.value);
			}

			m_tonemap->shader->drawIndexed(GL_TRIANGLES, 0, 2);
			glViewport(0, 0, mFBSize[0], mFBSize[1]);
		}
		
	}

private:
	Image *m_image = nullptr;
    uint32_t m_texture = 0;

    nanogui::Window *m_window = nullptr;
	nanogui::ComboBox *m_tonemapSelection = nullptr;
	nanogui::Widget *m_paramWidget = nullptr;

	ToneMappingOperator *m_tonemap = nullptr;

    const int MAIN_WIDTH = 960;
	
	Eigen::Vector2i m_windowSize;
	Eigen::Vector2i m_scaledImageSize;
};

int main(int argc, char *argv[])
{
	nanogui::init();

	TonemapperScreen *screen = new TonemapperScreen();

	nanogui::mainloop();

	delete screen;

	nanogui::shutdown();

	return 0;
}