#!/bin/bash

# A bash script to update the file_path inside each file from /.../imp_20 to /.../imp_30 and from _20_lb_data.txt to _30_lb_data.txt

for filename in *_30*; do
  # Check if the file exists and is a regular file
  if [ -f "$filename" ]; then
    # Use sed to replace the file paths inside the file
    # The -i flag edits the file in place.
    # The 's' command substitutes one string for another.
    sed -i 's|/imp_20|/imp_30|g' "$filename"
    sed -i 's|_20_lb_data.txt|_30_lb_data.txt|g' "$filename"
    
    # Optional: Print the action for verification
    echo "Updated file paths inside '$filename'"
  fi
done

echo "Batch text replacement complete."