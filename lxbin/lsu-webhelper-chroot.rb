#!/usr/bin/env ruby
# encoding: UTF-8

require 'fileutils'
require_relative '../bin/.utils'

slr_sniper_path = find_steamapp_dir('SteamLinuxRuntime_sniper') rescue nil
if !slr_sniper_path
  # using bootstrap runtime instead
  slr_sniper_path = File.join(STEAM_ROOT_PATH, 'ubuntu12_64/SteamLinuxRuntime_sniper')
  if !File.exist?(slr_sniper_path)
    tmp_slr_path = "#{slr_sniper_path}.tmp"
    FileUtils.mkdir_p(tmp_slr_path)
    system('tar', '--cd', tmp_slr_path, '-xf', File.join(STEAM_ROOT_PATH, 'ubuntu12_64/steam-runtime-sniper.tar.xz'),
      '--strip-components', '1', 'SteamLinuxRuntime_sniper') || raise
    FileUtils.mv(tmp_slr_path, slr_sniper_path)
  end
end

ENV['LSU_LINUX_LD_LIBRARY_PATH'] = ENV['LSU_LINUX_LD_LIBRARY_PATH'].gsub(/[^:]+steam-runtime[^:]+:/, '')
ENV['LSU_LINUX_PATH']            = '/bin:/usr/bin'

exec(File.expand_path('../bin/lsu-run-in-chroot', __dir__), slr_sniper_path, *ARGV)
