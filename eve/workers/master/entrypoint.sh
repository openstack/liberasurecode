#!/bin/sh

set -e

buildbot-worker create-worker . \
  "$BUILDMASTER:$BUILDMASTER_PORT" "$WORKERNAME" "$WORKERPASS"

if [ $# -gt 0 -a "$1" = "sh" ]; then
    exec bash
else
    set -- buildbot-worker "$@"
    exec "$@"
fi
