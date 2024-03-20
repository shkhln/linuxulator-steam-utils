#!/usr/bin/env ruby
# encoding: UTF-8

require 'fileutils'
require_relative '../bin/.utils'

slr_sniper_path = find_steamapp_dir('SteamLinuxRuntime_sniper') rescue nil
if !slr_sniper_path
  # using bootstrap runtime instead
  slr_sniper_path = File.join(STEAM_ROOT_PATH, 'ubuntu12_64/SteamLinuxRuntime_sniper')
  #TODO: unpack on version change as well?
  if !File.exist?(slr_sniper_path)
    tmp_slr_path = "#{slr_sniper_path}.tmp"
    FileUtils.mkdir_p(tmp_slr_path)
    system('tar', '--cd', tmp_slr_path, '-xf', File.join(STEAM_ROOT_PATH, 'ubuntu12_64/steam-runtime-sniper.tar.xz'),
      '--strip-components', '1', 'SteamLinuxRuntime_sniper') || raise
    FileUtils.mv(tmp_slr_path, slr_sniper_path)
  end
end

library_path = [
  File.expand_path('../lib64/fakepulse', __dir__),
  File.expand_path('../lib64/fakeudev',  __dir__),
  File.expand_path('../lib64/webfix',    __dir__),
  '/lib/x86_64-linux-gnu',
  '/lib/x86_64-linux-gnu/nss',
].compact.join(':')

ENV['LSU_LINUX_LD_LIBRARY_PATH'] = library_path
ENV['LSU_LINUX_PATH']            = '/bin'

# I don't know whose sick idea was to enable EGL and Vulkan probing on --disable-gpu, but it's not funny
args = ARGV.find_all{|arg| arg != '--disable-gpu-compositing' && arg != '--disable-gpu'}
args << '--use-gl=disabled'

exec(File.expand_path('../bin/lsu-run-in-chroot', __dir__), slr_sniper_path, *args)
