#!/bin/bash

BRANCH="$(git rev-parse --abbrev-ref HEAD)"
if [ "$BRANCH" == "master" ]; then
    git clone --depth=5 -b gh-pages https://$GITAUTH@github.com/open62541/open62541-www
    cd open62541-www

    git rm -r -f ./doc/current/*
    cp -r ../../doc_html/* ./doc/current/
    git add -A ./doc/current
    git config --global user.email "open62541-travis-ci@users.noreply.github.com"
    git config --global user.name "Open62541 travis-ci"
    git config --global push.default simple
    git commit -am "updated generated documentation on webpage by travis-ci [ci skip]"
    git push https://$GITAUTH@github.com/open62541/open62541-www
    cd .. && rm -rf open62541-www
fi
