#!/usr/local/bin/ruby
# encoding: UTF-8

def linux_to_native_path(file)
  if file.start_with?('/') && File.exists?(File.join('/compat/linux', file))
    File.join('/compat/linux', file)
  else
    file
  end
end

options, files = ARGV.partition{|arg| arg.start_with?('-')}

if options.size == 0 || (options.size == 1 && options.include?('-L'))
  for file in files
    print `/usr/bin/file #{options.join(' ')} #{linux_to_native_path(file)}`.gsub('/compat/linux', '')
  end
else
  raise
end
