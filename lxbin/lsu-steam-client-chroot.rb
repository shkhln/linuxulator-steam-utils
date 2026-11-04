#!/usr/bin/env ruby
# encoding: UTF-8

raise if !(ARGV[0] == '--' && ARGV[1] =~ /steamrt64\/steam$/)

require 'fileutils'
require_relative '../bin/.utils'

slr_sniper_path = File.join(STEAM_ROOT_PATH, 'steamrt64/pv-runtime/steam-runtime-steamrt')
if !slr_sniper_path
  perr "Unable to find steam-runtime"
  exit(1)
end

bin_path = [
  File.join(LSU_IN_CHROOT, 'lxbin-chroot'),
  '/bin'
].compact.join(':')

library_path = [
  File.join(LSU_IN_CHROOT, 'lib64/steam'),
  File.join(LSU_IN_CHROOT, 'lib64/fakenm',    __dir__), # ?
  File.join(LSU_IN_CHROOT, 'lib64/fakepulse', __dir__),
  File.join(LSU_IN_CHROOT, 'lib64/fakeudev',  __dir__),
  File.join(LSU_IN_CHROOT, 'lib64'),
  File.join(STEAM_ROOT_PATH, 'steamrt64'),
  '/lib/x86_64-linux-gnu',
  '/lib/x86_64-linux-gnu/nss',
].join(':')

preload = [
  'steamfix.so',
  'libSegFault.so',
  ENV['STEAM_LD_PRELOAD']
].compact.join(':')

ENV['LSU_LINUX_LD_LIBRARY_PATH'] = library_path
ENV['LSU_LINUX_LD_PRELOAD']      = preload
ENV['LSU_LINUX_PATH']            = bin_path

ENV['STEAM_EXTRA_COMPAT_TOOLS_PATHS'] =
  [File.join(LSU_IN_CHROOT, 'tools'), ENV['STEAM_EXTRA_COMPAT_TOOLS_PATHS']].compact.join(':')

exec(File.expand_path('../bin/lsu-run-in-chroot', __dir__), slr_sniper_path, *ARGV[1..-1])
