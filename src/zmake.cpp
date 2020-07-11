#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <sstream>
#include <vector>

static const std::string ZMAKE_VERSION = "ZMAKE VERSION 0.3.1";

// OS-SPECIFICS
#ifdef _WIN32
static const std::string ZMAKE_ROOT = "C:\\zmake";
static const std::string FOLDER_NOTATION = "\\";
const bool ON_WINDOWS = true;

#elif __linux__
static const std::string ZMAKE_ROOT = std::filesystem::path(getenv("HOME")).u8string() + "/zmake";
static const bool ON_WINDOWS = false;
static const std::string FOLDER_NOTATION = "/";

#else
static const std::string ZMAKE_ROOT = "/usr/local/opt/zmake";
static const bool ON_WINDOWS = false;
static const std::string FOLDER_NOTATION = "/";
#endif

// DEFAULTS (OS-DEPENDANT)
static const std::string DEFAULT_CFG =
std::string(R"([build]
version = "c++17"
autoflags = "-Wall -Wextra -Wpedantic"
)") +
"include = \"include () $ZMAKE_ROOT" + FOLDER_NOTATION + "global" + FOLDER_NOTATION + "include (-w)\"\n" +
"libraries = \"lib () $ZMAKE_ROOT" + FOLDER_NOTATION + "global" + FOLDER_NOTATION + "lib ()\"\n" +
#if defined(_WIN32) || defined(__APPLE__)
R"(
[profile.dev]
compiler = "clang"
optimization = ""
flags = "-Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic"

[profile.release]
compiler = "clang"
optimization = "-Ofast"
flags = "-Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -march=native"
)" +
#else
R"(
[profile.dev]
compiler = "gcc"
optimization = ""
flags = ""

[profile.release]
compiler = "gcc"
optimization = "-Ofast"
flags = "-march=native"
)" +
#endif
#ifdef __APPLE__
R"(
[profile.debug]
compiler = "clang"
optimization = "-Og"
flags = "-g -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic"
)";
#else
R"(
[profile.debug]
compiler = "gcc"
optimization = "-Og"
flags = "-g"
)";
#endif

static const std::string DEFAULT_INCLUDE =
R"(#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <random>
#include <string>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef char s8;
typedef short s16;
typedef long s32;
typedef float f32;
typedef double f64;
#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc++98-compat-pedantic"
#endif
typedef unsigned long long u64;
typedef long long s64;
#ifdef __clang__
#pragma GCC diagnostic pop
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-macros"
#define alloc(num, type) static_cast<type*>(malloc(sizeof(type)*num))
#define arrlen(array) (sizeof(array)/sizeof(array[0]))
#define putline(scanner, line) scanner << line << "\n"
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wc++98-compat"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-template"
#endif
template<typename... Args>
static inline void print(Args&&... args) { (std::cout << ... << std::forward<Args>(args)) << std::flush; }

template <typename Arg, typename... Args>
static inline void printl(Arg&& arg, Args&&... args) {
    std::cout << std::forward<Arg>(arg);
    ((std::cout << ' ' << std::forward<Args>(args)), ...);
    std::cout << std::endl;
}

// If string is equal to ANY of the others
template <typename... Args>
static inline bool streq(const std::string& str, Args&&... args) { return ((str.compare(std::forward<Args>(args)) == 0) || ...); }
#ifdef __clang__
#pragma GCC diagnostic pop
#endif // "-Wunused-template"

// Removes whitespace from both ends
static inline std::string trim(std::string str) {
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](int ch) { return !std::isspace(ch); }));
    str.erase(std::find_if(str.rbegin(), str.rend(), [](int ch) { return !std::isspace(ch); }).base(), str.end());
    return str;
}

static std::string timestr(const std::time_t& time = std::time(nullptr), const char* const format = "%Y-%m-%d %H:%M:%S") {
    #ifdef _WIN32
    std::tm tb;
    localtime_s(&tb, &time);
    char mbstr[64];
    std::strftime(mbstr, sizeof(mbstr), format, &tb);
    #else
    std::tm *tb;
    tb = localtime(&time);
    char mbstr[64];
    std::strftime(mbstr, sizeof(mbstr), format, tb);
    #endif
    return std::string(mbstr);
}

// Returns the result of system() call, _popen for Windows compatibility
static std::string syscall(const char* const cmd) {
    std::string ret = "";
    FILE* fpipe;
    #ifdef _WIN32
    fpipe = _popen(cmd, "r");
    #else
    fpipe = popen(cmd, "r");
    #endif
    if (fpipe == nullptr) return ret;
    char c;
    while (fread(&c, sizeof(c), 1, fpipe)) ret += c;
    #ifdef _WIN32
    _pclose(fpipe);
    #else
    pclose(fpipe);
    #endif
    return ret;
}
#ifdef __clang__
#pragma GCC diagnostic pop
#endif // "-Wc++98-compat" (print: fold, trim: lambda, timestr/syscall: nullptr)

static int rng(int lower_bound, int upper_bound) {
    thread_local std::random_device rd;
    thread_local std::mt19937 mt(rd());
    std::uniform_int_distribution<int> dist(lower_bound, upper_bound);
    return dist(mt);
}

static double rng() {
    thread_local std::random_device rd;
    thread_local std::mt19937 mt(rd());
    static std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(mt);
}
#pragma GCC diagnostic pop

using std::string;
using std::vector;
)";

static const std::string DEFAULT_PROGRAM =
R"(
int main() {
    print("Hello World!\n");
    print("Your lucky number is: ", rng(1, 100), "\n");
}
)";

static const std::string DEFAULT_GITIGNORE =
R"(# Ignore everything in this directory
*
# Except this file
!.gitignore)";

static const std::string DEFAULT_GITIGNORE_EMPTY =
R"(# Don't ignore this file
!.gitignore)";

enum {
    STATE_UNKNOWN,
    STATE_HELP,
    STATE_VERSION,
    STATE_CLEAN,
    STATE_NEW,
    STATE_OPEN,
    STATE_BUILD
};

// * * * * * * * * * * FUNCTIONS * * * * * * * * * *
template<typename... Args>
static inline void printz(Args&&... args) { (std::cout << ... << std::forward<Args>(args)) << std::flush; }

// If string is equal to ANY of the others
template <typename... Args>
static inline bool streqz(const std::string& str, Args&&... args) { return ((str.compare(std::forward<Args>(args)) == 0) || ...); }

// Removes whitespace from both ends
static inline std::string trimz(std::string str) {
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](int ch) { return !std::isspace(ch); }));
    str.erase(std::find_if(str.rbegin(), str.rend(), [](int ch) { return !std::isspace(ch); }).base(), str.end());
    return str;
}

static inline bool ends_with(const std::string& str, const std::string& suffix) {
    if (suffix.size() > str.size()) return false;
    return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}

static std::string timestrz(const std::time_t& time = std::time(nullptr), const char* const format = "%Y-%m-%d %H:%M:%S") {
    #ifdef _WIN32
    std::tm tb;
    localtime_s(&tb, &time);
    char mbstr[64];
    std::strftime(mbstr, sizeof(mbstr), format, &tb);
    #else
    std::tm *tb;
    tb = localtime(&time);
    char mbstr[64];
    std::strftime(mbstr, sizeof(mbstr), format, tb);
    #endif
    return std::string(mbstr);
}

