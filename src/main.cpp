#include <global.h>

#include <gui.h>

int main(int argc, char *argv[])
{
	nanogui::init();
	TonemapperScreen *screen = new TonemapperScreen();
	nanogui::mainloop();
	nanogui::shutdown();
	delete screen;

	return 0;
}