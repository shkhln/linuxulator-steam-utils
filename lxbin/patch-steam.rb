#!/usr/local/bin/ruby
# encoding: UTF-8

require 'fileutils'

def patch_executable(path, out_path)

  if not File.exist?(path)
    STDERR.puts "#{$PROGRAM_NAME}: #{path} not found, nothing to patch"
    return
  end

  def same_size(a, b); File.lstat(a).size == File.lstat(b).size; end
  def is_older (a, b); File.mtime(a)      <  File.mtime(b);      end

  if not (File.exists?(out_path) && same_size(path, out_path) && is_older(path, out_path))
    IO.binwrite(out_path, yield(IO.binread(path)))
    File.chmod(0700, out_path)
  end
end

steam_root = ENV['HOME'] + '/.steam/steam'

for f in ['ubuntu12_32/chromehtml.so', 'ubuntu12_32/steamclient.so', 'ubuntu12_32/steamui.so']
  patch_executable(steam_root + '/' + f, steam_root + '/' + f + '.patched') do |obj|
    obj.gsub!(Regexp.new("\0syscall\0".force_encoding('BINARY'), Regexp::FIXEDENCODING), "\0llacsys\0".force_encoding('BINARY'))
    obj.gsub!(Regexp.new("\0syscall@" .force_encoding('BINARY'), Regexp::FIXEDENCODING), "\0llacsys@" .force_encoding('BINARY'))
    obj
  end
end

patch_executable(steam_root + '/ubuntu12_64/steamwebhelper.sh', steam_root + '/ubuntu12_64/steamwebhelper.sh.patched') do |obj|
  obj.gsub!('"${in_container[@]}"', '')
  obj
end
