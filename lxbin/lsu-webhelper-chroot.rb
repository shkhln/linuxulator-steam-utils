#!/usr/bin/env ruby
# encoding: UTF-8

require 'fileutils'
require_relative '../bin/.utils'

slr_sniper_path = File.join(STEAM_ROOT_PATH, 'steamrt64/pv-runtime/steam-runtime-steamrt')
slr_sniper_path = File.join(STEAM_ROOT_PATH, 'steamrt64/steam-runtime-steamrt') if !File.exist?(slr_sniper_path)
slr_sniper_path = (find_steamapp_dir('SteamLinuxRuntime_sniper') rescue nil)    if !File.exist?(slr_sniper_path)

if !slr_sniper_path
  perr "Unable to find steam-runtime"
  exit(1)
end

library_path = [
  File.join(LSU_IN_CHROOT, 'lib64/fakepulse'),
  File.join(LSU_IN_CHROOT, 'lib64/fakeudev'),
  File.join(LSU_IN_CHROOT, 'lib64'),
  '/lib/x86_64-linux-gnu',
  '/lib/x86_64-linux-gnu/nss',
].compact.join(':')

ENV['LSU_LINUX_LD_LIBRARY_PATH'] = library_path
ENV['LSU_LINUX_PATH']            = '/bin'

# https://github.com/ValveSoftware/steam-for-linux/issues/11488
args = ARGV
args << '--disable-gpu'

exec(File.expand_path('../bin/lsu-run-in-chroot', __dir__), slr_sniper_path, *args)
