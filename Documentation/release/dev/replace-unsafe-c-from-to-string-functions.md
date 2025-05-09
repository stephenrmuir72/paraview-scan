## Replace unsafe c from/to string functions

VTK has been using a set of either unsafe or slow C/C++ functions to convert numbers to string or vice versa.
The exhaustive list of functions is given below. And all of them have been replaced with safer alternatives/faster
alternatives provided by scnlib, fmt, and fast_float libraries and exposed though the vtk:: namespace.

C/C++ has the following functions to convert one/many char or string to a number.

1. atof, atoi, atol, atoll,
2. std::stof, std::stod, std::stold, std::stoi, std::stol, std::stoll, std::stoul, std::stoull
3. std::strtof, std::strtod, std::strtold, std::strtol, std::strtoll/_strtoi64, std::strtoul,
   std::strtoull
4. sscanf, sscanf_s, vsscanf, vsscanf_s
5. std::from_chars (This is slow because it does not use fast_float under the hood)

These functions should be replaced by:

1. vtk::from_chars, vtk::scan_int, vtk::scan_value, if one number needs to be converted
2. vtk::scan, if one/many numbers need to be converted (optionally with a specific format)

C/C++ has the following functions to scan one/many numbers from a stdin/file.

1. scanf, scanf_s, vscanf, vscanf_s,
2. fscanf, fscanf_s, vfscanf, vfscanf_s,

These functions should be replaced by:

1. vtk::scan_value, if one number needs to be converted
2. vtk::input, vtk::scan, if one/many numbers need to be converted (optionally with a specific
   format)

C/C++ has the following functions to convert one/many numbers to a char or string.

1. itoa/_itoa, ltoa/_ltoa, lltoa/_i64toa, ultoa/_ultoa, ulltoa/_ulltoa/_ui64toa
2. sprintf, sprintf_s, vsprintf, vsprintf_s,
3. snprintf, snprintf_s, vsnprintf, vsnprintf_s,
4. strftime
5. std::to_chars, std::to_string

These functions should be replaced by:

1. vtk::to_chars or vtk::to_string, if one number needs to be converted
2. vtk::format, vtk::format_to, or vtk::format_to_n, if one/many
   numbers need to be converted with a specific format

C/C++ has the following functions to print one/many numbers to stdout/file.

1. printf, printf_s, vprintf, vprintf_s,
2. fprintf, fprintf_s, vfprintf, vfprintf_s,

These functions should be replaced by:

1. vtk::print, vtk::println

It should also be noted the double-conversion library is no longer a dependency of VTK. It was removed in favor of
using fmt which is faster.
