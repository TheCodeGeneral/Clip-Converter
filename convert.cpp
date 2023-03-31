#include <iostream>
#include <vector>
#include <string>
#include <string.h>
#include <regex>
#include <fstream>
#include <filesystem>
#include <iomanip>

namespace fs = std::filesystem;

void ffmpeg(std::string const& rpConfPath, std::string const& exportDir, std::string const& extension);
void print_help();
int main(int argc, char** argv)
{
    std::string rpConfPath{};
    std::string exportDir{};
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
                exportDir = argv[3];
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
                exportDir = argv[3];
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
                    exportDir = argv[5];
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

    // Verify valid extension
    // Verify valid directory
    ffmpeg(rpConfPath, fs::absolute(exportDir).string(), extension);

}

void print_help()
{
    int descSpacing = 40;
    std::cout << "Usage: clip_converter <rp_soundboard path> -o <output path> [OPTIONS]...\n\n";
    std::cout << "Options:\n";
    std::cout << std::left << std::setw(descSpacing) << "  -o, --output <directory>" << "Output directory\n";
    std::cout << std::left << std::setw(descSpacing) << "  -e, --extention [extension]" << "Converted file format (default=mp3)\n";
    std::cout << std::left << std::setw(descSpacing) << "  -h, --help" << "Print this help message" << std::endl;
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
...
*/
void ffmpeg(std::string const& rpConfPath, std::string const& exportDir, std::string const& extension)  
{
    // Read configuration into rpConfig
    std::fstream confIn(rpConfPath);
    std::string rpConfig((std::istreambuf_iterator<char>(confIn)), std::istreambuf_iterator<char>());
    confIn.close();

    std::string newRpConfig = rpConfig;

    std::string clipPath{};
    std::string customText{};
    bool cropEnabled;
    char cropStartUnit; // 0 = milliseconds 1 = seconds
    char cropStopUnit; // 0 = milliseconds 1 = seconds
    char cropStopAfterOrAt; // 0 = after 1 = seconds at
    std::string cropStart;
    std::string cropStop;

    std::regex re("(?:.+path=(.+)\\n.+customText=(.*)\\n.+\\n.+\\n.+cropEnabled=(.+)\\n.+cropStartValue=(\\d+)\\n.+cropStartUnit=(\\d)\\n.+cropStopAfterAt=(\\d)\\n.+cropStopValue=(\\d+)\\n.+cropStopUnit=(\\d))");
    for(std::sregex_iterator i = std::sregex_iterator(rpConfig.begin(), rpConfig.end(), re); i != std::sregex_iterator(); ++i )
    {
        std::smatch m = *i;
        clipPath = m[1].str();
        
        // Check if clip to be converted exists
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

        // Setup crop parameter for ffmpeg
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

        // Converted file path
        fs::path exp = (fs::path(exportDir) / fs::path(customText).filename()).replace_extension(fs::path(extension));


        // Make duplicate if converted clip already exists
        if(fs::exists(exp))
        {
            std::string filename = exp.stem().string();
            std::smatch m;
            int copyNum; 
            std::string filenameBase;
            // Check if copy already exists
            if(std::regex_search(filename, m, std::regex(".+ Copy - \\((\\d+)\\)")))
            {
                // Increase copy number by 1 in ( )
                copyNum = stoi(m[1].str()) + 1;
                filenameBase = std::regex_replace(filename,std::regex("(.+ Copy - \\()(\\d+)\\)"), "$1");
                filename = filenameBase + std::to_string(copyNum) + ")";
            }
            else
                filename += " Copy - (1)";

            exp = exp.parent_path() / fs::path(filename).replace_extension(exp.extension());

            while(fs::exists(exp))
            {
                filename = filenameBase + std::to_string(++copyNum) + ")";
                exp = exp.parent_path() / fs::path(filename).replace_extension(exp.extension());
            }
        }


        // Escape the / delimter for regex in clip path
        std::regex clipRegexFormat(std::regex_replace(clipPath, std::regex("\\/"), "\\/", std::regex_constants::match_any));

        // Wrap path in quotes in case path contains spaces
        std::string exportPath = "\"" + exp.string() + "\"";
        clipPath = "\"" + clipPath + "\" ";

        system(("ffmpeg.exe -i " + clipPath + timeConstrains + fArgs + exportPath ).c_str());

        // Repalce Windows path sperators
        exportPath = std::regex_replace(exp.string(), std::regex("\\\\"), "/", std::regex_constants::match_any);
        // Replace new clip path into config
        newRpConfig = std::regex_replace (newRpConfig, clipRegexFormat, exportPath, std::regex_constants::match_any);
    }

    // Disable crop on new rp config
    newRpConfig = std::regex_replace (newRpConfig, std::regex("cropEnabled=true"), "cropEnabled=false", std::regex_constants::match_any);

    // Write changes to rp config 
    std::ofstream confOut(rpConfPath, std::ios::trunc);
    confOut << newRpConfig;
    confOut.close();

    std::cout << newRpConfig << std::endl;
    system("pause");
}