#!/usr/bin/env ruby
# encoding: UTF-8

require_relative '../../bin/.utils'

SLR_DIR = 'SteamLinuxRuntime_sniper'

case ARGV[0]
  when 'waitforexitandrun'
    library_path = [
      File.join(LSU_IN_CHROOT, 'lib32/fakepulse'),
      File.join(LSU_IN_CHROOT, 'lib64/fakepulse'),
      File.join(LSU_IN_CHROOT, 'lib32/fakeudev'),
      File.join(LSU_IN_CHROOT, 'lib64/fakeudev'),
      File.join(LSU_IN_CHROOT, 'lib32'),
      File.join(LSU_IN_CHROOT, 'lib64'),
      '/lib/i386-linux-gnu',
      '/lib/x86_64-linux-gnu',
      '/lib/x86_64-linux-gnu/nss',
      ENV['STEAM_COMPAT_INSTALL_PATH'] # ?
    ].compact.join(':')

    ENV['LSU_LINUX_LD_LIBRARY_PATH'] = library_path
    ENV['LSU_LINUX_PATH']            = '/bin'
    ENV['STEAM_COMPAT_TOOL_PATHS']   = nil

    exec(File.expand_path('../../bin/lsu-run-in-chroot', __dir__), SLR_DIR, *ARGV[1..-1])
  else
    perr "Unknown command #{ARGV[0]}"
    exit(1)
end
