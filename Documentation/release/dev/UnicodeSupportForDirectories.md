# Use wide string Windows API for directory listings

Loading folders and files containing non-ASCII characters on Windows is handled via the wide string Windows API and uses vtksys for the utf8 <-> utf16 conversions.
