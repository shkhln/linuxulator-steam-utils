#!/usr/bin/env ruby
# encoding: UTF-8

name = File.basename($PROGRAM_NAME)
msg  = "Bubblewrap doesn't work on FreeBSD." +
  " Select LSU chroot or Legacy Runtime in the game compatibility settings."

system('zenity', '--error', '--modal', '--title', name, '--text', msg) ||
  STDERR.puts("\e[7m#{name}: #{msg}\e[27m")

exit 1
