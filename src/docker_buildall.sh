#!/bin/bash

# Flag to track if this is the first directory.
is_first_directory=true

# Get the directory where the script was started. We'll use this to return.
START_DIR=$(pwd)

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

    # Return to the starting directory.
    cd "$START_DIR"

    echo "--- Finished building in: $DIR ---"

done < <(find . -type f -name "build.sh" -not -path "./.*/*" -print0)

echo ""
echo "All builds complete."