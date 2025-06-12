#!/bin/bash
# version.sh - Generate version.h with build time and Git info

TOP="${1:-$(pwd)}"
VERSION_H="$TOP/include/version.h"

# date
CURRENT_TIME=$(date -u +"%Y-%m-%d %H:%M:%S")

# Git info
VERSION_CNT=0
VERSION_MAX_CNT=10
VERSION_INFO="unknown mpp version for missing VCS info"
unset VERSION_HISTORY

# Git check
if [ -d "$TOP/.git" ]; then
    if command -v git &> /dev/null; then
        # get current version
        GIT_LOG_FORMAT="%h author: %<(15)%an %cd %s"
        VERSION_INFO=$(git -C "$TOP" log -1 --oneline --date=short --pretty=format:"$GIT_LOG_FORMAT" 2>/dev/null)
        if [ -n "$VERSION_INFO" ]; then
            VERSION_INFO=$(printf "%s" "$VERSION_INFO" | sed 's/"/\\"/g')
        fi
        # get history version
        while [ $VERSION_CNT -lt $VERSION_MAX_CNT ]; do
            log=$(git -C $TOP log HEAD~$VERSION_CNT -1 --oneline --date=short --pretty=format:"$GIT_LOG_FORMAT" 2>/dev/null)
            if [ -n "$log" ]; then
                VERSION_HISTORY[$VERSION_CNT]="$log"
                ((VERSION_CNT++))
            else
                break
            fi
        done
    fi
fi

mkdir -p "$(dirname "$VERSION_H")"

# gen version.h
cat > "$VERSION_H" << EOF
#ifndef __VERSION_H__
#define __VERSION_H__

#define CURRENT_TIME       "$CURRENT_TIME"
#define KMPP_VERSION       "$VERSION_INFO"
#define KMPP_VER_HIST_CNT  $VERSION_CNT

EOF

for i in "${!VERSION_HISTORY[@]}"; do
    printf "#define KMPP_VER_HIST_%d    \"%s\"\n" "$i" "${VERSION_HISTORY[$i]}" >> "$VERSION_H"
done

cat >> "$VERSION_H" << EOF

#endif // __VERSION_H__
EOF

exit 0
