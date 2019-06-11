TEMPLATE = subdirs
SUBDIRS = isphlib console
CONFIG += ordered

###########################################

isphlib.file = ./isphlib/isphlib.pro

console.file = ./tools/console/console.pro
console.depends = isphlib

###########################################

OTHER_FILES += README COPYING AUTHORS

