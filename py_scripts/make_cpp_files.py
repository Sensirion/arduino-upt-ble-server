import os

# Create a .cpp file for all the .ino files in the examples dir (CAUTION: will overwrite existing)
print("PRE_SCRIPT: Copying .ino file contents to .cpp files.")
os.system(f'find examples -type f -name "*.ino" -exec bash -c \'for file; do cp "$file" "$(dirname "$file")/$(basename "$file" .ino).cpp"; done\' _ {{}} +')
