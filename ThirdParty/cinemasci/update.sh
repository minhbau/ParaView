#!/usr/bin/env bash

set -e
set -x
shopt -s dotglob

readonly name="cinemasci"
readonly ownership="Cinemasci Python Upstream <kwrobot@kitware.com>"
readonly subtree="ThirdParty/cinemasci/paraview/tpl"
readonly repo="https://github.com/cinemascience/cinemasci.git"
readonly tag="79c3cdbbc53c4b020191c09b5a37e04f5c807490"

readonly paths="
.gitignore
license.md
cinemasci/__init__.py
cinemasci/cdb/
cinemasci/cis/
cinemasci/pynb/
cinemasci/server/

cinemasci/viewers/.gitignore
cinemasci/viewers/*.html
cinemasci/viewers/*.md
cinemasci/viewers/__init__.py
cinemasci/viewers/cinema/
"

extract_source () {
    git_archive_all

    echo "* -whitespace" > "$extractdir/$name-reduced/.gitattributes"
    echo >> "$extractdir/$name-reduced/cinemasci/viewers/cinema/lib/d3.v4.min.js"
    echo >> "$extractdir/$name-reduced/cinemasci/viewers/cinema/view/2.0/css/range-css.css"
    echo >> "$extractdir/$name-reduced/cinemasci/viewers/.gitignore"

    rm -rfv "$extractdir/$name-reduced/cinemasci/viewers/cinema/testImages"
}

. "${BASH_SOURCE%/*}/../update-common.sh"
