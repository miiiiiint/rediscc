#!/bin/bash
echo "Starting clang-format check..."

FILES=$(find . -type f \( -name "*.cpp" -o -name "*.cc" -o -name "*.cxx" -o -name "*.hpp" -o -name "*.h" -o -name "*.hxx" -o -name "*.inc" \) )

HAS_CHANGES=false 
ERROR_MESSAGES="" 

for file in $FILES; do
  echo "Processing file: $file"
  clang-format "$file" > "$file.tmp" 

  if ! diff -q "$file" "$file.tmp" > /dev/null; then 
    HAS_CHANGES=true
    DIFF_OUTPUT=$(diff "$file" "$file.tmp") 
    ERROR_MESSAGES+="\n--- File: $file ---\n$DIFF_OUTPUT"
    echo "Formatting issues detected in: $file"
  fi
  rm "$file.tmp" 
done

if [[ "$HAS_CHANGES" == "true" ]]; then
  echo "::error::Formatting issues found! Please check the diff below:"
  echo "$ERROR_MESSAGES"
  exit 1 
else
  echo "No formatting issues found!"
fi