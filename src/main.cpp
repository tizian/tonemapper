/*
    Copyright (c) 2022 Tizian Zeltner

    Tone Mapper is provided under the MIT License.
    See the LICENSE.txt file for the conditions of the license.
*/

#include <iostream>

#include <Global.h>
#include <Image.h>
#include <Tonemap.h>

#ifdef TONEMAPPER_BUILD_GUI
    #include <Gui.h>
    #include <nanogui/nanogui.h>
#endif

#include <filesystem>

using namespace tonemapper;

void printUsage(char **argv) {
    PRINT("");
    PRINT("Usage:");
#ifdef TONEMAPPER_BUILD_GUI
    PRINT("* Open GUI:");
    PRINT("    tonemapper");
    PRINT("* Tonemap a list of images (without GUI):");
    PRINT("    tonemapper --no-gui <options> <list of images (.exr or .hdr format)>");
#else
    PRINT("* Tonemap a list of images:");
    PRINT("    tonemapper <options> <list of images (.exr or .hdr format)>");
#endif
    PRINT("* Get more information:");
    PRINT("    tonemapper --help");
    PRINT("");
}

void printHelp(const TonemapOperator *tm) {
    PRINT("");
    PRINT("Available options:");
    PRINT("  --exposure-value  Scale the input image with a factor of 2^Exposure.");
    PRINT("                    (Default: 0.0)");
    PRINT("");
    PRINT("  --exposure-key    Scale the input image with a key value as described in");
    PRINT("                    \"Photographic Tone Reproduction for Digital Images\" by");
    PRINT("                    Reinhard et al. 2002.");
    PRINT("                    (Default: 0.18)");
    PRINT("");
    PRINT("  --exposure-auto   Auto adjust the input image exposure as proposed in");
    PRINT("                    \"Perceptual Effects in Real-time Tone Mapping\" by");
    PRINT("                    Krawczyk et al. 2005.");
    PRINT("");
    PRINT("  --output-jpg      Write output images in \".jpg\" format.");
    PRINT("");
    PRINT("  --output-png      Write output images in \".png\" format.");
#ifdef TONEMAPPER_BUILD_GUI
    PRINT("");
    PRINT("  --no-gui          Do not open the GUI.");
#endif
    PRINT("");

    std::vector<std::string> operatorNames = TonemapOperator::orderedNames();

    if (tm == nullptr) {
        PRINT("List of available operators:");
        for (size_t i = 0; i < operatorNames.size(); ++i) {
            if (operatorNames[i].compare("") == 0) {
                PRINT("");
            } else {
                PRINT("    \"%s\"", operatorNames[i]);
            }
        }
    } else {
        PRINT("Chosen operator:");
        PRINT("    \"%s\"", tm->name);
        printMultiline(tm->description, 60, 4);
        PRINT("");
    }

    if (tm != nullptr) {
        PRINT("");
        PRINT("Operator specific parameters:");
        size_t indentation = 0;
        for (auto const &kv : tm->parameters) {
            indentation = std::max(indentation, kv.first.size());
        }
        if (tm->dataDriven) {
            indentation = std::max(indentation, std::string("file").size());
        }
        indentation += 6;

        for (auto const &kv : tm->parameters) {
            std::string param = "  --" + kv.first + "  ";
            printMultiline(kv.second.description, 60, indentation, param);
            PRINT("\n%s(Default: %s)\n", std::string(indentation, ' '), kv.second.defaultValue);
        }
        if (tm->dataDriven) {
            std::string param = "  --file  ";
            printMultiline("Path to a response function file.", 60, indentation, param);
        }
        if (tm->parameters.size() == 0) {
            PRINT("  None.");
        }
        PRINT("");
    }
}

