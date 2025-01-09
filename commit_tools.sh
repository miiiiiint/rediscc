#!/usr/bin/bash

#!/bin/bash

# Function to get emoji code based on commit type
get_emoji_code() {
    case $1 in
        "init") echo ":tada:" ;;
        "impr") echo ":art:" ;;
        "perf") echo ":zap:" ;;
        "remove") echo ":fire:" ;;
        "bugfix") echo ":bug:" ;;
        "hotfix") echo ":ambulance:" ;;
        "feat") echo ":sparkles:" ;;
        "docs") echo ":memo:" ;;
        *) echo "" ;;
    esac
}

# Check if commit type and message are provided
if [ -z "$1" ] || [ -z "$2" ]; then
    echo "Usage: $0 <commit_type> <commit_message>"
    echo "Commit types: init, impr, perf, remove, bugfix, hotfix, feat, docs"
    exit 1
fi

# Get the emoji code
emoji_code=$(get_emoji_code $1)

# Commit with the emoji code and message
git commit -m "[$1 $emoji_code]: $2"
