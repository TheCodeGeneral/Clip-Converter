#include <iostream>
#include <vector>
#include <string>
#include <string.h>
#include <regex>
#include <fstream>
#include <filesystem>
#include <iomanip>

namespace fs = std::filesystem;

void ffmpeg(std::string &rp_config, std::string export_dir, std::string extension);
void print_help();
int main(int argc, char** argv)
{
    std::string rpConfPath{};
    std::string export_path{};
    std::string extension{};
    switch(argc)
    {
        case 1:
            print_help();
            exit(1);
            break;
        case 4:
            rpConfPath = argv[1];
            if(strncmp("-o", argv[2],2) == 0 || strncmp("--output", argv[2], 8) == 0)
            {
                export_path = argv[3];
                extension = "mp3";
            }
            else
            {
                print_help();
                exit(-1);
            }
            break;
        case 6:
            rpConfPath = argv[1];
            if(strncmp("-o", argv[2], 2) == 0 || strncmp("--output", argv[2], 8) == 0)
            {
                export_path = argv[3];
                if(strncmp("-e", argv[4], 2) == 0 || strncmp("--extension", argv[4], 11) == 0)
                    extension = argv[5];
                else
                {
                    std::cout << "Invalid Usage" << std::endl;
                    print_help();
                    exit(-1);
                }
            }
            else if(strncmp("-e", argv[2], 2) == 0 || strncmp("--extension", argv[2], 11) == 0)
            {
                extension = argv[3];
                if(strncmp("-o", argv[4], 2) == 0 || strncmp("--output", argv[4], 8) == 0)
                    export_path = argv[5];
                else
                {
                    std::cout << "Invalid Usage" << std::endl;
                    print_help();
                    exit(-1);
                }
            }
            else
            {
                std::cout << "Invalid Usage" << std::endl;
                print_help();
                exit(-1);
            }
            break;
        default:
            std::cout << "Invalid Usage" << std::endl;
            print_help();
            exit(-1);
    }

    std::ifstream t(rpConfPath);
    std::string rp_config((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
    // Verify valid extension
    // Verify valid directory
    ffmpeg(rp_config, export_path, extension);
}
/*
Example 
1\path=C:/Users/User/Desktop/scav5_enemy_contact_25_bl.wav
1\customText=
1\customColor=ffffff
1\volume=-6
1\cropEnabled=false
1\cropStartValue=1
1\cropStartUnit=1
1\cropStopAfterAt=0
1\cropStopValue=0
1\cropStopUnit=1
*/

void print_help()
{
    int descSpacing = 40;
    std::cout << "Usage: clip_converter <rp_soundboard path> -o <output path> [OPTIONS]...\n\n";
    std::cout << "Options:\n";
    std::cout << std::left << std::setw(descSpacing) << "  -o, --output <directory>" << "Output directory\n";
    std::cout << std::left << std::setw(descSpacing) << "  -e, --extention [extension]" << "Converted file format (default=mp3)\n";
    std::cout << std::left << std::setw(descSpacing) << "  -h, --help" << "Print this help message" << std::endl;
}

void ffmpeg(std::string &rp_config, std::string export_dir, std::string extension)  
{
    std::string clipPath{};
    std::string customText{};
    bool cropEnabled;
    char cropStartUnit; // 0 = milliseconds 1 = seconds
    char cropStopUnit; // 0 = milliseconds 1 = seconds
    char cropStopAfterOrAt; // 0 = after 1 = seconds at
    std::string cropStart;
    std::string cropStop;

    std::regex re("(?:.+path=(.+)\\n.+customText=(.*)\\n.+\\n.+\\n.+cropEnabled=(.+)\\n.+cropStartValue=(\\d+)\\n.+cropStartUnit=(\\d)\\n.+cropStopAfterAt=(\\d)\\n.+cropStopValue=(\\d+)\\n.+cropStopUnit=(\\d))");
    for(std::sregex_iterator i = std::sregex_iterator(rp_config.begin(), rp_config.end(), re); i != std::sregex_iterator(); ++i )
    {
        std::smatch m = *i;
        clipPath = m[1].str();
        if(!fs::exists(fs::path(clipPath)))
            continue;
        
        customText = m[2].str();
        cropEnabled = m[3].str() == "true" ? true : false;
        cropStart = m[4].str();
        cropStartUnit = m[5].str()[0];
        cropStopAfterOrAt = m[6].str()[0];
        cropStop = m[7].str();
        cropStopUnit = m[8].str()[0];

        std::string timeConstrains{};
        std::string fArgs = "-vn ";

        if(cropEnabled)
        {
            if(cropStartUnit == '1')
                timeConstrains += "-ss " + cropStart + "s "; 
            else 
                timeConstrains += "-ss " + cropStart + "ms "; 
            
            if(stoi(cropStop) > 0)
            {
                if(cropStopAfterOrAt == '1')
                    timeConstrains += "-to ";
                else
                    timeConstrains += "-t ";
            
                if(cropStopUnit == '1')
                    timeConstrains += cropStop + "s " ; 
                else
                    timeConstrains += cropStop + "ms ";
            }
        }

        if(customText.empty())
            customText = clipPath;

        fs::path exp = (fs::path(export_dir) / fs::path(customText).filename()).replace_extension(fs::path(extension));
        if(fs::exists(exp))
            //exp = exp.root_path() / fs::path(exp.stem().string() + "(1)").replace_extension(exp.extension());
            continue;
        std::string exportPath = "\"" + exp.string() + "\"";
        clipPath = "\"" + clipPath + "\" ";

        system(("ffmpeg.exe -i " + clipPath + timeConstrains + fArgs + exportPath).c_str());
    }

}