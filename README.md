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

## stuff i want to add / problems

- right now the baseline is just a text file so if someone already has access they could just edit it too. would be better to keep it somewhere read only or sign it
- use inotify so it watches in real time instead of only when you run it
- maybe put the baselines in sqlite
- an exclude option for files that change all the time like logs
