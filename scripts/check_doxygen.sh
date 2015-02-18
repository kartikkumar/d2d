#!/bin/bash
set -ev

# Count the number of warnings when building Doxygen output.
warnings=$(make doxygen_docs 2>&1 >/dev/null | wc -l)
if [[ $warnings -ne 0 ]]; then
  # Print the output.
  make doxygen_docs
  echo "Found Doxygen warnings"
  exit 1
fi;