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
		
		m_tonemap = new ReinhardOperator(); // TODO: select with GUI

		using namespace nanogui;

		glfwSetWindowPos(glfwWindow(), 20, 60);
		setBackground(Vector3f(0.8f, 0.8f, 0.8f));

		auto *window = new Window(this, "Tone Mapper");
		window->setPosition(Vector2i(15, 15));
		window->setLayout(new GroupLayout());

		auto *openBtn = new Button(window, "Open image");
		//openBtn->setBackgroundColor(nanogui::Color(Vector3f(0.3f, 0.1f, 0.5f)));
		openBtn->setIcon(ENTYPO_ICON_FOLDER);
		openBtn->setCallback([&] {
			std::string filename = file_dialog({ {"exr", "OpenEXR"} }, false);
			setImage(filename);
		});

		auto label = new Label(window, "Parameters", "sans-bold");
		label->setFontSize(24.f);

		for (auto &parameter : m_tonemap->parameters) {
			auto &p = parameter.second;

			new Label(window, parameter.first, "sans-bold");

			auto *panel = new Widget(window);
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

		drawAll();
		setVisible(true);
	}

	void setImage(const std::string &filename) {
		using namespace nanogui;
		if (m_image) {
			delete m_image;
		}

		m_image = new Image(filename);

		m_scaledImageSize = Vector2i(MAIN_WIDTH, (MAIN_WIDTH * m_image->getHeight()) / m_image->getWidth());
		m_windowSize = Vector2i(m_scaledImageSize.x(), m_scaledImageSize.y());

		setSize(m_windowSize);
		setBackground(Vector3f(43 / 255.f, 43 / 255.f, 43 / 255.f));
		glfwSetWindowPos(glfwWindow(), 20, 100);

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

		glGenTextures(1, &m_texture);
		glBindTexture(GL_TEXTURE_2D, m_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
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

	ToneMappingOperator *m_tonemap;
    
    float m_exposure = 1.f;
    float m_gamma = 2.2f;

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