#!/usr/bin/env bash
set -e

echo "Setting up ESP-IDF environment..."

ACTION=${1:-build}

# Try common locations
ESP_IDF_PATHS=(
    "$HOME/esp/esp-idf/export.sh"
    "/opt/esp/idf/export.sh"
    "/usr/local/esp/esp-idf/export.sh"
)

EXPORT_SCRIPT=""
for path in "${ESP_IDF_PATHS[@]}"; do
    if [ -f "$path" ]; then
        EXPORT_SCRIPT="$path"
        break
    fi
done

if [ -z "$EXPORT_SCRIPT" ]; then
    if [ -n "$IDF_PATH" ] && [ -f "$IDF_PATH/export.sh" ]; then
        EXPORT_SCRIPT="$IDF_PATH/export.sh"
    else
        echo "Error: Could not find ESP-IDF export.sh. Please ensure ESP-IDF is installed."
        exit 1
    fi
fi

echo "Found ESP-IDF at $EXPORT_SCRIPT"
. "$EXPORT_SCRIPT"

if [ "$ACTION" = "build" ]; then
    echo "Building project..."
    idf.py build
elif [ "$ACTION" = "flash" ]; then
    echo "Building and flashing project..."
    idf.py build flash monitor
else
    echo "Running custom idf.py command: idf.py $ACTION"
    idf.py $ACTION
fi
