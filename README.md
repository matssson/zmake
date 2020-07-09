# zmake: A Convenient and Customizable Build System for C++ (without Headers)
A build system for C/C++ inspired by Rust's cargo,
meant to be as convenient and simple as possible.

If you use the custom file extension .zpp (or .z) you also get things
to improve the quality of life when coding in C++, like the removal of headers,
and automatically adding things into one file for fast compilation, with forward declarations
so you don't have to worry about the order of functions or includes.

It achieves this by making everything global in scope,
importing macros and functions, and using things from the std namespace -
but all of this is customizable and it works seamlessly with other C/C++ code and headers.

If you want to change what zmake includes by default,
just edit the autoinclude.hpp header in /global. You can also change the
default config this way by changing autoconfig.cfg.

# How to Build
Make a new project with "zmake new project_name".
Then just put the code in the /src folder, if you have any libraries put them in
/include and /lib, or specify them along with other build flags in zmake.cfg,
or put them globally in zmake/global/include and zmake/global/lib.

Build and run the dev build with "zmake run".
Build the release build with "zmake build".
Build the debug build with "zmake debug".
Open the most recently compiled build with "zmake open".
Remove target files with "zmake clean".

You can also add any "-gccflags" at the end of your command
to compile with them, or the following built in commands:
"-dev/-debug/-release" (change build profile),
"-nocmd" (hide compiler command),
"-notime" (hide compilation time),
"-nobuild" (only running),
"-norun" (only building),
"-run" (run after building),
or "-gcc/-clang/-clang++" to change compiler.
On Windows, clang (or clang-cl) compiles with clang-cl, unlike clang++.

# Features
No headers, so you don't have to worry about function prototypes
and which order your functions are in.

To include another file, just type: (NOTE: this is not yet implemented,
right now every .zpp file is just included by default)
```cpp
#include <otherfile.zpp>
```
Note that \<algorithm\>, \<cstdlib\>, \<cstdio\>, \<ctime\>,
\<iostream\>, \<string\> and \<random\> are provided by default.
(Libraries used by the default functions, cstdlib is used in a #define).

The following are in the global namespace:
string, vector.

### Syntactic Sugar
New Command             | Old Command
----------------------- | -----------------------
alloc(num, type)        | (type*) malloc(sizeof(type) * num)
arrlen(array)           | sizeof(array) / sizeof(array[0])
putline(scanner, line)  | scanner << line << "\\n"

### Simplified Types
u8  = unsigned char

u16 = unsigned short

u32 = unsigned long

u64 = unsigned long long

s8  = char

s16 = short

s32 = long

s64 = long long

f32 = float

f64 = double

### Built in Functions
A built in print function that works for any type. It also adds a flushes
the output stream automatically, if you print a lot and don't want to flush
you should use std::cout;
```cpp
print("hello ", 5, " ", variable, " world!\\n");
```
There's also printl() that does the same as the above, but also
adds spaces between agruments and a newline at the end (like the print in Python 3).
```cpp
printl("hello", 5, variable, "world!");
```

A built in rng function that generates random numbers with much better
statistical properties than C's rand() ever could.
Note that it works on multiple threads due to being thread_local.
- rng(int a, int b) returns a random int in the range [a, b],
- rng() returns a random float in the range [0, 1).

```cpp
streq(str, "test 1", "test 2", test_str_etc);
```
Which checks if a string/c_string is equal to any of the other parameters.
```cpp
string timestr(time_t time, char* format);
```
Which turns a date into a string with the current time and Y-M-D H:M:S as standard.

A function trim() that trims a string at both ends from whitespace and returns it.

A function syscall() that takes a system() command,
executes it and returns the output as a string.

# Installing zmake
### Windows
I strongly recommend using clang-cl for Windows development, since it has full
functionality with all the system headers (unlike gcc) and it has better warnings,
and faster performing code than MSVC. To get clang-cl, you first need to install
MSVC by downloading Visual Studio, and then install clang and adding it to PATH
by downloading the latest version of LLVM.

On Windows, download the code to C:\\zmake, and add C:\\zmake to the PATH, by going to:
```
"Edit the system environment variables" ("Redigera systemets miljövariabler")
-> "Environment Variables" -> "Path" -> "Edit" -> "New"
```
And putting in "C:\\zmake", then build it with Clang (or use the precompiled binary):
```
clang-cl -std:c++17 -EHsc -Ofast -march=native src/zmake.cpp -o zmake
```
Now you can just call it from cmd, git bash, or any other terminal of choice!

### Linux
On Linux, download the code to /$HOME/zmake and add the directory to PATH,
by opening the file ".bashrc" in $HOME and adding the line:
```
export PATH=/$HOME/zmake:$PATH
```
This is then run every time you start the computer. To update without restarting, run:
```
source .bashrc
```
Or if you don't wanna add a PATH variable you can create the following alias:
```
alias zmake="/$HOME/zmake/zmake"
```
Then build it with gcc:
```
g++ -std=c++17 -fexceptions -Ofast -march=native src/zmake.cpp -o zmake
```
And you're good to go!

### macOS (and BSDs)
This has not been tested, but download the code to /usr/local/opt/zmake,
and add the directory to your system's PATH. Then compile it with clang (or gcc):
```
clang++ -std=c++17 -fexceptions -Ofast -march=native src/zmake.cpp -o zmake
```
And it should work.

# Other
To make .z and .zpp files automatically detected as C++ code in Atom
for syntax highlighting, Press Ctrl+Shift+P and go to Application: Open Your Config
and under the core extension add:
```
core:
  customFileTypes:
    'source.cpp': [
      'z'
      'zpp'
    ]
```

### Notes
zmake flags work with hyphens or slashes (-dev = /dev).

If you change the compiler when running zmake, the resulting file will be named "\_custom",
and the flags (but not the optimization) from your initial build profile will be ignored.

If you have a "defaultinclude.hpp" in your current working directory when compiling a
zmake project, then that file is used instead of the one in /global.

If you don't want to create a new git project, you can use zmake gl projname (gl = gitless).

You can also compile using simply "zmake file.z" or "zmake file.z -flag name".

If you add -o or not doesn't matter unless you specify -c -S or -E.

Compiler flags:
-pedantic   -> -Wpedantic (just use Wpedantic always)
-save-temps -> -save-temps=obj (not yet implemented)

Clang-cl specific improvements (also applies for MSVC, but support for that compiler is limited overall):
-fexceptions -> -EHsc
-O0          -> -Od     // Clang-cl recognizes -Ofast as -O2 by default, but not -O0.
