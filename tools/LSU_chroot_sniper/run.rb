#!/usr/bin/env ruby
# encoding: UTF-8

SLR_DIR = 'SteamLinuxRuntime_sniper'

def run(args)
  ENV['LSU_LINUX_LD_PRELOAD']      = ['shmfix.so', ENV['LSU_LINUX_LD_PRELOAD']].compact.join(':')
  ENV['LSU_LINUX_LD_LIBRARY_PATH'] = ENV['LSU_LINUX_LD_LIBRARY_PATH'].gsub(/[^:]+steam-runtime[^:]+:/, '')
  ENV['LSU_LINUX_PATH']            = '/bin'

  exec(File.expand_path('../../bin/lsu-run-in-chroot', __dir__), SLR_DIR, *args)
end

case ARGV[0]
  when 'waitforexitandrun'
    run(ARGV[1..-1])
  else
    puts "Unknown command #{ARGV[0]}"
    exit(1)
end
