#!/usr/bin/env sh

# Identify repository
repo="$(git rev-parse --show-toplevel)"
if [ $? -ne 0 ]; then
    echo "Not standing in a git repository.."
    exit 1
fi
# Add commit msg template
git config commit.template dev/template/commit-template-msg

# Add repository hooks
cp ${repo}/dev/hooks/* ${repo}/.git/hooks/

# Copy over the local sources from lib
mkdir -p ${repo}/.git/local
cp ${repo}/dev/lib/* ${repo}/.git/local