// Returns the result of system() call, _popen for Windows compatibility
static std::string syscallz(const char* const cmd) {
    std::string ret = "";
    FILE* fpipe;
    #ifdef _WIN32
    fpipe = _popen(cmd, "r");
    #else
    fpipe = popen(cmd, "r");
    #endif
    if (fpipe == nullptr) return ret;
    char c;
    while (fread(&c, sizeof(c), 1, fpipe)) ret += c;
    #ifdef _WIN32
    _pclose(fpipe);
    #else
    pclose(fpipe);
    #endif
    return ret;
}

// Keeps asking the user for input until he answers yes or no
static bool get_yes_or_no() {
    std::string input;
    get_the_input:
        std::cout << "- " << std::flush;
        getline(std::cin, input);
        transform(input.begin(), input.end(), input.begin(), [](unsigned char c){ return toupper(c); });
        if (input == "YES" || input == "Y" || input == "JA" || input == "J") return true;
        else if (input == "NO" || input == "N" || input == "NEJ") {
            std::cout << "- Exiting." << std::endl;
            return false;
        }
        else goto get_the_input;
}

// Resets defaultinclude.hpp or defaultconfig.cfg
static bool reset_default(const char* const filename) {
    if (!std::filesystem::exists(ZMAKE_ROOT + FOLDER_NOTATION + "global" + FOLDER_NOTATION + filename)) {
        std::ofstream pt;
        std::cout << "- \"" << filename << "\" missing in " << ZMAKE_ROOT << FOLDER_NOTATION << "global." << std::endl;
        std::cout << "- Do you want to restore the default? [y/n]" << std::endl;
        if (!get_yes_or_no()) return false;
        std::cout << "- Restoring \"" << filename << "\"." << std::endl;
        pt.open(ZMAKE_ROOT + FOLDER_NOTATION + "global" + FOLDER_NOTATION + filename, std::ios::trunc);
        if (strcmp(filename, "defaultinclude.hpp") == 0)        pt << DEFAULT_INCLUDE;
        else if (strcmp(filename, "defaultconfig.cfg") == 0)    pt << DEFAULT_CFG;
        pt.close();
    }
    return true;
}

// Gross hack, but the only thing that I've found to work on C++17
static std::time_t file_to_t(const std::string& path) {
    auto ft = std::filesystem::last_write_time(path);
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(ft - decltype(ft)::clock::now() + std::chrono::system_clock::now());
    return std::chrono::system_clock::to_time_t(sctp);
}

// Uses ends_with
static inline bool is_file_include(const std::string& str) {
    if (str.substr(0, 1).compare("-") == 0)     return false;
    if (ends_with(str, std::string(".*")))      return true;
    if (ends_with(str, std::string(".c")))      return true;
    if (ends_with(str, std::string(".cc")))     return true;
    if (ends_with(str, std::string(".cpp")))    return true;
    if (ends_with(str, std::string(".z")))      return true;
    if (ends_with(str, std::string(".zpp")))    return true;
    return false;
}

static inline void change_folder_notation(std::string& str) { for (char& c: str) if (c == '\\' || c == '/') c = FOLDER_NOTATION.at(0); }

static inline bool str_is_in_vec(const std::string& str, const std::vector<std::string>& vec) {
    for (const std::string& s: vec) if (str.compare(s) == 0) return true;
    return false;
}

static inline bool str_is_in_vec(const std::string& str, const std::vector<std::filesystem::path>& vec) {
    for (const std::filesystem::path& s: vec) if (str.compare(s.u8string()) == 0) return true;
    return false;
}

