#include <app/application.h>
#include <states/game_state.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <utils/filesystem.h>
#include <filesystem>

class TeeBuf : public std::streambuf {
public:
    TeeBuf(std::streambuf* sb1, std::streambuf* sb2) : sb1(sb1), sb2(sb2) {}
protected:
    virtual int overflow(int c) {
        if (c == EOF) return !EOF;
        int const r1 = sb1->sputc(c);
        int const r2 = sb2->sputc(c);
        return r1 == EOF || r2 == EOF ? EOF : c;
    }
    virtual int sync() {
        int const r1 = sb1->pubsync();
        int const r2 = sb2->pubsync();
        return r1 == 0 && r2 == 0 ? 0 : -1;
    }
private:
    std::streambuf *sb1, *sb2;
};

TeeBuf* teeBufOut = nullptr;
TeeBuf* teeBufErr = nullptr;
std::ofstream logFile;

void InitLogging() {
    std::string logsDirStr = FileSystem::getPath("logs");
    namespace fs = std::filesystem;
    fs::path logsDir = fs::path(logsDirStr);

    if (!fs::exists(logsDir)) {
        fs::create_directories(logsDir);
    }

    auto now = std::chrono::system_clock::now();
    auto timeT = std::chrono::system_clock::to_time_t(now);
    std::tm bt{};
    localtime_s(&bt, &timeT);

    std::stringstream ss;
    ss << "log_" << std::put_time(&bt, "%Y-%m-%d_%H-%M-%S") << ".txt";
    fs::path logPath = logsDir / ss.str();

    logFile.open(logPath);
    if (logFile.is_open()) {
#ifdef ENABLE_DEBUG_SYSTEM
        static TeeBuf tOut(std::cout.rdbuf(), logFile.rdbuf());
        static TeeBuf tErr(std::cerr.rdbuf(), logFile.rdbuf());
        
        teeBufOut = &tOut;
        teeBufErr = &tErr;

        std::cout.rdbuf(teeBufOut);
        std::cerr.rdbuf(teeBufErr);
#else
        std::cout.rdbuf(logFile.rdbuf());
        std::cerr.rdbuf(logFile.rdbuf());
#endif

        std::cout << "[LOG] Logging started at " << std::put_time(&bt, "%Y-%m-%d %H:%M:%S") << std::endl;
        std::cout << "[LOG] Log file: " << logPath.string() << std::endl;
    }
}

#ifndef ENABLE_DEBUG_SYSTEM
#include <Windows.h>
#endif

int main() {
#ifndef ENABLE_DEBUG_SYSTEM
    FreeConsole();
#endif

    InitLogging();

    Application app;

    if (app.Init()) {
        app.PushState<GameState>();
        app.Run();
    }
    
    return 0;
}