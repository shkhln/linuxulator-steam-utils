#!/usr/local/bin/ruby
# encoding: UTF-8

require 'fileutils'

def same_size(a, b); File.lstat(a).size == File.lstat(b).size; end
def is_older (a, b); File.mtime(a)      <  File.mtime(b);      end

def patch_executable(path, out_path, check_size = true)

  if not File.exist?(path)
    STDERR.puts "#{$PROGRAM_NAME}: #{path} not found, nothing to patch"
    return
  end

  if !(File.exists?(out_path) && (check_size ? same_size(path, out_path) : true) && is_older(path, out_path) && is_older(__FILE__, out_path))
    IO.binwrite(out_path, yield(IO.binread(path)))
    File.chmod(0700, out_path)
  end
end

def patch_script(path, out_path, &block)
  patch_executable(path, out_path, false, &block)
end

steam_root = ENV['HOME'] + '/.steam/steam'

#patch_script(steam_root + '/ubuntu12_64/steamwebhelper.sh', steam_root + '/ubuntu12_64/steamwebhelper.sh.patched') do |obj|
#  obj.gsub!('mkdir -p "${STEAM_BASE_FOLDER}/logs"', 'mkdir -p "${STEAM_BASE_FOLDER}/logs" || true')
#  obj.gsub!('exec ${in_container[0]+"${in_container[@]}"}', 'exec lsu-webhelper-chroot')
#  obj
#end

if File.exists?(File.join(steam_root, 'ubuntu12_64/steamwebhelper.sh'))
  out_path = File.join(steam_root, 'ubuntu12_64/steamwebhelper.sh.patched')
  if !(File.exists?(out_path) && is_older(__FILE__, out_path))
    script = <<~SCRIPT
    #!/bin/sh
    cd "$(dirname "$(realpath "$0")")" && exec lsu-webhelper-chroot ./steamwebhelper_sniper_wrap.sh "$@" # &>> "${STEAM_BASE_FOLDER}/logs/steamwebhelper.log"
    SCRIPT
    File.write(out_path, script)
    File.chmod(0700, out_path)
  end
end
