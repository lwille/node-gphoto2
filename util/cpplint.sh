#!/bin/sh

exit_status=0
linter="python util/cpplint.py --root=$(pwd)"

target_files() {
  find src -maxdepth 1 -type f | grep -E "\.(cc|h)$"
}

violation_count=$(target_files | xargs $linter 2>&1 | grep Total | sed -e "s/[^0-9]*\([0-9]*\)/\1/")
if [ $violation_count != "0" ]
then
  echo "Error: Please follow Google's style guidelines"
  target_files | xargs $linter
  exit_status=1
fi

exit $exit_status
