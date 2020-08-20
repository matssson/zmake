# On Linux set GCC
# If you don't have make:
# clang-cl -std:c++17 -Wall -Wextra -Wpedantic -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -EHsc src/zmake.cpp -Ofast -march=native -o zmake
# g++ -std=c++17 -Wall -Wextra -Wpedantic -fexceptions src/zmake.cpp -Ofast -march=native -o zmake
USE_GCC=
USE_FAST=true

COMPILER=clang-cl
FLAGS=-std:c++17 -Wall -Wextra -Wpedantic -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic -EHsc
FAST=

ifdef USE_GCC
COMPILER=g++
FLAGS=-std=c++17 -Wall -Wextra -Wpedantic -fexceptions
endif

ifdef USE_FAST
FAST=-Ofast -march=native
endif

zmake:
	$(COMPILER) $(FLAGS) src/zmake.cpp $(FAST) -o zmake

clean:
	rm -rf zmake.exe zmake