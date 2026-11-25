#!/bin/bash
# tests for fim, builds it and then runs it thru a bunch of cases
# run it from the repo folder: bash test.sh
# gives back 0 if everything passed and 1 if anything failed

cd "$(dirname "$0")" || exit 1

# build it first (make doesnt redo it if nothing changed)
make || { echo "build failed"; exit 1; }
FIM="$(pwd)/fim"

# do everything in a temp folder so it doesnt mess w the repo
T="$(mktemp -d)" || exit 1
trap '[ -n "$T" ] && rm -rf "$T"' EXIT
cd "$T" || exit 1

pass=0
fail=0

# runs fim and checks the exit code and that the output has some text in it
# usage: expect "name" <exit code> "text to look for" fim args...
expect(){
  name="$1"; want="$2"; text="$3"; shift 3
  out=$("$@" 2>&1); code=$?
  if [ "$code" = "$want" ] && echo "$out" | grep -qF -- "$text"; then
    echo "PASS $name"
    pass=$((pass+1))
  else
    echo "FAIL $name (wanted exit $want w \"$text\", got exit $code)"
    echo "$out" | sed 's/^/    /'
    fail=$((fail+1))
  fi
}

# same thing but the text should NOT be in the output
expectNot(){
  name="$1"; want="$2"; text="$3"; shift 3
  out=$("$@" 2>&1); code=$?
  if [ "$code" = "$want" ] && ! echo "$out" | grep -qF -- "$text"; then
    echo "PASS $name"
    pass=$((pass+1))
  else
    echo "FAIL $name (wanted exit $want wo \"$text\", got exit $code)"
    echo "$out" | sed 's/^/    /'
    fail=$((fail+1))
  fi
}

# same thing but there shouldnt be any output at all (for -q)
expectEmpty(){
  name="$1"; want="$2"; shift 2
  out=$("$@" 2>&1); code=$?
  if [ "$code" = "$want" ] && [ -z "$out" ]; then
    echo "PASS $name"
    pass=$((pass+1))
  else
    echo "FAIL $name (wanted exit $want and no output, got exit $code)"
    echo "$out" | sed 's/^/    /'
    fail=$((fail+1))
  fi
}

# makes a fresh folder to watch and gets rid of old baseline n log
fresh(){
  rm -rf w baseline.txt fim.log
  mkdir -p w/sub w/logs
  echo a > w/a.txt
  echo b > w/b.txt
  echo c > w/c.txt
  echo s > w/sub/s.txt
  echo l > w/logs/app.log
  echo t > w/x.tmp
  echo n > w/notes.txt
}


echo "== basic stuff"
fresh
expect "init saves a baseline" 0 "saved baseline w 7 files" "$FIM" init w
expect "check w nothing changed" 0 "no changes" "$FIM" check w

echo changed > w/a.txt
expect "modified file" 2 "MODIFIED a.txt" "$FIM" check w

fresh; "$FIM" init w >/dev/null
rm w/c.txt
expect "deleted file" 2 "DELETED  c.txt" "$FIM" check w

fresh; "$FIM" init w >/dev/null
echo new > w/sub/new.txt
expect "added file in a sub folder" 2 "ADDED    sub/new.txt" "$FIM" check w

fresh; "$FIM" init w >/dev/null
echo changed > w/a.txt
"$FIM" check w >/dev/null
expect "changes go in the log" 0 "MODIFIED a.txt" cat fim.log


echo "== CANTREAD"
# root can read anything so this one only works as a normal user
if [ "$(id -u)" = "0" ]; then
  echo "SKIP cantread (running as root)"
else
  fresh; "$FIM" init w >/dev/null
  chmod 000 w/b.txt
  expect "unreadable file says CANTREAD" 2 "CANTREAD b.txt" "$FIM" check w
  expectNot "unreadable file isnt DELETED" 2 "DELETED  b.txt" "$FIM" check w
  chmod 644 w/b.txt
fi


echo "== -x excludes"
fresh
expect "init w excludes" 0 "saved baseline w 4 files" "$FIM" init w -x logs/ -x '*.tmp' -x notes.txt
echo z >> w/logs/app.log
echo z >> w/x.tmp
echo z >> w/notes.txt
expect "changes to excluded stuff are ignored" 0 "no changes" "$FIM" check w -x logs/ -x '*.tmp' -x notes.txt

fresh; "$FIM" init w >/dev/null
expectNot "adding -x only on check doesnt say DELETED" 0 "DELETED" "$FIM" check w -x sub/

fresh; "$FIM" init w >/dev/null
echo z >> w/a.txt
expect "folder name wo the / works too" 2 "MODIFIED a.txt" "$FIM" check w -x logs
expectNot "folder name wo the / skips the folder" 2 "logs/" "$FIM" check w -x logs


echo "== -q quiet"
fresh; "$FIM" init w >/dev/null
expectEmpty "quiet w no changes" 0 "$FIM" check w -q
echo changed > w/a.txt
rm -f fim.log
expectEmpty "quiet w changes" 2 "$FIM" check w --quiet
expect "quiet still writes the log" 0 "MODIFIED a.txt" cat fim.log
expectEmpty "quiet init" 0 "$FIM" -q init w


echo "== file names n args"
fresh
expect "custom baseline n log names" 0 "saved baseline w 7 files to my.txt" "$FIM" init w my.txt my.log
echo changed > w/a.txt
expect "check w custom names" 2 "saved to my.log" "$FIM" check w my.txt my.log
expect "-x n -q can go anywhere" 0 "" "$FIM" -q check -x a.txt w my.txt my.log

fresh
expect "baseline inside the folder doesnt flag itself" 0 "saved baseline" "$FIM" init w w/bl.txt w/fim.log
expect "check w baseline inside the folder" 0 "no changes" "$FIM" check w w/bl.txt w/fim.log


echo "== errors"
fresh
expect "no args shows usage" 1 "usage" "$FIM"
expect "unknown command shows usage" 1 "usage" "$FIM" blah w
expect "-x wo a pattern" 1 "-x needs a pattern" "$FIM" check w -x
expect "folder that doesnt exist" 1 "isnt a folder" "$FIM" check nope
expect "check before init" 1 "run init first" "$FIM" check w
expect "baseline cant be saved" 1 "couldnt save baseline" "$FIM" init w /nonexistent/dir/b.txt
"$FIM" init w >/dev/null
expect "log cant be opened" 0 "couldnt open log" "$FIM" check w baseline.txt /nonexistent/dir/fim.log
expect "log warning still shows w -q" 0 "couldnt open log" "$FIM" check w baseline.txt /nonexistent/dir/fim.log -q


echo ""
echo "$pass passed, $fail failed"
[ "$fail" = "0" ]
