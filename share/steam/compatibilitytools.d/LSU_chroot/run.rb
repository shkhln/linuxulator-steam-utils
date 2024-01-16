#!/usr/bin/env ruby
# encoding: UTF-8

def run(args)

  if `sysctl -nq security.bsd.unprivileged_chroot`.to_i != 1
    STDERR.puts "This script requires security.bsd.unprivileged_chroot=1."
    exit(1)
  end

  mroot = File.join(ENV['HOME'], '.steam/mnt')

  ENV['LSU_LINUX_LD_PRELOAD']      = ['shmfix.so', ENV['LSU_LINUX_LD_PRELOAD']].compact.join(':')
  ENV['LSU_LINUX_LD_LIBRARY_PATH'] = ENV['LSU_LINUX_LD_LIBRARY_PATH'].gsub(/[^:]+steam-runtime[^:]+:/, '')
  ENV['LSU_LINUX_PATH']            = "/bin"

  wrapper = case ENV['LSU_DEBUG']
    when 'ktrace'
      ['/usr/bin/ktrace', '-i', '-d']
    else
      []
  end

  # yes, Linux /bin/sh receives FreeBSD LD_* and PATH vars here, fortunately that is mostly benign
  exec(*wrapper, '/usr/sbin/chroot', '-n', mroot,
    '/bin/sh', '-c', ". \"#{File.expand_path("../../../../bin/lsu-freebsd-to-linux-env", __dir__)}\" && cd \"$1\" && shift && exec \"$@\"", '-', ENV['PWD'], *args)
end

case ARGV[0]
  when 'waitforexitandrun'
    run(ARGV[1..-1])
  else
    puts "Unknown command #{ARGV[0]}"
    exit(1)
end
