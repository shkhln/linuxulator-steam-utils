#!/usr/bin/env ruby
# encoding: UTF-8

require_relative '../../bin/.utils'

PROTON_DIR = 'Proton 8.0'
SLR_DIR    = 'SteamLinuxRuntime_sniper'

def run(args)
  proton_path = find_steamapp_dir(PROTON_DIR)
  if not proton_path
    STDERR.puts "Can't find #{PROTON_DIR}."
    exit(1)
  end

  ENV['LSU_LINUX_LD_PRELOAD']      = ['protonfix.so', ENV['LSU_LINUX_LD_PRELOAD']].compact.join(':')
  ENV['LSU_LINUX_LD_LIBRARY_PATH'] = ENV['LSU_LINUX_LD_LIBRARY_PATH'].gsub(/[^:]+steam-runtime[^:]+:/, '')
  ENV['LSU_LINUX_PATH']            = '/bin:/usr/bin'
  ENV['PROTON_NO_ESYNC']           = '1'
  ENV['PROTON_NO_FSYNC']           = '1'

  exec(File.expand_path('../../bin/lsu-run-in-chroot', __dir__), SLR_DIR, File.join(proton_path, 'proton'), *args)
end

case ARGV[0]
  when 'waitforexitandrun'
    run(ARGV[0..-1])
  else
    puts "Unknown command #{ARGV[0]}"
    exit(1)
end
