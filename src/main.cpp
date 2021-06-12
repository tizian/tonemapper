#include <iostream>

#include <Gui.h>
#include <Image.h>
#include <Tonemap.h>
#include <nanogui/nanogui.h>

using namespace tonemapper;

int main(int argc, char **argv) {
    std::vector<std::string> names;
    for (auto const& kv: *TonemapOperator::constructors) {
        names.push_back(kv.first);
    }
    std::sort(names.begin(), names.end());

    std::cout << "Available operators" << std::endl;
    for (size_t i = 0; i < names.size(); ++i) {
        std::cout << "    " << names[i] << std::endl;
    }
    std::cout << std::endl;

    // TonemapOperator *tm = TonemapOperator::create("linear");

    // Image *img = Image::load(argv[1]);
    // VARLOG2(img->getWidth(), img->getHeight());

    // tm->preprocess(img);

    // Image *out = new Image(img->getWidth(), img->getHeight());

    // tm->process(img, out, 1.f, nullptr);

    // out->save("out_jpg.jpg");

    // delete tm;
    // delete img;
    // delete out;

    nanogui::ref<TonemapperGui> gui;
    nanogui::init();
    gui = new TonemapperGui();
    gui->draw_all();
    gui->set_visible(true);

    nanogui::mainloop(1 / 60.f * 1000);

    nanogui::shutdown();


    return 0;
}
