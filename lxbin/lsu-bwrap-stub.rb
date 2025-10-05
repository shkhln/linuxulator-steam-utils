#!/usr/bin/env ruby
# encoding: UTF-8

PROTON_APP_IDS = [
  '1420170', # Proton 5.13
  '1580130', # Proton 6.3
  '1887720', # Proton 7.0
  '2348590', # Proton 8.0
  '2805730', # Proton 9.0
  '3658110'  # Proton 10.0 (Beta)
]

# This is just Steam being stupid (vs the user), so no warning
exit(1) if PROTON_APP_IDS.include?(ENV['STEAM_COMPAT_APP_ID'])

name = File.basename($PROGRAM_NAME)
msg  = "Bubblewrap doesn't work on FreeBSD." +
  " Select LSU chroot or Legacy Runtime in the game compatibility settings."

system('zenity', '--error', '--modal', '--title', name, '--text', msg) ||
  STDERR.puts("\e[7m#{name}: #{msg}\e[27m")

exit(1)
