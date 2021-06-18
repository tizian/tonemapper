#include <iostream>

#include <Gui.h>
#include <Image.h>
#include <Tonemap.h>
#include <nanogui/nanogui.h>

using namespace tonemapper;

int main(int argc, char **argv) {
    PRINT("=========================");
    PRINT(" Tonemapper v%s", VERSION);
    PRINT(" (c) %s Tizian Zeltner", YEAR);
    PRINT("=========================");

    std::vector<std::string> names;
    for (auto const& kv: *TonemapOperator::constructors) {
        names.push_back(kv.first);
    }
    std::sort(names.begin(), names.end());

    // Values to be set via command line arguments below

    bool showHelpText = false;

    TonemapOperator *tm = nullptr;

    ExposureMode exposureMode = ExposureMode::Value;
    float exposureInput = 0.f;

    std::vector<std::string> inputImages;
    bool saveAsJpg = true;

    bool useGUI = false;


    if (argc > 1) {
        std::string token(argv[1]);

        if (token.compare("--help") == 0) {
            showHelpText = true;
        } else {
            // Determine which operator should be used
            if (std::find(names.begin(), names.end(), token) != names.end()) {
                tm = TonemapOperator::create(token);
            } else {
                WARN("Unkown operator \"%s\"", token);
                showHelpText = true;
            }
        }
    }

    for (int i = 2; i < argc; ++i) {
        std::string token(argv[i]);
        std::string extension = fileExtension(token);

        if (token.compare("--help") == 0) {
            showHelpText = true;
            break;
        } else if (token.compare("--gui") == 0) {
            useGUI = true;
            break;
        } else if (token.compare("--exposure-value") == 0) {
            exposureMode = ExposureMode::Value;
            if (i+1 >= argc) {
                WARN("Parameter \"exposure-value\" expects a (float) value following it.");
                showHelpText = true;
                break;
            }
            exposureInput = atof(argv[i+1]);
            i++;
        } else if (token.compare("--exposure-key") == 0) {
            exposureMode = ExposureMode::Key;
            if (i+1 >= argc) {
                WARN("Parameter \"exposure-key\" expects a (float) value following it.");
                showHelpText = true;
                break;
            }
            exposureInput = atof(argv[i+1]);
            i++;
        } else if (token.compare("--exposure-auto") == 0) {
            exposureMode = ExposureMode::Auto;
        } else if (token.compare("--output-jpg") == 0) {
            saveAsJpg = true;
        } else if (token.compare("--output-png") == 0) {
            saveAsJpg = false;
        } else if (extension.compare("exr") == 0 ||
                   extension.compare("hdr") == 0) {
            inputImages.push_back(token);
        } else if (tm) {
            if (tm->parameters.find(token) != tm->parameters.end()) {
                // This is a valid parameter for the chosen operator
                if (i+1 >= argc) {
                    WARN("Parameter \"%s\" expects a (float) value following it.", token);
                    showHelpText = true;
                    break;
                }
                float value = atof(argv[i+1]);
                tm->parameters.at(token).value = value;
                i++;
            } else {
                WARN("Unkown option \"%s\"", token);
                showHelpText = true;
                break;
            }
        }
    }

    if (useGUI) {
        nanogui::ref<TonemapperGui> gui;
        nanogui::init();
        gui = new TonemapperGui();
        gui->draw_all();
        gui->set_visible(true);

        nanogui::mainloop(1 / 60.f * 1000);

        nanogui::shutdown();
    } else {

        if (tm == nullptr) {
            WARN("Need to specify one of the operators as the first argument.");
            showHelpText = true;
        } else if (inputImages.size() == 0) {
            WARN("Need to specify at least one (.exr or .hdr) input image.");
            showHelpText = true;
        }

        if (showHelpText) {
            PRINT("");
            PRINT("Usage:");
            PRINT("* Tonemap a set of images:");
            PRINT("    %s <operator> <options> <list of images (.exr or .hdr format)>", argv[0]);
            PRINT("* Get list of options for a specific operator:");
            PRINT("    %s <operator> --help", argv[0]);
            PRINT("* Open GUI:");
            PRINT("    %s --gui", argv[0]);
            PRINT("");
            if (tm == nullptr) {
                PRINT("List of available operators:");
                for (size_t i = 0; i < names.size(); ++i) {
                    PRINT("    \"%s\"", names[i]);
                }
            } else {
                PRINT("Chosen operator:");
                PRINT("    \"%s\"", tm->name);
                PRINT("    %s", tm->description);
            }
            PRINT("");
            PRINT("Available options:");
            PRINT("--exposure-value   Scale the input image with a factor of 2^Exposure.");
            PRINT("                   (Default: 0.0");
            PRINT("--exposure-key     Scale the input image with a key value as described in");
            PRINT("                   \"Photographic Tone Reproduction for Digital Images\" by");
            PRINT("                   Reinhard et al. 2002.");
            PRINT("                   (Default: 0.18");
            PRINT("--exposure-auto    Auto adjust the input image exposure as proposed in");
            PRINT("                   \"Perceptual Effects in Real-time Tone Mapping\" by");
            PRINT("                   Krawczyk et al. 2005.");
            PRINT("--output-jpg       Write output images in \".jpg\" format.");
            PRINT("--output-png       Write output images in \".png\" format.");
            if (tm != nullptr) {
                PRINT("");
                PRINT("Available operator specific options:");
                for (auto const &kv : tm->parameters) {
                    // Format parameter while potentially handling
                    // multi-line descriptions.
                    std::string name = kv.first;
                    std::string description = kv.second.description;
                    std::string buffer;
                    std::stringstream ss(description);

                    std::vector<std::string> tokens;
                    while (ss >> buffer) {
                        tokens.push_back(buffer);
                    }

                    const size_t maxWidth = 60;
                    size_t currentWidth = 0;
                    std::cout << tfm::format("--%-16s", name);
                    for (size_t i = 0; i < tokens.size(); ++i) {
                        size_t diff = tokens[i].size() + 1;
                        if (currentWidth + diff <= maxWidth) {
                            std::cout << " " << tokens[i];
                            currentWidth += diff;
                        } else {
                            std::cout << std::endl;
                            std::cout << "                  ";
                            currentWidth = 0;
                        }
                    }

                    std::cout << std::endl;
                    std::cout << "                  ";
                    std::cout << tfm::format(" (Default: %s)", kv.second.defaultValue);
                    std::cout << std::endl;
                }
                if (tm->parameters.size() == 0) {
                    PRINT("    None.");
                }
            }
            PRINT("");
        }


        if (tm) {
            for (size_t i = 0; i < inputImages.size(); ++i) {
                Image *img = Image::load(inputImages[i]);
                tm->preprocess(img);

                Image *out = new Image(img->getWidth(), img->getHeight());

                float exposure = 1.f;
                if (exposureMode == ExposureMode::Value) {
                    exposure = std::pow(2.f, exposureInput);
                } else if (exposureMode == ExposureMode::Key) {
                    /* See Eq. (1) in "Photographic Tone Reproduction for Digital Images"
                       by Reinhard et al. 2002. */
                    exposure = exposureInput / img->getLogMeanLuminance();
                } else {
                    /* See Eqs. (1) and (11) in "Perceptual Effects in Real-time Tone Mapping"
                       by Krawczyk et al. 2005. */
                    float alpha = 1.03f - 2.f / (2.f + std::log10(img->getLogMeanLuminance() + 1.f));
                    exposure = alpha / img->getLogMeanLuminance();
                }

                tm->process(img, out, exposure, nullptr);

                std::string outname = inputImages[i].substr(0, inputImages[i].size() - 4);
                if (saveAsJpg) {
                    outname += ".jpg";
                } else {
                    outname += ".png";
                }
                out->save(outname);

                delete img;
                delete out;
            }
            delete tm;
        }
    }

    return 0;
}
