#!/bin/bash
# SPDX-License-Identifier: GPL-2.0

#
# This file is part of Lustre, http://www.lustre.org/
#
# autogen.sh
#
# Run various autotools, thereby creating the configure
# script and top-level make files
#

set -e
pw="$PWD"

# avoid the "modules.order: No such file or directory" failure
touch modules.order

libtoolize -q -f
aclocal -I $pw/config $ACLOCAL_FLAGS
autoheader
automake -a -c
autoconf
