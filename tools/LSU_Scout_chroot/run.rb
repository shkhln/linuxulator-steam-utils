#!/usr/bin/env ruby
# encoding: UTF-8

SLR_DIR = 'SteamLinuxRuntime_sniper'

case ARGV[0]
  when 'waitforexitandrun'
    #TODO: what should we do with pins?
    steam_runtime_libs = ENV['LSU_LINUX_LD_LIBRARY_PATH'].split(':')
      .find_all{|path| path =~ /\/steam-runtime\// && !(path =~ /\/pinned_libs_(32|64)/)}

    library_path = [
      File.expand_path('../../lib32/fakepulse',   __dir__),
      File.expand_path('../../lib64/fakepulse',   __dir__),
      File.expand_path('../../lib32/fakeudev',    __dir__),
      File.expand_path('../../lib64/fakeudev',    __dir__),
      File.expand_path('../../lib32/noepollexcl', __dir__),
      File.expand_path('../../lib64/noepollexcl', __dir__),
      File.expand_path('../../lib32/shmfix',      __dir__),
      File.expand_path('../../lib64/shmfix',      __dir__),
      '/lib/i386-linux-gnu',
      '/lib/x86_64-linux-gnu',
      '/usr/lib/i386-linux-gnu',
      '/usr/lib/x86_64-linux-gnu',
      '/usr/lib/x86_64-linux-gnu/nss',
      steam_runtime_libs,
      ENV['STEAM_COMPAT_INSTALL_PATH'] # ?
    ].compact.join(':')

    ENV['LSU_LINUX_LD_LIBRARY_PATH'] = library_path
    ENV['LSU_LINUX_PATH']            = '/bin:/usr/bin'

    exec(File.expand_path('../../bin/lsu-run-in-chroot', __dir__), SLR_DIR, *ARGV[1..-1])
  else
    puts "Unknown command #{ARGV[0]}"
    exit(1)
end
