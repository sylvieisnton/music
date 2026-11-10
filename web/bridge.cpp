#define main music_cli_main

#ifdef MUSIC_ENCODE
#include "../encode.cpp"
#else
#include "../decode.cpp"
#endif

#undef main

#include <string>
#include <vector>

extern "C" int run_music(const char *speed, const char *scale, int raw)
{
    try
    {
        std::vector<std::string> args = {
            "music",
            "-i", "/input",
            "-o", "/output",
            "--speed", speed,
            "--scale", scale};

        if (raw)
            args.emplace_back("--raw");
        
        std::vector<char*> argv;
        for (auto& arg : args) 
            argv.push_back(arg.data());

        argv.push_back(nullptr);

        int result = music_cli_main(static_cast<int>(args.size()), argv.data());
        
        std::cout.flush();
        std::cerr.flush();
        return result;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << std::endl;
        return 1;
    }
}