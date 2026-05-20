#!/bin/bash

# Flag to track if this is the first directory.
is_first_directory=true

# Get the directory where the script was started. We'll use this to return.
START_DIR=$(pwd)

# Output directories
OUTPUT_PPMP="$START_DIR/output_ppmp"
OUTPUT_H="$START_DIR/output_h"

# Create output directories if they don't exist
mkdir -p "$OUTPUT_PPMP"
mkdir -p "$OUTPUT_H"

# Use process substitution to avoid a subshell for the loop.
while IFS= read -r -d '' build_script; do
    # Get the directory where the build.sh was found.
    DIR=$(dirname "$build_script")

    echo ""
    echo "--- Building in: $DIR ---"

    # Go into the target directory.
    cd "$DIR" || continue # Use '|| continue' to skip if cd fails.

    # Check if this is the first directory. This check now happens
    # in the main shell, so the variable change will stick.
    if [ "$is_first_directory" = true ]; then
        echo "This is the first directory. Looking for prebuild.sh..."
        if [ -f "prebuild.sh" ]; then
            ./prebuild.sh
        else
            echo "Warning: prebuild.sh not found in the first directory ($DIR)."
        fi
        # Set the flag to false.
        is_first_directory=false
    fi

    # Run the main build.sh script.
    ./build.sh

    if [ $? -ne 0 ]; then
        echo "Error: build.sh failed with a non-zero exit code in '$DIR'. Aborting." >&2
        cd "$START_DIR"
        exit 1
    fi

    # --- Copy artifacts from the build directory ---
    if [ -d "build" ]; then
        echo "Copying artifacts from build directory..."
        
        # Copy .ppmp files if they exist
        if compgen -G "build/*.ppmp" > /dev/null; then
            cp build/*.ppmp "$OUTPUT_PPMP/"
            echo "  -> .ppmp files copied to output_ppmp/"
        fi
        
        # Copy .h files if they exist
        if compgen -G "build/*.h" > /dev/null; then
            cp build/*.h "$OUTPUT_H/"
            echo "  -> .h files copied to output_h/"
        fi
    else
        echo "Warning: 'build' directory not found in '$DIR'."
    fi

    # Return to the starting directory.
    cd "$START_DIR"

    echo "--- Finished building in: $DIR ---"

done < <(find . -type f -name "build.sh" -not -path "./.*/*" -print0)

echo ""
echo "All builds complete. Artifacts collected."