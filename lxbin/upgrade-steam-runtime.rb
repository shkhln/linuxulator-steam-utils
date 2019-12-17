#!/usr/bin/env ruby
# encoding: UTF-8

require 'fileutils'

steam_root = ENV['HOME'] + '/.steam/steam'

def with_work_dir(dir)
  pwd = Dir.pwd
  Dir.chdir(dir)
  yield
  Dir.chdir(pwd)
end

with_work_dir(steam_root + '/ubuntu12_32') do

  runtime_md5 = File.exist?('steam-runtime/checksum') ? File.read('steam-runtime/checksum').split(' ').first : nil
  archive_md5 = File.exist?('steam-runtime.checksum') ? File.read('steam-runtime.checksum').split(' ').first : nil

  if archive_md5 && runtime_md5 != archive_md5

    system('cat steam-runtime.tar.xz.part* > steam-runtime.tar.xz')

    if `md5 -q steam-runtime.tar.xz`.chomp != archive_md5
      raise 'steam-runtime.tar.xz failed integrity check'
    end

    version_txt = `tar -xf steam-runtime.tar.xz --to-stdout steam-runtime/version.txt`
    version_txt =~ /_([\d\.]+)$/
    version = $1

    raise if not version

    if not File.exist?("steam-runtime_#{version}")
      puts 'Extracting steam-runtime...'

      FileUtils.mkdir('steam-runtime_' + version)
      system("tar -C steam-runtime_#{version} -xf steam-runtime.tar.xz --strip-components 1")

      FileUtils.cp('steam-runtime.checksum', "steam-runtime_#{version}/checksum")
    end

    FileUtils.rm_r('steam-runtime') if File.exist?('steam-runtime')
    FileUtils.ln_s("steam-runtime_#{version}", 'steam-runtime')
  end

  if not (File.exists?('steam-runtime/pinned_libs_32') && File.exists?('steam-runtime/pinned_libs_64'))
    system("/compat/linux/bin/env PATH=\"#{__dir__}:/compat/linux/bin\" steam-runtime/setup.sh")
  end

  # keep previous 2 versions just in case
  Dir['steam-runtime_*'].sort[0..-3].each do |dir|
    FileUtils.rm_r(dir)
  end
end