int main(int argc, char **argv) {
    PRINT("=========================");
    PRINT(" tonemapper v%s", VERSION);
    PRINT(" (c) %s Tizian Zeltner", YEAR);
    PRINT("=========================");

    std::vector<std::string> operatorNames = TonemapOperator::orderedNames();

    std::vector<std::string> inputImages;
    std::vector<std::string> additionalTokens;
    TonemapOperator *tm       = nullptr;
    ExposureMode exposureMode = ExposureMode::Value;
    float exposureInput       = 0.f;
    bool saveAsJpg            = true;
    bool openGUI              = true;

    bool showHelp             = false;
    std::string operatorKey;
    std::string rfFilename;

    std::vector<std::string> warnings;
    std::vector<std::string> warningsGUI;

    for (int i = 1; i < argc; ++i) {
        std::string token(argv[i]);
        std::string extension = std::filesystem::path(token).extension();

        if (token.compare("--help") == 0) {
            showHelp = true;
            openGUI  = false;
        } else if (token.compare("--no-gui") == 0) {
            openGUI = false;
        } else if (token.compare("--exposure-value") == 0) {
            exposureMode = ExposureMode::Value;
            if (i + 1 >= argc) {
                warnings.push_back("Parameter \"exposure-value\" expects a float value following it.");
            } else {
                exposureInput = atof(argv[i + 1]);
                i++;
            }
        } else if (token.compare("--exposure-key") == 0) {
            exposureMode = ExposureMode::Key;
            if (i + 1 >= argc) {
                warnings.push_back("Parameter \"exposure-key\" expects a float value following it.");
            } else {
                exposureInput = atof(argv[i + 1]);
                i++;
            }
        } else if (token.compare("--exposure-auto") == 0) {
            exposureMode = ExposureMode::Auto;
        } else if (token.compare("--output-jpg") == 0) {
            saveAsJpg = true;
        } else if (token.compare("--output-png") == 0) {
            saveAsJpg = false;
        } else if (token.compare("--operator") == 0) {
            // Determine which operator should be used
            if (i + 1 >= argc) {
                warnings.push_back("Parameter \"operator\" expects a string following it.");
            } else {
                std::string operatorName = argv[i + 1];
                i++;
                if (std::find(operatorNames.begin(), operatorNames.end(), operatorName) != operatorNames.end()) {
                    tm = TonemapOperator::create(operatorName);
                    operatorKey = operatorName;
                } else {
                    warnings.push_back("Unknown operator \"" + operatorName + "\"");
                }
            }

        } else if (extension.compare(".exr") == 0 ||
                   extension.compare(".hdr") == 0) {
            if (std::filesystem::exists(token)) {
                inputImages.push_back(token);
            } else {
                std::string warning = "Specified input file \"" + token + "\" does not exist.";
                warnings.push_back(warning);
                warningsGUI.push_back(warning);
            }
        } else {
            additionalTokens.push_back(token);
        }
    }

#ifdef TONEMAPPER_BUILD_GUI
    if (openGUI) {
        if (warningsGUI.size() > 0) {
            PRINT("");
            WARN("%s", warnings[0]);
            PRINT("");
            return -1;
        }

        nanogui::ref<TonemapperGui> gui;
        nanogui::init();
        gui = new TonemapperGui();
        gui->draw_all();
        gui->set_visible(true);
        if (inputImages.size() > 0) {
            gui->setImage(inputImages[0]);
        }
        if (tm) {
            gui->setTonemapOperator(operatorKey);
        }

        nanogui::mainloop(1 / 60.f);
        nanogui::shutdown();

        return 0;
    }
#endif

    if (!tm) {
        warnings.push_back("Need to specify one tonemapping operator via the \"operator\" option.");
    }
    if (inputImages.size() == 0) {
        warnings.push_back("Need to specify at least one (.exr or .hdr) input image.");
    }

    if (tm) {
        int len = additionalTokens.size();
        for (int i = 0; i < len; ++i) {
            std::string token = additionalTokens[i];
            if (token.compare(0, 2, "--") != 0 || token.length() < 3) {
                warnings.push_back("Operator parameter \"" + token + "\" has wrong formatting. (Too short or no proceeding \"--\")");
            } else {
                std::string param = token.substr(2, token.length() - 1);
                if (tm->parameters.find(param) != tm->parameters.end()) {
                    if (i + 1 >= len) {
                        warnings.push_back("Operator parameter \"" + token + "\" expects a float value following it.");
                    } else {
                        float value = atof(additionalTokens[i + 1].c_str());
                        tm->parameters.at(param).value = value;
                        i++;
                    }
                } else if (tm->dataDriven && param.compare("file") == 0) {
                    if (i + 1 >= len) {
                        warnings.push_back("Operator parameter \"" + token + "\" expects a string following it.");
                    } else {
                        rfFilename = additionalTokens[i + 1];
                        tm->fromFile(rfFilename);
                        i++;
                    }
                } else {
                    warnings.push_back("Unkonwn option \"" + token + "\"");
                }
            }
        }
    }

    if (tm && tm->dataDriven && rfFilename.compare("") == 0) {
        warnings.push_back("Operator \"" + operatorKey + "\" requires a filepath (provided via \"--file\") to work.");
    }

    if (tm && tm->dataDriven && tm->irradiance.size() == 0) {
        warnings.push_back("");
    }

    if (showHelp) {
        printUsage(argv);
        printHelp(tm);
        PRINT("");
        if (warnings.size() > 0) {
            return 0;
        }
    }

    if (warnings.size() > 0) {
        printUsage(argv);
        printHelp(tm);
        PRINT("");
        WARN("%s", warnings[0]);
        PRINT("");
        return -1;
    }
    PRINT("");

    PRINT("* Chosen operator: \"%s\"", tm->name);
    PRINT("* Parameters:");
    size_t maxLength = 0;
    for (auto &parameter : tm->parameters) {
        maxLength = std::max(maxLength, parameter.first.size());
    }
    if (tm->dataDriven) {
        maxLength = std::max(maxLength, std::string("file").size());
    }
    for (auto &parameter : tm->parameters) {
        auto &p = parameter.second;
        if (p.constant) continue;
        size_t spaces = maxLength - parameter.first.size() + 1;
        PRINT("    %s%s= %.3f", parameter.first, std::string(spaces, ' '), p.value);
    }
    if (tm->dataDriven) {
        size_t spaces = maxLength - std::string("file").size() + 1;
        PRINT("    %s%s= %.3f", "file", std::string(spaces, ' '), rfFilename);
    }
    PRINT("");

    for (size_t i = 0; i < inputImages.size(); ++i) {
        PRINT_("* Read \"%s\" .. ", inputImages[i]);
        Image *img = Image::load(inputImages[i]);
        PRINT("done.");
        tm->preprocess(img);

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

        Image *out = new Image(img->getWidth(), img->getHeight());

        PRINT_("  Processing %d x %d pixels, exposure = %.2f .. ", img->getWidth(), img->getHeight(), exposure);
        tm->process(img, out, exposure);
        PRINT("done.");

        std::string outname = inputImages[i].substr(0, inputImages[i].size() - 4);
        if (saveAsJpg) {
            outname += ".jpg";
        } else {
            outname += ".png";
        }
        PRINT_("  Save \"%s\" .. ", outname);
        out->save(outname);
        PRINT("done.");


        delete img;
        delete out;
    }
    delete tm;

    PRINT("");

    return 0;
}
