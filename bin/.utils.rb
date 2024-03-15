require 'json'

raise '$HOME is undefined.' if !ENV['HOME']

LSU_IN_CHROOT   = '/usr/steam-utils'
LSU_DIST_PATH   = File.join(ENV['HOME'], '.steam/dist')
LSU_TMPDIR_PATH = File.join(ENV['HOME'], '.steam/tmp')
STEAM_ROOT_PATH = File.join(ENV['HOME'], '.steam/steam')

def with_env(vars)
  temp = {}

  for key in vars.keys
    temp[key] = ENV[key]
  end

  for key in vars.keys
    ENV[key] = vars[key]
  end

  value = yield

  for key in vars.keys
    ENV[key] = temp[key]
  end

  value
end

def with_fbsd_env
  if ENV['LSU_FBSD_PATH']
    env = {
      'PATH'            => ENV['LSU_FBSD_PATH'],
      'LD_LIBRARY_PATH' => ENV['LSU_FBSD_LD_LIBRARY_PATH'],
      'LD_PRELOAD'      => ENV['LSU_FBSD_LD_PRELOAD']
    }
    with_env(env) do
      yield
    end
  else
    yield
  end
end

def init_tmp_dir
  if ENV['LSU_COOKIE']
    with_fbsd_env do
      system(File.join(__dir__, 'lsu-umount'), '--unmount-on-different-cookie') || raise
    end
  end

  FileUtils.mkdir_p(LSU_TMPDIR_PATH)
  if try_mount('tmpfs', 'tmpfs', LSU_TMPDIR_PATH, 'nocover')
    File.write(File.join(LSU_TMPDIR_PATH, '.cookie'), "#{ENV['LSU_COOKIE']}\n") if ENV['LSU_COOKIE']
    File.write(File.join(LSU_TMPDIR_PATH, '.setup-done'), '')
  end

  raise if !File.exist?(File.join(LSU_TMPDIR_PATH, '.setup-done'))
end

def read_tmp_dir_cookie
  cookie_path = File.join(LSU_TMPDIR_PATH, '.cookie')
  File.exist?(cookie_path) ? File.read(cookie_path).chomp : nil
end

def find_steamapp_dir(name)
  library_folders = [STEAM_ROOT_PATH]

  vdf = File.read(File.join(STEAM_ROOT_PATH, 'steamapps/libraryfolders.vdf'))
    .gsub(/"(?=\t+")/, '":').gsub(/"(?=\s+\{)/, '":').gsub(/"(?=\n\t+")/, '",').gsub(/\}(?=\n\t+")/, '},')

  data = JSON.parse("{#{vdf}}")

  for key, value in (data['LibraryFolders'] || data['libraryfolders'])
    if key =~ /^\d+$/
      if value.is_a?(Hash)
        library_folders << value['path']
      else
        library_folders << value
      end
    end
  end

  library_folders.map{|dir| File.join(dir, 'steamapps/common', name)}.find{|dir| File.exist?(dir)}
end

def print_cmd(cmd)
  STDERR.puts cmd.map{|s| s =~ /\s/ ? s.inspect : s}.join(' ')
end

class MountError < StandardError
end

def mount(fs, from, to, options = nil)
  begin
    if fs == 'nullfs' && File.file?(from)
      FileUtils.touch(to)
    else
      FileUtils.mkdir_p(to)
    end
  rescue Errno::EROFS
    # do nothing
  end
  cmd = ['mount']
  if options
    cmd << '-o' << options
  end
  cmd << '-t' << fs
  cmd << from
  cmd << to
  print_cmd(cmd)
  system(*cmd) || raise(MountError.new("unable to mount #{from.inspect} to #{to.inspect}"))
  File.realpath(to)
end

def try_mount(fs, from, to, options = nil)
  mount(fs, from, to, options)
rescue MountError => ex
  nil
end

def download_debs(dpkgs, dist_path)
  queue = Queue.new
  dpkgs.each do |pkg, meta|
    queue << [pkg, meta] if !File.exist?(File.join(dist_path, pkg))
  end
  queue.close

  return if queue.empty?

  FileUtils.mkdir_p(dist_path)

  mirror_list_path = File.join(dist_path, 'mirrors.txt')

  if !File.exist?(mirror_list_path) || File.mtime(mirror_list_path) < Time.now - 3600
    system('fetch', '--no-mtime', '-o', mirror_list_path, 'http://mirrors.ubuntu.com/mirrors.txt')
  end

  mirrors = begin
    File.readlines(mirror_list_path, chomp: true)
  rescue Errno::ENOENT
    []
  end

  # fallback list
  mirrors.concat([
    'https://mirrors.kernel.org/ubuntu',
    'https://nl.archive.ubuntu.com/ubuntu'
  ].shuffle)

  threads = []
  4.times do
    thread = Thread.new do
      while e = queue.deq
        pkg, meta  = e
        downloaded = false
        for mirror in mirrors
          STDERR.puts "Downloading #{pkg} from #{mirror}..."
          if system('fetch', '-o', File.join(dist_path, "#{pkg}.tmp"), "#{mirror}/#{meta[:path]}/#{pkg}")
            downloaded = true
            break
          end
        end
        raise "unable to download #{pkg}" if !downloaded
        FileUtils.mv(File.join(dist_path, "#{pkg}.tmp"), File.join(dist_path, pkg))
      end
    end # thread
    thread.abort_on_exception  = false
    thread.report_on_exception = false
    threads << thread
  end

  threads.each(&:join)
end

def extract_debs(dpkgs, dist_path, target_path)
  queue = Queue.new
  dpkgs.each do |e|
    queue << e
  end
  queue.close

  threads = []
  [`sysctl -nq hw.ncpu`.to_i, 16].min.times do
    thread = Thread.new do
      while e = queue.deq
        pkg, meta = e
        sha256 = IO.popen(['sha256', '-q', File.join(dist_path, pkg)]) do |io|
          value = io.read.chomp
          io.close
          raise "unable to compute checksum of #{pkg}" if !$?.success?
          value
        end
        if sha256 != meta[:sha256]
          raise "wrong checksum #{sha256.inspect} for #{pkg}, expected #{meta[:sha256].inspect}"
        end
        system('sh', '-c', 'set -o pipefail && tar --to-stdout -xf "$0" data.tar.zst | tar --cd "$1" -x',
          '-', File.join(dist_path, pkg), target_path) || raise("unable to extract #{pkg}")
      end
    end # thread
    thread.abort_on_exception  = false
    thread.report_on_exception = false
    threads << thread
  end

  threads.each(&:join)
end
