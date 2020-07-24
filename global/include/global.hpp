#include <algorithm>
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
