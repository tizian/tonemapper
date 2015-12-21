#include <iostream>

#include <nanogui/glutil.h>
#include <nanogui/nanogui.h>
#include <filesystem/resolver.h>

#include <image.h>

using namespace std;

class TonemapperScreen : public nanogui::Screen {
public:
	TonemapperScreen(Image *image)
		: nanogui::Screen(Eigen::Vector2i(640, 480), "Tonemapper"), m_image(image) {
		using namespace nanogui;

		glfwSetWindowPos(glfwWindow(), 20, 100);

		m_scaledImageSize = Vector2i(MAIN_WIDTH, (MAIN_WIDTH * image->getHeight()) / image->getWidth());
		m_windowSize = Vector2i(m_scaledImageSize.x() + SIZE_WIDTH, m_scaledImageSize.y());

		setSize(m_windowSize);

		glfwSetWindowPos(glfwWindow(), 20, 100);

	    Widget *sidebar = new Widget(this);

	    sidebar->setLayout(new BoxLayout(Orientation::Vertical, Alignment::Middle, 20, 20));
	    new Label(sidebar, "Exposure value: ", "sans-bold");

	    Widget *panel = new Widget(sidebar);
	    panel->setLayout(new BoxLayout(Orientation::Horizontal, Alignment::Middle, 0, 20));

	    Slider *slider = new Slider(panel);
	    slider->setValue(0.5f);
	    slider->setFixedWidth(SIZE_WIDTH-120);

	    FloatBox<float> *textBox = new FloatBox<float>(panel);
	    textBox->setFixedSize(Vector2i(60, 25));
	    textBox->setValue(1.0);
	    textBox->setAlignment(TextBox::Alignment::Right);
	    textBox->setEditable(true);
	    textBox->numberFormat("%.1f");

	    textBox->setCallback([&](float value) {
	    	m_scale = value;
	    	return true;
	    });

	    slider->setCallback([&,textBox](float value) {
	    	m_scale = std::pow(2.f, (value - 0.5f) * 20);
	    	textBox->setValue(m_scale);
	    });

	    new Label(sidebar, "Gamma value: ", "sans-bold");

	    slider = new Slider(sidebar);
	    slider->setValue(1.f/2.f);
	    slider->setFixedWidth(SIZE_WIDTH-2*20);
	    slider->setCallback(
	        [&](float value) {
	            m_gamma = 10 * value;
	        }
	    );

	    sidebar->setSize(Vector2i(SIZE_WIDTH, m_scaledImageSize.y()));
    	performLayout(mNVGContext);

	    sidebar->setPosition(Vector2i(0, 0));

	    m_shader = new GLShader();
	    m_shader->init(
	        "Tonemapper",


	        "#version 330\n"
	        "in vec2 position;\n"
	        "out vec2 uv;\n"
	        "void main() {\n"
	        "    gl_Position = vec4(position.x*2-1, position.y*2-1, 0.0, 1.0);\n"
	        "    uv = vec2(position.x, 1-position.y);\n"
	        "}",


	        "#version 330\n"
	        "uniform sampler2D source;\n"
	        "uniform float scale;\n"
	        "uniform float gamma;\n"
	        "in vec2 uv;\n"
	        "out vec4 out_color;\n"
	        "float correct(float value) {\n"
	        "    return pow(value, 1/gamma);\n"
	        "}\n"
	        "void main() {\n"
	        "    vec4 color = scale * texture(source, uv);\n"
	        "    out_color = vec4(correct(color.r), correct(color.g), correct(color.b), 1);\n"
	        "}"
	    );

	    MatrixXu indices(3, 2);
	    indices.col(0) << 0, 1, 2;
	    indices.col(1) << 2, 3, 0;

	    MatrixXf positions(2, 4);
	    positions.col(0) << 0, 0;
	    positions.col(1) << 1, 0;
	    positions.col(2) << 1, 1;
	    positions.col(3) << 0, 1;

	    m_shader->bind();
	    m_shader->uploadIndices(indices);
	    m_shader->uploadAttrib("position", positions);

	    glGenTextures(1, &m_texture);
	    glBindTexture(GL_TEXTURE_2D, m_texture);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	    drawAll();
	    setVisible(true);
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
    	delete m_shader;
	}

	virtual void drawContents() {
		using namespace nanogui;

		const Vector2i &imageSize = m_image->getSize();

	    glActiveTexture(GL_TEXTURE0);
	    glBindTexture(GL_TEXTURE_2D, m_texture);
	    glPixelStorei(GL_UNPACK_ROW_LENGTH, m_image->getWidth());
	    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, imageSize.x(), imageSize.y(), 0, GL_RGB, GL_FLOAT, (uint8_t *) m_image->getData());
	    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	    glViewport(SIZE_WIDTH * mPixelRatio, 0, mPixelRatio*m_scaledImageSize[0], mPixelRatio*m_scaledImageSize[1]);
	    m_shader->bind();
	    m_shader->setUniform("scale", m_scale);
	    m_shader->setUniform("gamma", m_gamma);
	    m_shader->setUniform("source", 0);
	    m_shader->drawIndexed(GL_TRIANGLES, 0, 2);
	    glViewport(0, 0, mFBSize[0], mFBSize[1]);
	}

private:
	Image *m_image;
	nanogui::GLShader *m_shader = nullptr;
    uint32_t m_texture = 0;
    
    float m_scale = 1.f;
    float m_gamma = 2.2f;

    const int MAIN_WIDTH = 960;
	const int SIZE_WIDTH = 400;
	
	Eigen::Vector2i m_windowSize;
	Eigen::Vector2i m_scaledImageSize;
};

int main(int argc, char *argv[])
{
	if (argc < 2) {
        std::cerr << "Syntax: " << argv[0] << " <image.exr>" << std::endl;
        return -1;
    }

    filesystem::path path(argv[1]);

    if (path.extension() == "exr") {
        Image image(argv[1]);

        nanogui::init();

        TonemapperScreen *screen = new TonemapperScreen(&image);

        nanogui::mainloop();

        delete screen;

        nanogui::shutdown();
    }
    else {
        std::cerr << "Fatal error: unknown file \"" << argv[1] << "\", expected an extension of type .exr" << std::endl;
    }

	return 0;
}