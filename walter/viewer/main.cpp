// Copyright 2017 Rodeo FX.  All rights reserved.

#include "FreeCamera.h"
#include "Scene.h"
#include "Types.h"
#include "View.h"

#include <cstring>
#include <iostream>

void help(const char* progName, int status)
{
    (status == EXIT_SUCCESS ? std::cout : std::cerr)
        << "Basic application to display Walter layers in a 3d viewer\n"
        << "Usage: " << progName << " file.usd [file.usd ...] [options]\n"
        << "Require: walter layers:\n"
        << "     A list of files supported by Walter (.usd, .usda, .abc\n"
        << "Options:\n"
        << "    -h, -help          Print this usage message and exit\n"
        << "    -c, -complexity    Set geometry complexity from 1.0 to 2.0 "
           "(default 1.0)\n"
        << "    -f, -frame         The frame to view (default 1)\n"
        << "    -o, -output        Export the stage instead of viewing it\n"
        << "    -r, -renderer      Set the renderer to 'Embree' or 'Stream' "
           "(default 'Stream')\n"
        << "\n"
        << "Shortcuts:\n"
        << "\tf: Frame the whole scene\n"
        << "\tr: Reset camera\n"
        << "\te: Set Hydra renderer to use Embree (scene is rendered on CPU "
           "and the result is sent to GPU for display)\n"
        << "\ts: Set Hydra renderer to use Stream (whole scene is sent to GPU "
           "and "
           "rendered by OpenGL)\n"
        << "\tw: Switch between wireframe and shaded\n"
        << "\t+: Increment subdiv\n"
        << "\t-: Decrement subdiv\n"
        << "\n";

    exit(status);
}

namespace walter_viewer
{

void exec(const std::string& output, const Scene::Options& opt)
{
    if(output != "")
    {
        auto scene = std::make_shared<Scene>(opt);
        scene->Export(output);  
    }
    else
    {
        auto view = getView();
        if (!view->isValid())
        {
            exit(EXIT_FAILURE);
        }

        view->setScene(std::make_shared<Scene>(
            opt, std::make_shared<FreeCamera>()));
        view->show();
        view->refresh();        
    }
}

} // end namespace walter_viewer

int main(int argc, char* argv[])
{
    const char* progName = argv[0];
    if (const char* ptr = ::strrchr(progName, '/'))
    {
        progName = ptr + 1;
    }

    // Parse the command line.
    walter_viewer::Scene::Options opt;
    std::string layers, output;
    if (argc == 1)
    {
        help(progName, EXIT_FAILURE);
    }

    for (int n = 1; n < argc; ++n)
    {
        std::string str(argv[n]);

        if (str == "-h" || str == "-help" || str == "--help")
        {
            help(progName, EXIT_SUCCESS);
        }

        else if (str == "-c" || str == "-complexity")
        {
            opt.complexity = std::stof(argv[n + 1]);
            ++n;
            continue;
        }
        else if (str == "-f" || str == "-frame")
        {
            opt.frame = std::stof(argv[n + 1]);
            ++n;
            continue;
        }
        else if (str == "-o" || str == "-output")
        {
            output = argv[n + 1];
            ++n;
            continue;
        }        
        else if (str == "-r" || str == "-renderer")
        {
            opt.renderer = argv[n + 1];
            ++n;
            continue;
        }

        if (str[0] != '-')
        {
            opt.filePath += str;
            if (n < argc - 1)
            {
                opt.filePath += ":";
            }
        }
    }

    walter_viewer::exec(output, opt);

    exit(EXIT_SUCCESS);
}
