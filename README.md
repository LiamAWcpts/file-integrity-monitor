# FIM - file integrity monitor

this is a command line tool i made in c++ that tells you if files in a folder got changed. it saves a sha256 hash of every file, then later when you run it again it compares and tells you what got modified, added or deleted

its basically a small version of stuff like tripwire or ossec, which are used to catch if someone messed with files on a system (like changing a config file or sneaking in a script)

## how to build it

i built it on ubuntu. you need g++, make and the openssl dev library

```
sudo apt install g++ make libssl-dev
make
```

## how to use it

first make a baseline of the folder you want to watch:

```
./fim init myfolder
```

then whenever you want to check it:

```
./fim check myfolder
```

by default it saves the baseline to baseline.txt and the log to fim.log but you can give it your own names too:

```
./fim check myfolder mybaseline.txt mylog.log
```

if theres stuff you dont want it to watch (like logs that change all the time) you can skip it with -x. you can use it as many times as you want:

```
./fim init myfolder -x logs/ -x '*.tmp' -x notes.txt
./fim check myfolder -x logs/ -x '*.tmp' -x notes.txt
```

heres what it looks like when stuff changes:

```
$ ./fim check watch
[2026-11-22 00:38:31] MODIFIED a.txt
[2026-11-22 00:38:31] DELETED  c.txt
[2026-11-22 00:38:31] ADDED    sub/new.txt
3 change(s) found, saved to fim.log
```

it also gives back an exit code, 0 means nothing changed, 1 means an error, 2 means it found changes. so you could run it with cron every hour or something and have it do stuff when it returns 2

```
0 * * * * /path/to/fim check /path/to/folder /path/to/baseline.txt /path/to/fim.log
```

## how it works

when you run init it goes thru every file in the folder (and the folders inside it) and hashes each one, then writes the hash and the path to the baseline file. it reads files in chunks so really big files dont eat all the memory.

when you run check it loads the old baseline, scans the folder again, and compares the two. if a file was in the old one but isnt there anymore its deleted, if its in both but the hash is different its modified, and if its there now but wasnt before its added

it also skips its own baseline and log files so it doesnt flag itself if you put them in the same folder

files are in src/, main.cpp has the commands and the comparing, baseline.cpp does the scanning and saving/loading, and hash.cpp does the sha256 part with openssl

## stuff i fixed / added

### crash fixes

- the scan used recursive_directory_iterator which throws if a folder goes away while its scanning (like if something deletes a folder right as fim gets to it). nothing caught that so the whole thing just crashed. even the version that doesnt throw just stops the whole scan, so every file after that point would show up as DELETED. now it goes thru the folders by hand, one folder at a time, so if one folder breaks it prints a warning and keeps going with the rest
- the other filesystem calls (is_regular_file, weakly_canonical, is_directory in main) could also throw on weird paths or broken symlinks, so they all use the error code versions now that dont throw
- EVP_MD_CTX_new() can give back null and the code used it anyway. now it checks, and it also checks the other openssl calls. if reading a file breaks partway thru it doesnt hash the half it got anymore
- std::localtime can give back null which would crash strftime, so now it just says "unknown time" if that happens
- if the log file couldnt be opened the alerts just silently didnt get saved, now it warns you
- if saving the baseline failed partway (like the disk is full) it still said "saved", now it actually checks

### CANTREAD

before if a file couldnt be read it just got skipped so check said it was DELETED, which isnt really true and kinda hides whats going on (someone could chmod 000 a file to hide it changing). now it says CANTREAD instead:

```
$ ./fim check watch
cant read watch/b.txt
[2026-11-23 21:06:36] CANTREAD b.txt
1 change(s) found, saved to fim.log
```

### -x exclude option

theres 3 kinds of patterns:

- ends with / skips a whole folder, like `-x logs/`
- starts with * skips anything ending with that, like `-x '*.tmp'`
- anything else has to match the whole path, like `-x notes.txt` or `-x sub/notes.txt`

the paths are relative to the folder youre watching, same as how they show up in the alerts. put quotes around the * ones or the shell will expand them before fim even sees them

if you add a -x on check that you didnt use on init, the files it skips wont show up as DELETED, it just ignores them. but if you use a -x on init and then leave it off on check those files will show up as ADDED, so its best to just use the same ones both times

the -x stuff can go anywhere so the old way of giving the baseline and log names still works:

```
./fim check myfolder mybaseline.txt mylog.log -x logs/
```

the matching is in baseline.cpp (isExcluded) and main.cpp pulls the -x ones out of the args before reading the rest like before

## stuff i want to add / problems

- right now the baseline is just a text file so if someone already has access they could just edit it too. would be better to keep it somewhere read only or sign it
- use inotify so it watches in real time instead of only when you run it
- maybe put the baselines in sqlite
- a folder it cant open still shows its files as DELETED, would be better to say CANTREAD for those too
