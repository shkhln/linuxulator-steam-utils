#!/usr/bin/env ruby
# encoding: UTF-8

SLR_DIR = 'SteamLinuxRuntime_sniper'

case ARGV[0]
  when 'waitforexitandrun'
    ENV['LSU_LINUX_PATH'] = '/bin:/usr/bin'
    exec(File.expand_path('../../bin/lsu-run-in-chroot', __dir__), SLR_DIR, *ARGV[1..-1])
  else
    puts "Unknown command #{ARGV[0]}"
    exit(1)
end
