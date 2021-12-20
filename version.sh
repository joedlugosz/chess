#!/bin/sh

# Generate a header file with version and build info

echo ''

echo '/* This file is generated by version.sh */'
echo ''

echo '#ifndef VERSION_H'
echo '#define VERSION_H'
echo ''

echo -n '#define GIT_VERSION "'
git rev-parse HEAD | tr -d '\n'
echo '"'

echo -n '#define OS_NAME "'
uname -o | tr -d '\n'
echo '"'
echo '#define OS POSIX'

echo -n '#define TARGET_NAME "'
uname -p | tr -d '\n'
echo '"'

echo ''
echo '#endif'
