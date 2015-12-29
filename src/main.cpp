#include <global.h>

#include <gui.h>

int main(int argc, char *argv[])
{
	try {
        nanogui::init();

        {
            nanogui::ref<TonemapperScreen> app = new TonemapperScreen();
            app->drawAll();
            app->setVisible(true);
            nanogui::mainloop();
        }

        nanogui::shutdown();
    } catch (const std::runtime_error &e) {
        std::string error_msg = std::string("Caught a fatal error: ") + std::string(e.what());
		std::cerr << error_msg << endl;
        return -1;
    }

    return 0;
}