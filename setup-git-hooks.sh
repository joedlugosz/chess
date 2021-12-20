#!/bin/sh

# Run this script to symlink the hooks in this repo into the .git/hooks directory

ln -sf ../../git/post-commit ./.git/hooks/post-commit
