#!/usr/bin/env ruby
# encoding: UTF-8

require 'fileutils'

#TODO: what about non-bootstrap sniper runtime?
Dir.chdir(File.join(ENV['HOME'], '.steam/steam/ubuntu12_64')) do
  if !File.exist?('steam-runtime-sniper')
    system('tar -xf steam-runtime-sniper.tar.xz') || raise
    FileUtils.ln_s("SteamLinuxRuntime_sniper", 'steam-runtime-sniper') # ?
  end
end

SLR_PATH = File.join(ENV['HOME'], '.steam/steam/ubuntu12_64/SteamLinuxRuntime_sniper')

ENV['LSU_LINUX_LD_LIBRARY_PATH'] = ENV['LSU_LINUX_LD_LIBRARY_PATH'].gsub(/[^:]+steam-runtime[^:]+:/, '')
ENV['LSU_LINUX_PATH']            = '/bin:/usr/bin'

exec(File.expand_path('../bin/lsu-run-in-chroot', __dir__), SLR_PATH, *ARGV)
