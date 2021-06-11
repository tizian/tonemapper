#include <iostream>
#include <Gui.h>

#include <Image.h>

using namespace tonemapper;

int main(int argc, char **argv) {

    Image *img = Image::load(argv[1]);
    VARLOG2(img->getWidth(), img->getHeight());

    // img->save("out_jpg.jpg");

    // nanogui::ref<TonemapperGui> gui;
    // nanogui::init();
    // gui = new TonemapperGui();
    // gui->draw_all();
    // gui->set_visible(true);

    // nanogui::mainloop(1 / 60.f * 1000);

    // nanogui::shutdown();

    delete img;

    return 0;
}