// * * * * * * * * * * MAIN * * * * * * * * * *
/*
    TAGS:
    zmake flags work with hyphens or slashes (-dev = /dev).
    You can also type zmake gl projname (gl = gitless), to make a new program without git.
    -dev, -debug, -release
    -gcc, -clang-, -clang++, -msvc
    -nocmd, -notime, -nobuild, -norun, -run
    -std=c++17 = /std:c+17 = -c++17 => c++17
    * * * Clang-cl/msvc specific * * *:
    -fexceptions -> -EHsc
    -O0 -> -Od  // It recognizes -Ofast as -O2 by default, but not -O0.
    Adds libraries with -link -libpath:"dir" at the end of the compilation string.

    BUGS: Opening an executable without an extension doesn't work on Windows.
    - Opening "C:\zmake\target\Ny mapp\zztest_debug"
    'C:\zmake\target\Ny' is not recognized as an internal or external command, operable program or batch file.
    Running build files with zmake *.* takes in all files like zmake.cfg and adds it to the program name.
    If { goes on the next line, the regexes don't work.

    NOTES:
    "C:\\zmake\\global\\defaultinclude.hpp" path
    C:\zmake\global\defaultinclude.hpp      u8string
    Since everything is in the same compilation unit, we might want to automatically set everything to static.

    TODO:
    Add command "zmake reset local/global config/include/zmake.cfg/defaultinclude.hpp"
    Passing "-o", "-c", "-S", "-E" and -save-temps to clang-cl
    save-temps -> save-temps=obj
    Clean up the ugly code.
*/
int main(int argc, char* argv[]) {
    std::ofstream pt;   // Used everywhere with trunc
    std::ifstream qt;   // Used everywhere to read
    std::string read_line;
    std::string read_line_next;
    bool read_line_loop = false;

    // For the config, taken from git
    std::string username = "";
    std::string mail = "";
    std::string date = timestrz();

    // Things you can change from profile standards
    std::string build_profile = "";     // "debug"
    std::string compiler = "";          // "gcc"
    std::string cversion = "";          // "c++17"
    std::string optimization = "";
    std::string program_name = "";
    bool has_build_profile_flag = false;    // Otherwise use std
    bool has_compiler_flag = false;         // Otherwise use std
    bool has_cversion_flag = false;         // Otherwise use std
    bool has_optimization_flag = false;     // Otherwise use standards
    bool has_program_name_flag = false;     // Otherwise name it according to cfg or main.zpp

    bool has_output_flag = false;           // Otherwise just add -o

    std::vector<std::string> cppfiles;      // *.c *.cpp target/debug/main.cpp
    bool use_build_files = false;           // Otherwise build /src

    bool use_no_run = false;            // Otherwise run it, NOTE: use_no_build is STATE_OPEN
    bool use_no_time = false;           // Otherwise print compilation time
    bool use_no_cmd = false;            // Otherwise hide the cmd
    bool use_only_cpp = false;          // Otherwise add defaultinclude and zpp features
    bool use_gitless = false;           // Don't create .git and .gitignore
    bool use_local_include = false;     // Otherwise use the global one

    // For building both with and without use_build_files
    std::vector<std::string> build_files;
    std::vector<std::filesystem::path> zfiles;  // /src, /lib, /global/lib
    std::vector<std::filesystem::path> zfiles_inclist;  // /src, /lib, /global/lib

    // Commands used in everything
    std::vector<std::string> commands;
    commands.reserve(static_cast<unsigned int>(argc));
    for (int i = 1; i < argc; i++) {
        commands.emplace_back(std::string(argv[i]));
    }



    // Interpret input and Deletes the commands from vector
    // Aborts if there are not enough arguments to continue
    // (We do so since we print ignored commands afterwards)
    // Note that OPEN ingores flags and other commands
    int state;
    if (commands.size() == 0) {
        state = STATE_HELP;
    }
    else if (streqz(commands.at(0), "help", "-help", "/help", "--help", "h", "-h", "/h", "--h")) {
        state = STATE_HELP;
        commands.erase(commands.begin());
    }
    else if (streqz(commands.at(0), "version", "-version", "/version", "--version", "v", "-v", "/v", "--v")) {
        state = STATE_VERSION;
        commands.erase(commands.begin());
    }
    else if (streqz(commands.at(0), "clean")) {
        state = STATE_CLEAN;
        commands.erase(commands.begin());
        if (commands.size() != 0) {
            printz("- Too many arguments, aborting.\n");
            printz("- Note: \"clean\" only deletes the target directory.\n");
            return EXIT_FAILURE;
        }
    }
    else if (streqz(commands.at(0), "new", "gl", "gitless")) {
        if (streqz(commands.at(0), "gl", "gitless")) use_gitless = true;
        state = STATE_NEW;
        commands.erase(commands.begin());
        if (commands.size() == 0) {
            printz("- Too few arguments, aborting.\n");
            return EXIT_FAILURE;
        }
    }
    // Gets STATE_BUILD/STATE_OPEN, use_build_files and has_build_profile_flag
    // Needs to get build_profile for opening -debug
    else if (streqz(commands.at(0), "open", "run", "build", "debug") || is_file_include(commands.at(0))) {
        state = STATE_BUILD;    // Can change to STATE_OPEN with "open" or "-nobuild"

        // If you build with files
        if (is_file_include(commands.at(0))) {
            use_build_files = true;
            for (unsigned int i = 0; i < commands.size(); i++) {
                if (is_file_include(commands.at(i))) {
                    build_files.emplace_back(commands.at(i));
                    commands.erase(commands.begin() + i);
                    i--;
                }
            }
        }
        else {
            if (streqz(commands.at(0), "open")) state = STATE_OPEN;
            if (streqz(commands.at(0), "run")) build_profile = "dev";
            if (streqz(commands.at(0), "build")) build_profile = "release";
            if (streqz(commands.at(0), "debug")) build_profile = "debug";
            if (streqz(commands.at(0), "build", "debug")) use_no_run = true;
            else use_no_run = false;
            commands.erase(commands.begin());
        }

        // -nobuild THEN -norun
        for (unsigned int i = 0; i < commands.size(); i++) {
            if (streqz(commands.at(i), "-nobuild", "/nobuild")) {
                if (use_build_files) {  // If building with somefile.zpp commands
                    printz("- Need to build when only specifying files, aborting.\n");
                    return EXIT_FAILURE;
                }
                state = STATE_OPEN;
                commands.erase(commands.begin() + i);
                i--;
            }
        }
        for (unsigned int i = 0; i < commands.size(); i++) {
            if (streqz(commands.at(i), "-norun", "/norun")) {
                if (state == STATE_OPEN) {
                    printz("- Neither building nor running, done!\n");
                    return EXIT_SUCCESS;
                }
                use_no_run = true;
                commands.erase(commands.begin() + i);
                i--;
            }
            if (streqz(commands.at(i), "-run", "/run")) {
                use_no_run = false;
                commands.erase(commands.begin() + i);
                i--;
            }
        }

        // Get build profile
        for (unsigned int i = 0; i < commands.size(); i++) {
            if (streqz(commands.at(i), "-dev", "-debug", "-release", "-custom", "/dev", "/debug", "/release", "/custom")) {
                if (state != STATE_OPEN && streqz(commands.at(i), "-custom", "/custom")) {
                    printz("- Build profile \"custom\" can only be called when opening files, aborting.\n");
                    return EXIT_FAILURE;
                }
                if (has_build_profile_flag) {
                    printz("- Multiple build profile arguments, aborting.\n");
                    return EXIT_FAILURE;
                }
                else {
                    if (streqz(commands.at(i).substr(0, 1), "-", "/")) commands.at(i) = commands.at(i).substr(1);
                    has_build_profile_flag = true;
                    build_profile = commands.at(i);
                    commands.erase(commands.begin() + i);
                    i--;
                }
            }
        }
    }
    else {
        state = STATE_UNKNOWN;
        printz("- Unknown command \"" + commands.at(0) + "\", aborting.\n");

        std::string firstarg = commands.at(0);
        transform(firstarg.begin(), firstarg.end(), firstarg.begin(), [](unsigned char c){ return tolower(c); });
        if (firstarg.compare(commands.at(0)) != 0)  printz("- Note: zmake is case-sensitive, try using lowercase only!\n");
        else if (streqz(firstarg, "clear"))         printz("- Note: Did you mean \"clean\"?\n");

        return EXIT_FAILURE;
    }



    // Print out INGORED COMMANDS
    if (commands.size() != 0 && state != STATE_BUILD && state != STATE_NEW && state != STATE_CLEAN) {
        for (unsigned int i = 0; i < commands.size(); i++) {
            if (i == 0) printz("- Ignoring commands: \"" + commands.at(i) + "\"");
            else printz(", \"" + commands.at(i) + "\"");
        }
        printz(".\n");
    }



    // Last step - do the actual work
    if (state == STATE_HELP) {
        printz("- ", ZMAKE_VERSION, ".",
R"(

- Make a new project with "zmake new project_name".
- Build and run the dev build with "zmake run".
- Build the release build with "zmake build".
- Build the debug build with "zmake debug".
- Open the most recently compiled build with "zmake open".
- Remove target files with "zmake clean".

- You can also add any "-gccflags" at the end of your command
- to compile with them, or the following built in commands:
- "-dev/-debug/-release" (change build profile),
- "-nocmd" (hide compiler command),
- "-notime" (hide compilation time),
- "-nobuild" (only running),
- "-norun" (only building),
- "-run" (run after building),
- or "-gcc/-clang/-clang++" to change compiler.
- On Windows, clang (or clang-cl) compiles with clang-cl, unlike clang++.
)");
        return EXIT_SUCCESS;
    }

    if (state == STATE_VERSION) {
        printz("- ", ZMAKE_VERSION, ".\n");
        return EXIT_SUCCESS;
    }

    if (state == STATE_CLEAN) {
        if (!std::filesystem::exists("src")) {
            printz("- Not a zmake directory, aborting.\n");
            return EXIT_FAILURE;
        }
        std::uintmax_t del_num = 0;
        for (const auto& p: std::filesystem::recursive_directory_iterator("target")) {
            if (std::filesystem::is_directory(p)) continue;
            if (streqz(p.path().filename().u8string(), ".gitignore")) continue;
            if (std::filesystem::remove(p)) del_num++;
        }
        printz("- Deleted ", del_num, " files.\n");
        return EXIT_SUCCESS;
    }

    if (state == STATE_NEW) {
        std::string new_project_name = commands.at(0);
        for (unsigned int i = 1; i < commands.size(); i++) {
            new_project_name += " ";
            new_project_name += commands.at(i);
        }
        bool CONexception;
        try {
            CONexception = std::filesystem::exists(new_project_name);
        } catch (const std::filesystem::filesystem_error&) {
            printz("- Invalid directory \"" + new_project_name + "\", aborting.\n");
            return EXIT_FAILURE;
        }
        if (CONexception) {
            printz("- Directory \"" + new_project_name + "\" already exists, aborting.\n");
            return EXIT_FAILURE;
        }
        // Reset the default config if not found
        if (!reset_default("defaultconfig.cfg")) return EXIT_FAILURE;

        auto c_path = std::filesystem::current_path();
        // Create new folder and try to git init
        try {
            CONexception = std::filesystem::create_directory(new_project_name);
        } catch (const std::filesystem::filesystem_error&) {
            printz("- Couldn't create directory \"" + new_project_name + "\", aborting.\n");
            return EXIT_FAILURE;
        }
        if (!CONexception) {
            printz("- Couldn't create directory \"" + new_project_name + "\", aborting.\n");
            return EXIT_FAILURE;
        }
        std::filesystem::current_path(new_project_name);
        syscallz("git init");
        bool git_success = std::filesystem::exists(".git");
        // Subfolders
        std::filesystem::create_directory("include");
        std::filesystem::create_directory("lib");
        std::filesystem::create_directory("src");
        std::filesystem::create_directory("target");
        // Get git username and email
        if (git_success) {
            if (!use_gitless) {
                pt.open("include/.gitignore", std::ios::trunc);
                pt << DEFAULT_GITIGNORE_EMPTY;
                pt.close();
                pt.open("lib/.gitignore", std::ios::trunc);
                pt << DEFAULT_GITIGNORE_EMPTY;
                pt.close();
                pt.open("target/.gitignore", std::ios::trunc);
                pt << DEFAULT_GITIGNORE;
                pt.close();
            }
            else {
                std::filesystem::remove_all(".git");
            }
            username = trimz(syscallz("git config user.name"));
            mail = trimz(syscallz("git config user.email"));
        }
        // Create main.zpp
        pt.open("src/main.zpp", std::ios::trunc);
        pt << DEFAULT_PROGRAM;
        pt.close();
        // Create zmake.cfg
        pt.open("zmake.cfg", std::ios::trunc);
        pt << std::string("[package]\nname = \"") + new_project_name +
            "\"\nversion = \"0.1.0\"\nauthor = \"" + username + " <" +
            mail + ">\"\ncreated = \"" + date + "\"\n\n";

        qt.open(ZMAKE_ROOT + FOLDER_NOTATION + "global" + FOLDER_NOTATION + "defaultconfig.cfg");
        if (!qt.is_open()) {
            printz("- Couldn't open defaultconfig.cfg, aborting.\n");
            return EXIT_FAILURE;
        }
        while(getline(qt, read_line)) {
            pt << read_line;
            pt << "\n";
        }
        pt.close();
        qt.close();

        std::filesystem::current_path(c_path);
        printz("- Directory \"" + new_project_name + "\" created.\n");
        return EXIT_SUCCESS;
    }

    if (state == STATE_OPEN) {
        if (!use_build_files && !std::filesystem::exists("src")) {
            printz("- Not a zmake directory, aborting.\n");
            return EXIT_FAILURE;
        }

        if (!std::filesystem::exists("target")) {
            printz("- Target directory doesn't exist, aborting.\n");
            return EXIT_FAILURE;
        }
        // Find latest executable
        std::vector<std::filesystem::path> programs;
        programs.reserve(3);

        for (const auto& p: std::filesystem::recursive_directory_iterator("target")) {
            if (std::filesystem::is_directory(p.path())) continue;
            if (!streqz(p.path().extension().u8string(), "", ".exe")) continue;
            programs.emplace_back(p.path());
        }

        if (programs.size() == 0) {
            printz("- No executables found, aborting.\n");
            return EXIT_FAILURE;
        }
        // If build_profile == "" from open, then just take any old exe
        if (streqz(build_profile, "dev", "debug", "release", "custom")) {
            for (unsigned int i = 0; i < programs.size(); i++) {
                if (!ends_with(programs.at(i).u8string(), "_" + build_profile) &&
                    !ends_with(programs.at(i).u8string(), "_" + build_profile + ".exe")) {
                    programs.erase(programs.begin() + i);
                    i--;
                }
            }
            if (programs.size() == 0) {
                printz("- No \"" + build_profile + "\" build executable found, aborting.\n");
                return EXIT_FAILURE;
            }
        }

        std::time_t bestTime = file_to_t(programs.at(0).u8string());
        std::time_t tempTime;

        for (unsigned int i = 1; i < programs.size(); i++) {
            tempTime = file_to_t(programs.at(i).u8string());
            if (tempTime > bestTime) {
                bestTime = tempTime;
                programs.erase(programs.begin(), programs.begin() + i);
                i = 0;
            }
        }
        // Open the program
        std::string progname = programs.at(0).stem().u8string() + programs.at(0).extension().u8string();
        printz("- Opening " + progname + ".\n\n");
        std::string progcmd = "\"" + std::filesystem::absolute(programs.at(0)).u8string() + "\"";
        system(progcmd.c_str());

        return EXIT_SUCCESS;
    }

    if (state == STATE_BUILD) {
        if (!use_build_files && !std::filesystem::exists("src")) {
            printz("- Not a zmake directory, aborting.\n");
            return EXIT_FAILURE;
        }
        // Get compiler, cversion, optimization, -o/-c/-E/-S, program_name
        // i.e. flags that change defaults
        // Get also the last zmake flags, i. e. -notime, -nocmd
        for (unsigned int i = 0; i < commands.size(); i++) {
            if (streqz(commands.at(i), "-gcc", "-g++", "-clang", "-clang-cl", "-clang++", "-msvc", "-cl",
                                       "/gcc", "/g++", "/clang", "/clang-cl", "/clang++", "/msvc", "/cl")) {
                if (has_compiler_flag) {
                    printz("- Multiple compiler arguments, aborting.\n");
                    return EXIT_FAILURE;
                }
                else {
                    if (streqz(commands.at(i).substr(0, 1), "-", "/")) commands.at(i) = commands.at(i).substr(1);
                    has_compiler_flag = true;
                    if (streqz(commands.at(i), "gcc")) compiler = "g++";
                    else if (streqz(commands.at(i), "clang") && ON_WINDOWS) compiler = "clang-cl";
                    else if (streqz(commands.at(i), "msvc")) compiler = "cl";
                    else compiler = commands.at(i);
                    commands.erase(commands.begin() + i);
                    i--;
                }
            }
            else if (streqz(commands.at(i).substr(0, 8), "-std=c++", "/std:c++") ||
                     streqz(commands.at(i).substr(0, 4), "-c++", "/c++")) {
                if (has_cversion_flag) {
                    printz("- Multiple C++ version arguments, aborting.\n");
                    return EXIT_FAILURE;
                }
                else {
                    has_cversion_flag = true;
                    if (streqz(commands.at(i).substr(0, 1), "-", "/")) commands.at(i) = commands.at(i).substr(1);
                    if (streqz(commands.at(i).substr(0, 4), "std=", "std:")) commands.at(i) = commands.at(i).substr(4);
                    cversion = commands.at(i);
                    commands.erase(commands.begin() + i);
                    i--;
                }
            }
            else if (streqz(commands.at(i).substr(0, 2), "-O") || (streqz(commands.at(i).substr(0, 2), "/O") && commands.at(i).length() == 3)) {
                if (has_optimization_flag) {
                    printz("- Multiple optimization arguments, aborting.\n");
                    return EXIT_FAILURE;
                }
                else {
                    has_optimization_flag = true;
                    optimization = commands.at(i);
                    commands.erase(commands.begin() + i);
                    i--;
                }
            }
            else if (streqz(commands.at(i), "-nocmd", "/nocmd")) {
                use_no_cmd = true;
                commands.erase(commands.begin() + i);
                i--;
            }
            else if (streqz(commands.at(i), "-notime", "/notime")) {
                use_no_time = true;
                commands.erase(commands.begin() + i);
                i--;
            }
            else if (streqz(commands.at(i), "-o", "-c", "-S", "-E", "/o", "/c", "/S", "/E")) {
                has_output_flag = true;
            }
            // Loop to get long names, first time we manipulate program_name
            else if (streqz(program_name, "")) {
                long_name:
                if (!streqz(commands.at(i).substr(0, 1), "-", "/")) {
                    has_program_name_flag = true;
                    if (!streqz(program_name, "")) program_name += " ";
                    program_name += commands.at(i);
                    commands.erase(commands.begin() + i);
                    if (i < commands.size()) goto long_name;
                    else i--;
                }
            }
        }

        // If name faults, use the directory
        if (streqz(program_name, "")) program_name = "\"" + std::filesystem::current_path().stem().u8string() + "\"";
        else program_name = "\"" + program_name + "\"";

        // Fix config
        if (use_build_files && !reset_default("defaultconfig.cfg")) return EXIT_FAILURE;

        if (!use_build_files && !std::filesystem::exists("zmake.cfg")) {
            printz("- \"zmake.cfg\" missing in current directory.\n");
            printz("- Do you want to use the default config? [y/n]\n");
            if (!get_yes_or_no()) return EXIT_FAILURE;
            if (!reset_default("defaultconfig.cfg")) return EXIT_FAILURE;
            printz("- Restoring \"zmake.cfg\".\n");
            if (std::filesystem::exists(".git")) {
                username = trimz(syscallz("git config user.name"));
                mail = trimz(syscallz("git config user.email"));
            } else {
                syscallz("git init");
                if (std::filesystem::exists(".git")) {
                    std::filesystem::remove_all(".git");
                    username = trimz(syscallz("git config user.name"));
                    mail = trimz(syscallz("git config user.email"));
                }
            }
            pt.open("zmake.cfg", std::ios::trunc);
            pt << std::string("[package]\nname = ") << program_name <<
                "\nversion = \"0.1.0\"\nauthor = \"" + username + " <" +
                mail + ">\"\ncreated = \"\"\n\n";
            qt.open(ZMAKE_ROOT + FOLDER_NOTATION + "global" + FOLDER_NOTATION + "defaultconfig.cfg");
            if (!qt.is_open()) {
                printz("- Couldn't open defaultconfig.cfg, aborting.\n");
                return EXIT_FAILURE;
            }
            while(getline(qt, read_line)) {
                pt << read_line;
                pt << "\n";
            }
            pt.close();
            qt.close();
        }

        auto a = std::chrono::steady_clock::now();

        // Regex
        // These variables are reused throughout the code
        std::string in;
        std::smatch matches;

        std::string current_profile = "";
        std::string current_flag = "";
        std::string config_flags = "";
        const std::regex reg_profile("^\\[(.*)\\](.*)");
        const std::regex reg_flag("^(.*?)(\\s)*=(\\s)*\"(.*)\"(.*)");
        const std::regex reg_lib_inc("(.*?)\\((.*?)\\)(.*)");
        if (streqz(build_profile, "")) build_profile = "dev";

        if (!use_build_files) qt.open("zmake.cfg");
        else qt.open(ZMAKE_ROOT + FOLDER_NOTATION + "global" + FOLDER_NOTATION + "defaultconfig.cfg");

        if (!qt.is_open()) {
            printz("- Couldn't open config, aborting.\n");
            return EXIT_FAILURE;
        }

        std::vector<std::filesystem::path> cfg_includes;
        std::vector<std::filesystem::path> cfg_libs;
        std::vector<std::string> cfg_inccommands;
        std::vector<std::string> cfg_libcommands;

        // Read cfg
        while(getline(qt, read_line)) {
            if (std::regex_match(read_line, matches, reg_profile)) {
                current_profile = matches[1];
                continue;
            }
            if (std::regex_match(read_line, matches, reg_flag)) {
                current_flag = matches[1];
                if (streqz(current_profile, "package")) {
                    if (streqz(current_flag, "name")) {
                        if (has_program_name_flag) continue;
                        program_name = "\"" + std::string(matches[4]) + "\"";
                    }
                }
                else if (streqz(current_profile, "build")) {
                    if (streqz(current_flag, "version")) {
                        if (has_cversion_flag) continue;
                        cversion = matches[4];
                        // Set it to C++xx
                        if (streqz(cversion.substr(0, 1), "-", "/")) cversion = cversion.substr(1);
                        if (streqz(cversion.substr(0, 4), "std=", "std:")) cversion = cversion.substr(4);
                    }
                    else if (streqz(current_flag, "autoflags")) {
                        if (!streqz(config_flags, "")) config_flags += " ";
                        config_flags += matches[4];
                    }
                    else if (streqz(current_flag, "include")) {
                        std::string inc = matches[4];
                        std::string incmd = "";
                        while (std::regex_match(inc, matches, reg_lib_inc)) {
                            in = trimz(std::string(matches[1]));
                            in = std::regex_replace(in, std::regex("\\$ZMAKE_ROOT"), ZMAKE_ROOT);
                            change_folder_notation(in);
                            incmd = trimz(std::string(matches[2]));
                            if (!std::filesystem::exists(in)) {
                                printz("- Include path \"" + in + "\" in config doesn't exist, aborting.\n");
                                return EXIT_FAILURE;
                            }
                            inc = matches[3];
                            cfg_includes.emplace_back(in);
                            cfg_inccommands.emplace_back(incmd);
                        }
                    }
                    else if (streqz(current_flag, "libraries")) {
                        std::string inc = matches[4];
                        std::string incmd = "";
                        while (std::regex_match(inc, matches, reg_lib_inc)) {
                            in = trimz(std::string(matches[1]));
                            in = std::regex_replace(in, std::regex("\\$ZMAKE_ROOT"), ZMAKE_ROOT);
                            change_folder_notation(in);
                            incmd = trimz(std::string(matches[2]));
                            if (!std::filesystem::exists(in)) {
                                printz("- Library path \"" + in + "\" in config doesn't exist, aborting.\n");
                                return EXIT_FAILURE;
                            }
                            inc = matches[3];
                            cfg_libs.emplace_back(in);
                            cfg_libcommands.emplace_back(incmd);
                        }
                    }
                }
                else if (streqz(current_profile, "profile." + build_profile)) {
                    if (streqz(current_flag, "compiler")) {
                        if (has_compiler_flag) continue;
                        compiler = matches[4];
                        if (streqz(compiler.substr(0, 1), "-", "/")) compiler = compiler.substr(1);
                        if (streqz(compiler, "gcc")) compiler = "g++";
                        else if (ON_WINDOWS && streqz(compiler, "clang")) compiler = "clang-cl";
                        else if (streqz(compiler, "msvc")) compiler = "cl";
                    }
                    else if (streqz(current_flag, "optimization")) {
                        if (has_optimization_flag) continue;
                        optimization = matches[4];
                    }
                    else if (streqz(current_flag, "flags")) {
                        if (has_compiler_flag) continue;
                        if (!streqz(config_flags, "")) config_flags += " ";
                        config_flags += matches[4];
                    }
                }
            }
        }
        qt.close();
        commands.emplace_back(optimization);
        if (has_compiler_flag) build_profile = "custom";

        // Import default commands
        std::vector<std::string> default_commands;
        commands.reserve(5);
        std::istringstream iss(config_flags.c_str());
        while (getline(iss, in, ' ')) default_commands.insert(default_commands.begin(), in);

        // Insert default_commands into commands
        for (unsigned int i = 0; i < default_commands.size(); i++) {
            // Manual output flag takes priority
            if (has_output_flag && streqz(default_commands.at(i), "-o", "-c", "-S", "-E", "/o", "/c", "/S", "/E")) continue;
            // No duplicates
            if (str_is_in_vec(default_commands.at(i), commands)) continue;
            commands.insert(commands.begin(), default_commands.at(i));
        }
        default_commands.clear();

        // Get all files to compile
        if (!use_build_files) {
            for (const auto& p: std::filesystem::recursive_directory_iterator("src")) {
                if (std::filesystem::is_directory(p.path())) continue;
                in = p.path().relative_path().parent_path().u8string() + FOLDER_NOTATION;
                if (streqz(p.path().extension().u8string(), ".c")) {
                    // No duplicates
                    if (str_is_in_vec(in + "*.c", cppfiles)) continue;
                    cppfiles.emplace_back(in + "*.c");
                }
                else if (streqz(p.path().extension().u8string(), ".cpp")) {
                    if (str_is_in_vec(in + "*.cpp", cppfiles)) continue;
                    cppfiles.emplace_back(in + "*.cpp");
                }
                else if (streqz(p.path().extension().u8string(), ".cc")) {
                    if (str_is_in_vec(in + "*.cc", cppfiles)) continue;
                    cppfiles.emplace_back(in + "*.cc");
                }
                else if (streqz(p.path().extension().u8string(), ".z", ".zpp")) {
                    if (streqz(p.path().stem().u8string(), "main")) zfiles_inclist.insert(zfiles_inclist.begin(), p.path());
                    else zfiles_inclist.emplace_back(p.path());
                }
            }
        }
        else {
            // .c and .cpp are left for the compiler
            for (unsigned int i = 0; i < build_files.size(); i++) {
                if (ends_with(build_files.at(i), ".c") || ends_with(build_files.at(i), ".cpp") || ends_with(build_files.at(i), ".cc")) {
                    cppfiles.emplace_back(build_files.at(i));
                }
                else if (ends_with(build_files.at(i), "*.z") || ends_with(build_files.at(i), "*.zpp")) {
                    std::filesystem::path build_file_path = build_files.at(i);
                    for (const auto& p: std::filesystem::directory_iterator(build_file_path.parent_path())) {
                        if (std::filesystem::is_directory(p.path())) continue;
                        if (!streqz(p.path().extension().u8string(), ".z", ".zpp")) continue;
                        // No duplicates
                        if (str_is_in_vec(p.path().u8string(), zfiles_inclist)) continue;
                        zfiles_inclist.emplace_back(p.path().u8string());
                    }
                }
                else if (ends_with(build_files.at(i), ".z") || ends_with(build_files.at(i), ".zpp")) {
                    std::filesystem::path build_file_path = build_files.at(i);
                    if (str_is_in_vec(build_file_path.u8string(), zfiles_inclist)) continue;
                    zfiles_inclist.emplace_back(build_file_path.u8string());
                }
            }
        }

        if (zfiles_inclist.size() == 0) use_only_cpp = true;

        // Find main
        int main_entry = -1;
        const std::regex reg_main("((.*) main|^main)(\\s)*\\((.*)\\)(\\s)*\\{(.*)");
        const std::regex reg_string_start("(.*)R\"\\((.*)");
        const std::regex reg_string_end("(.*)\\)\"(.*)");
        const std::regex reg_comment_start("(.*)/\\*(.*)");
        const std::regex reg_comment_end("(.*)\\*/(.*)");
        const std::regex reg_comment_one_line("^(\\s*)//(.*)");
        const std::regex reg_second_line_bracket("^(\\s*)\\{(.*)");
        bool in_string = false;
        bool in_comment = false;

        for (unsigned int i = 0; i < zfiles_inclist.size(); i++) {
            qt.open(zfiles_inclist.at(i));
            if (!qt.is_open()) {
                printz("- Couldn't open file \"", zfiles_inclist.at(i), "\", aborting.\n");
                return EXIT_FAILURE;
            }
            while (getline(qt, read_line)) {
                mainfinder:
                if (in_string) {
                    if (std::regex_match(read_line, matches, reg_string_end)) {
                        in_string = false;
                        read_line = matches[2];
                        goto mainfinder;
                    }
                }
                else if (in_comment) {
                    if (std::regex_match(read_line, matches, reg_comment_end)) {
                        in_comment = false;
                        read_line = matches[2];
                        goto mainfinder;
                    }
                }
                else if (std::regex_match(read_line, matches, reg_comment_one_line)) {
                    continue;
                }
                else {
                    if (std::regex_match(read_line, matches, reg_string_start) && !in_comment) {
                        if (in_string) continue;
                        else {
                            in_string = true;
                            goto mainfinder;
                        }
                    }
                    if (std::regex_match(read_line, matches, reg_comment_start) && !in_string) {
                        if (in_comment) continue;
                        else {
                            in_comment = true;
                            goto mainfinder;
                        }
                    }
                    // Get { on second line, put this after comment and strings
                    if (getline(qt, read_line_next)) {
                        read_line_loop = true;
                        if (std::regex_match(read_line_next, matches, reg_second_line_bracket)) {
                            read_line = trimz(read_line) + trimz(read_line_next);
                        }
                    }
                    if (std::regex_match(read_line, matches, reg_main)) {
                        main_entry = static_cast<int>(i);
                        break;
                    }
                    if (read_line_loop) {
                        read_line = read_line_next;
                        read_line_loop = false;
                        goto mainfinder;
                    }
                }
            }
            qt.close();
            if (main_entry != -1) break;
        }
        if (main_entry == -1) {
            use_only_cpp = true;
            if (cppfiles.size() == 0)  {
                printz("- Couldn't find main function, aborting.\n");
                return EXIT_FAILURE;
            }
        }
        in_string = false;
        in_comment = false;
        read_line_loop = false;

        // Find more files in includes
        for (unsigned int i = 0; i < cfg_includes.size(); i++) {
            for (const auto& p: std::filesystem::recursive_directory_iterator(cfg_includes.at(i))) {
                if (std::filesystem::is_directory(p.path())) continue;
                if (!streqz(p.path().extension().u8string(), ".z", ".zpp")) continue;
                zfiles_inclist.emplace_back(p.path());
            }
        }
        // Defaultinclude in front
        if (!use_only_cpp) {
            if (std::filesystem::exists("defaultinclude.hpp")) {    // Local in cwd
                use_local_include = true;
            }
            else {
                if (!reset_default("defaultinclude.hpp")) return EXIT_FAILURE;
            }
            if (use_local_include) zfiles_inclist.insert(zfiles_inclist.begin(), "defaultinclude.hpp");
            else zfiles_inclist.insert(zfiles_inclist.begin(), ZMAKE_ROOT + FOLDER_NOTATION + "global" + FOLDER_NOTATION + "defaultinclude.hpp");

            /* Put the .zpp files into a cpp file */
            zfiles.emplace_back(zfiles_inclist.at(0));  // default
            zfiles.emplace_back(zfiles_inclist.at(1));  // main

            // 2D vector to store includes along with which files included them to show in *_zmake.cpp
            std::vector<std::vector<std::string>> include_list;
            include_list.reserve(8);
            const std::regex reg_include("^#include (<(.*?)>|\"(.*)\")(.*)");

            // Forward declarations (reg functions varför funkar du inte)
            const std::regex reg_structs("^(struct|class|union) (\\S+)\\s*\\{(.*)");
            const std::regex reg_functions("^((\\S+\\s+)+?)(\\S+)\\((.*)\\)\\s*\\{(.*)"); //[1] type, [2] fcn_name, [3] args.
            const std::regex reg_template("^template(\\s*)<(.*?)>(.*)");
            const std::regex reg_args("(.*?)=(.*?),(.*)");
            const std::regex reg_args_end("(.*?)=(.*)");
            std::vector<std::string> forward_structs;
            std::vector<std::string> forward_functions;
            forward_structs.reserve(8);
            forward_functions.reserve(64);
            std::string template_fcn = "";

            // Get includes, and structs, classes, unions and functions
            // so we can forward declare them, in that order.
            for (unsigned int i = 0; i < zfiles.size(); i++) {
                qt.open(zfiles.at(i));
                if (!qt.is_open()) {
                    printz("- Couldn't open file \"", zfiles.at(i), "\", aborting.\n");
                    return EXIT_FAILURE;
                }
                // Annotate where structs and functions come from, removed at end if not filled
                forward_structs.emplace_back(std::string("// From ") + zfiles.at(i).u8string());
                forward_functions.emplace_back(std::string("// From ") + zfiles.at(i).u8string());
                while (getline(qt, read_line)) {
                    forwarddeclarer:
                    if (in_string) {
                        if (std::regex_match(read_line, matches, reg_string_end)) {
                            in_string = false;
                            read_line = matches[2];
                            goto forwarddeclarer;
                        }
                    }
                    else if (in_comment) {
                        if (std::regex_match(read_line, matches, reg_comment_end)) {
                            in_comment = false;
                            read_line = matches[2];
                            goto forwarddeclarer;
                        }
                    }
                    else if (std::regex_match(read_line, matches, reg_comment_one_line)) {
                        continue;
                    }
                    else {
                        if (std::regex_match(read_line, matches, reg_string_start) && !in_comment) {
                            if (in_string) continue;
                            else {
                                in_string = true;
                                goto forwarddeclarer;
                            }
                        }
                        if (std::regex_match(read_line, matches, reg_comment_start) && !in_string) {
                            if (in_comment) continue;
                            else {
                                in_comment = true;
                                goto forwarddeclarer;
                            }
                        }
                        // Get { on second line, put this after comment and strings
                        if (getline(qt, read_line_next)) {
                            read_line_loop = true;
                            if (std::regex_match(read_line_next, matches, reg_second_line_bracket)) {
                                read_line = trimz(read_line) + trimz(read_line_next);
                            }
                        }
                        if (std::regex_match(read_line, matches, reg_include)) {
                            bool include_already_exists = false;
                            std::string incfile = matches[1];
                            std::string zpp_file_inc = incfile.substr(1, incfile.length() - 2);
                            // Including a .zpp file
                            if (ends_with(zpp_file_inc, ".zpp") || ends_with(zpp_file_inc, ".z")) {
                                for (unsigned int j = 0; j < zfiles_inclist.size(); j++) {
                                    if (streqz(zfiles_inclist.at(j).filename().u8string(), zpp_file_inc)) {
                                        if (!str_is_in_vec(zfiles_inclist.at(j).u8string(), zfiles)) {
                                            zfiles.emplace_back(zfiles_inclist.at(j));
                                        }
                                        incfile = "//#include \"" + zpp_file_inc + "\"";
                                        goto inc_label;
                                    }
                                }
                                printz("- Couldn't find file \"", zpp_file_inc, "\", aborting.\n");
                                return EXIT_FAILURE;
                            }
                            else incfile = "#include " + incfile;
                            inc_label:
                            for (unsigned int j = 0; j < include_list.size(); j++) {
                                if (streqz(incfile, include_list.at(j).at(0))) {
                                    include_already_exists = true;
                                    include_list.at(j).emplace_back(zfiles.at(i).u8string());
                                    break;
                                }
                            }
                            if (!include_already_exists) {
                                include_list.emplace_back(std::vector<std::string> { incfile, zfiles.at(i).u8string() });
                            }
                        }
                        else if (std::regex_match(read_line, matches, reg_structs)) {
                            if (i == 0) continue;   // Don't include structs from defaultinclude.hpp
                            std::string temp_struct = matches[1];
                            temp_struct += " ";
                            temp_struct += matches[2];
                            temp_struct += ";";
                            forward_structs.emplace_back(temp_struct);
                        }
                        else if (std::regex_match(read_line, matches, reg_functions)) {
                            if (i == 0) continue;   // Don't include functions from defaultinclude.hpp
                            if (streqz("main", matches[3])) continue;
                            std::string temp_fcn = template_fcn;
                            temp_fcn += matches[1];
                            temp_fcn += matches[3];
                            temp_fcn += "(";
                            std::string temp_args = matches[4];
                            while (std::regex_match(temp_args, matches, reg_args)) {
                                temp_args = trimz(matches[1]);
                                temp_args += ", ";
                                temp_args += trimz(matches[3]);
                            }
                            if (std::regex_match(temp_args, matches, reg_args_end)) {
                                temp_args = trimz(matches[1]);
                            }
                            temp_fcn += temp_args;
                            temp_fcn += ");";
                            forward_functions.emplace_back(temp_fcn);
                        }
                        else if (std::regex_match(read_line, matches, reg_template)) {
                            template_fcn = "template <" + std::string(matches[2]) + ">\n";
                        }
                        else {
                            template_fcn = "";
                        }
                        if (read_line_loop) {
                            read_line = read_line_next;
                            read_line_loop = false;
                            goto forwarddeclarer;
                        }
                    }
                }
                // If a file doesn't add any new structs/functions, remove it from *_zmake.cpp.
                if (streqz(forward_structs.at(forward_structs.size() - 1).substr(0, 2), "//")) forward_structs.erase(forward_structs.begin() + static_cast<long long>(forward_structs.size()) - 1);
                else forward_structs.emplace_back("");
                if (streqz(forward_functions.at(forward_functions.size() - 1).substr(0, 2), "//")) forward_functions.erase(forward_functions.begin() + static_cast<long long>(forward_functions.size()) - 1);
                else forward_functions.emplace_back("");
                qt.close();
            }
            in_string = false;
            in_comment = false;
            read_line_loop = false;
            bool empty_start_lines = true;

            // Add rest of code to *_zmake.cpp
            std::vector<std::string> forward_zcode;
            forward_zcode.reserve(16);
            std::string zfile_code = "";

            for (unsigned int i = 0; i < zfiles.size(); i++) {
                qt.open(zfiles.at(i));
                if (!qt.is_open()) {
                    printz("- Couldn't open file \"", zfiles.at(i), "\", aborting.\n");
                    return EXIT_FAILURE;
                }
                zfile_code = std::string("\n// From ") + zfiles.at(i).u8string() + std::string("\n");
                empty_start_lines = true;
                // Just dont add includes
                while (getline(qt, read_line)) {
                    if (in_string) {
                        if (std::regex_match(read_line, matches, reg_string_end)) {
                            in_string = false;
                        }
                    }
                    else if (in_comment) {
                        if (std::regex_match(read_line, matches, reg_comment_end)) {
                            in_comment = false;
                        }
                    }
                    else {
                        if (std::regex_match(read_line, matches, reg_string_start) && !in_comment) {
                            in_string = true;
                        }
                        else if (std::regex_match(read_line, matches, reg_comment_start) && !in_string) {
                            in_comment = true;
                        }
                        else if (std::regex_match(read_line, matches, reg_include)) {
                            continue;
                        }
                    }
                    if (empty_start_lines && streqz(read_line, "")) continue;
                    else empty_start_lines = false;
                    zfile_code += read_line;
                    zfile_code += "\n";
                }
                forward_zcode.emplace_back(zfile_code);
                qt.close();
            }

            // Fix main.cpp
            std::string main_cpp = "//// This file was automatically generated by\n//// "
                                 + ZMAKE_VERSION + ", at " + timestrz() + ".\n\n"
                                 + "//// Includes\n";
            for (unsigned int i = 0; i < include_list.size(); i++) {
                main_cpp += include_list.at(i).at(0);
                // Add soft tabs
                int tabsize = static_cast<int>(include_list.at(i).at(0).length());
                tabsize = 4 - (tabsize % 4);
                for (int j = 0; j < tabsize; j++) main_cpp += " ";

                main_cpp += "// From ";
                for (unsigned int j = 1; j < include_list.at(i).size(); j++) {
                    if (j > 1) main_cpp += ", ";
                    main_cpp += include_list.at(i).at(j);
                }
                main_cpp += "\n";
            }
            main_cpp += "\n//// Default include";
            main_cpp += forward_zcode.at(0) + "\n";
            main_cpp += "//// Structs, classes and unions\n";
            for (unsigned int i = 0; i < forward_structs.size(); i++) {
                main_cpp += forward_structs.at(i) + "\n";
            }
            main_cpp += "//// Functions\n";
            for (unsigned int i = 0; i < forward_functions.size(); i++) {
                main_cpp += forward_functions.at(i) + "\n";
            }
            main_cpp += "//// Code";
            for (unsigned int i = 1; i < forward_zcode.size(); i++) {
                main_cpp += forward_zcode.at(i);
            }

            // Add it to cppfiles (program_name looks like "boo" with quotations)
            std::string open_filename = program_name.substr(1, program_name.length() - 2) + "_zmake.cpp";
            if (!use_build_files) open_filename = "target" + FOLDER_NOTATION + open_filename;
            pt.open(open_filename, std::ios::trunc);
            pt << main_cpp;
            pt.close();
            cppfiles.insert(cppfiles.begin(), open_filename);
        }

        // Fix imports - cppfiles, imports and libraries need to be in unix form
        // Because git bash doesn't understand the compile commands
        std::string libpath_cl = "";
        for (unsigned int i = 0; i < cppfiles.size(); i++) {
            cppfiles.at(i) = "\"" + cppfiles.at(i) + "\"";
        }
        for (int i = static_cast<int>(cfg_libs.size() - 1); i >= 0; i--) {
            std::string temp_str = std::filesystem::absolute(cfg_libs.at(static_cast<unsigned int>(i))).u8string();
            temp_str = "\"" + temp_str + "\"";
            if (!streqz(cfg_libcommands.at(static_cast<unsigned int>(i)), "")) temp_str += " " + cfg_libcommands.at(static_cast<unsigned int>(i));
            if (ends_with(compiler, "cl")) libpath_cl = " -libpath:" + temp_str + libpath_cl;
            else commands.insert(commands.begin(), "-L" + temp_str);
        }
        for (int i = static_cast<int>(cfg_includes.size() - 1); i >= 0; i--) {
            std::string temp_str = std::filesystem::absolute(cfg_includes.at(static_cast<unsigned int>(i))).u8string();
            temp_str = "\"" + temp_str + "\"";
            if (streqz(cfg_inccommands.at(static_cast<unsigned int>(i)), "-w", "-W")) {
                if (streqz(compiler, "clang-cl")) temp_str = "-Xclang -isystem" + temp_str;
                else temp_str = "-isystem" + temp_str;
            }
            else temp_str = "-I" + temp_str;
            commands.insert(commands.begin(), temp_str);
        }
        // Put them into the commands
        for (unsigned int i = 0; i < cppfiles.size(); i++) {
            commands.insert(commands.begin(), cppfiles.at(i));
        }

        // Fix cversion and compiler flags
        if (ends_with(compiler, "cl")) cversion = "-std:" + cversion;
        else cversion = "-std=" + cversion;
        commands.insert(commands.begin(), cversion);

        if (!has_output_flag) commands.emplace_back("-o");
        std::string compilation_string = compiler;

        // Replace flags, -fexceptions and -pedantic
        for (unsigned int i = 0; i < commands.size(); i++) {
            if (streqz(commands.at(i), "")) continue;
            if (ends_with(compiler, "cl")) {
                if (streqz(commands.at(i), "-fexceptions")) {
                    // No duplicates
                    if (str_is_in_vec("-EHsc", commands)) {
                        commands.erase(commands.begin() + i);
                        i--;
                        continue; // To not confuse the compilation_string
                    }
                    else commands.at(i) = "-EHsc";
                }
                else if (streqz(commands.at(i), "-O0")) {
                    // No duplicates
                    if (str_is_in_vec("-Od", commands)) {
                        commands.erase(commands.begin() + i);
                        i--;
                        continue; // To not confuse the compilation_string
                    }
                    else commands.at(i) = "-Od";
                }
            }
            if (streqz(commands.at(i), "-pedantic")) {
                if (str_is_in_vec("-Wpedantic", commands)) {
                    commands.erase(commands.begin() + i);
                    i--;
                    continue; // To not confuse the compilation_string
                }
                else commands.at(i) = "-Wpedantic";
            }
            compilation_string += " " + commands.at(i);
        }

        // Add build profile to program_name and fix compile target
        program_name = program_name.substr(0, program_name.length()-1) + "_" + build_profile + "\"";
        std::string target_name = program_name;
        if (!use_build_files) target_name = "\"target" + FOLDER_NOTATION + target_name.substr(1);

        compilation_string += " " + target_name;
        if (!streqz(libpath_cl, "")) compilation_string += " -link" + libpath_cl;

        if (!use_no_cmd) {
            printz("- Compiling ", program_name, " with the following:\n", compilation_string, "\n\n");
        }

        // Compile
        auto b = std::chrono::steady_clock::now();
        system(compilation_string.c_str());
        auto c = std::chrono::steady_clock::now();

        std::chrono::duration<double, std::milli> fp_zmake = b - a;
        std::chrono::duration<double, std::milli> fp_compiler = c - b;

        printz("\n");

        if (!use_no_time) {
            printz("- zmake took ", fp_zmake.count(), " ms, ", compiler, " took ", fp_compiler.count(), " ms.\n");
        }

        // Open the program
        if (!use_no_run) {
            printz("- Opening ", program_name, ":\n\n");
            system(target_name.c_str());
            return EXIT_SUCCESS;
        }
        else return EXIT_SUCCESS;
    }

    printz("- How did you end up here?\n");
    return EXIT_FAILURE;
}
