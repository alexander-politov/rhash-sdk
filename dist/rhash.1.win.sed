#!/bin/sed
# insert encoding options before sfv
/^\.IP "\\-\\-sfv"/ {
i\
.IP "\\-\\-ansi"\
Use Windows codepage for output.\
.IP "\\-\\-oem"\
Use DOS (OEM) codepage for output.\
.IP "\\-\\-utf8"\
Use UTF\\-8 codepage for output.
}

/ looks for a config file/ {
a\
on Windows at\
%APPDATA%\\\\RHash\\\\rhashrc, %HOMEDRIVE%%HOMEPATH%\\\\rhashrc\
\
and on Linux/Unix
}
