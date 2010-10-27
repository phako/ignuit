#!/bin/sh

autoreconf -vif $ACLOCAL_FLAGS
./configure $*
