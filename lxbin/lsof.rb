#!/usr/local/bin/ruby
# encoding: UTF-8

args = ARGV.join(' ')

if !(args =~ /-P -F upnR -i TCP@127\.0\.0\.1:(\d+)/)
  require_relative '../bin/.utils'
  perr "Unpexted args: #{args.inspect}"
  exit(1)
end

# we don't care about returning correct information to Steam,
# we just need it to fuck off

dest = $1

PID_FILE = File.join(ENV['HOME'], '.steam/steam.pid')

pid = begin
  File.read(PID_FILE).to_i
rescue Errno::ENOENT
  require_relative '../bin/.utils'
  pwarn 'No pid file found'
  0
end

uid = `id -u`.chomp

puts "p#{pid}\nR0\nu#{uid}\nnlocalhost:0->localhost:#{dest}\np#{pid}\nR0\nu#{uid}\nnlocalhost:#{dest}->localhost:0"
