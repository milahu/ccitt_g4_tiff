#!/usr/bin/env bash

set -eux

exec magick "$1" -compress Group4 "$2"
