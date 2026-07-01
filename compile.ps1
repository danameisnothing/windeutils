$ErrorActionPreference = "Stop"

# yes, I indeed used an LLM to search for this -mwindows flag.
gcc windeutils.c -o windeutils.exe -lgdi32
strip windeutils.exe