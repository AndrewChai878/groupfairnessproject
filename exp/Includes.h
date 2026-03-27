#include <iostream>
#include <iomanip>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <math.h>
#include <cmath>
#include <random>
//#include <intrin.h>
// The explanation engine does not use intrinsics directly, so only load the
// x86 headers on x86 targets; they fail to compile on arm64 macOS.
#if defined(__i386__) || defined(__x86_64__)
#include <x86intrin.h>
#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
#include <immintrin.h>
#endif

#define indd int
#define numm double

using namespace std;
