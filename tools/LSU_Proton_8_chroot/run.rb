#!/usr/bin/env ruby
# encoding: UTF-8

require_relative '../../bin/.utils'

PROTON_DIR = 'Proton 8.0'
SLR_DIR    = 'SteamLinuxRuntime_sniper'

def run(args)
  proton_path, steamapp_lib_path = find_steamapp_with_library_path(PROTON_DIR)
  if not proton_path
    pwarn "Can't find #{PROTON_DIR}"
    exit(1)
  end

  library_path = [
    File.join(LSU_IN_CHROOT, 'lib32/fakepulse'),
    File.join(LSU_IN_CHROOT, 'lib64/fakepulse'),
    File.join(LSU_IN_CHROOT, 'lib32/fakeudev'),
    File.join(LSU_IN_CHROOT, 'lib64/fakeudev'),
    File.join(LSU_IN_CHROOT, 'lib32'),
    File.join(LSU_IN_CHROOT, 'lib64'),
    '/lib/i386-linux-gnu',
    '/lib/x86_64-linux-gnu',
    '/lib/x86_64-linux-gnu/nss'
  ].compact.join(':')

  ENV['LSU_LINUX_LD_PRELOAD']       = ['protonfix.so', ENV['LSU_LINUX_LD_PRELOAD']].compact.join(':')
  ENV['LSU_LINUX_LD_LIBRARY_PATH']  = library_path
  ENV['LSU_LINUX_PATH']             = '/bin'
  ENV['PROTON_NO_ESYNC']            = '1'
  ENV['PROTON_NO_FSYNC']            = '1'
  ENV['STEAM_COMPAT_LIBRARY_PATHS'] = [File.join(steamapp_lib_path, 'steamapps'), ENV['STEAM_COMPAT_LIBRARY_PATHS']].compact.uniq.join(':')
  ENV['STEAM_COMPAT_TOOL_PATHS']    = proton_path

  exec(File.expand_path('../../bin/lsu-run-in-chroot', __dir__), SLR_DIR, File.join(proton_path, 'proton'), *args)
end

case ARGV[0]
  when 'waitforexitandrun'
    run(ARGV[0..-1])
  else
    perr "Unknown command #{ARGV[0]}"
    exit(1)
end
