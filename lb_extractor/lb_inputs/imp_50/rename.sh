#!/bin/bash

# A bash script to rename files from _20 to _30

for filename in *_40*; do
  # Check if the file exists and is a regular file
  if [ -f "$filename" ]; then
    # Create the new filename by replacing _20 with _30

    # Use sed to replace the file paths inside the file
    # The -i flag edits the file in place.
    # The 's' command substitutes one string for another.
    sed -i 's|/imp_40|/imp_50|g' "$filename"
    sed -i 's|/1.4|/1.5|g' "$filename"
    sed -i 's|_40_lb_data.txt|_50_lb_data.txt|g' "$filename"

    new_filename="${filename//_40/_50}"

    # Rename the file
    mv "$filename" "$new_filename"
    
    # Optional: Print the action for verification
    echo "Renamed '$filename' to '$new_filename'"
  fi
done

echo "Batch renaming complete."