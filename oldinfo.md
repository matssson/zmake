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
