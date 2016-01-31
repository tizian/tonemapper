/*
    src/main.cpp -- Tone Mapper main function
    
    Copyright (c) 2016 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license. 
*/

#include <global.h>

#include <gui.h>

#ifdef WIN32
	#include <windows.h>
#endif

int main(int argc, char *argv[])
{
	#ifndef DEBUG
	#ifdef WIN32
		HWND hWnd = GetConsoleWindow();
		ShowWindow(hWnd, SW_HIDE);
	#endif
	#endif

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