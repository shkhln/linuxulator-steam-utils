require 'json'

raise '$HOME is undefined.' if !ENV['HOME']

HOME_DIR_PATH   = File.realpath(ENV['HOME'])

LSU_IN_CHROOT   = '/usr/steam-utils'
LSU_DIST_PATH   = File.join(HOME_DIR_PATH, '.steam/dist')
LSU_TMPDIR_PATH = File.join(HOME_DIR_PATH, '.steam/tmp')
PID_FILE_PATH   = File.join(HOME_DIR_PATH, '.steam/steam.pid')
STEAM_ROOT_PATH = File.join(HOME_DIR_PATH, '.steam/steam')

LSU_MESA_LIBS   = ENV['LSU_MESA_LIBS'] || 'base' # base, ubuntu, none

PWARN_PROGRAM_NAME = if ENV['LSU_COOKIE'] # ?
  name = File.basename($PROGRAM_NAME)
  name.start_with?('lsu-') ? name : $PROGRAM_NAME
else
  nil
end

def perr(msg)
  if PWARN_PROGRAM_NAME
    STDERR.puts "\e[7m#{PWARN_PROGRAM_NAME}: #{msg}\e[27m"
  else
    STDERR.puts msg
  end
end

def pwarn(msg)
  if PWARN_PROGRAM_NAME
    STDERR.puts "#{PWARN_PROGRAM_NAME}: #{msg}"
  else
    STDERR.puts msg
  end
end

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

def init_tmp_dir
  FileUtils.mkdir_p(LSU_TMPDIR_PATH)
  if try_mount('tmpfs', 'tmpfs', LSU_TMPDIR_PATH, 'nocover')
    File.write(File.join(LSU_TMPDIR_PATH, '.cookie'), "#{ENV['LSU_COOKIE']}\n") if ENV['LSU_COOKIE']
    File.write(File.join(LSU_TMPDIR_PATH, '.setup-done'), '')
  end

  raise if !File.exist?(File.join(LSU_TMPDIR_PATH, '.setup-done'))
end

def read_steam_pid
  begin
    File.read(PID_FILE_PATH).to_i
  rescue Errno::ENOENT
    pwarn 'No pid file found'
    nil
  end
end

def read_tmp_dir_cookie
  begin
    File.read(File.join(LSU_TMPDIR_PATH, '.cookie')).chomp
  rescue Errno::ENOENT
    nil
  end
end

def find_steam_library_folders
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

  library_folders.uniq
end

def find_steamapp_with_library_path(name)
  library_folders = find_steam_library_folders()
  for dir in library_folders
    target = File.join(dir, 'steamapps/common', name)
    if File.exist?(target)
      return [target, dir]
    end
  end
  nil
end

def find_steamapp_dir(name)
  game_dir, _ = find_steamapp_with_library_path(name)
  game_dir
end

def format_cmd(cmd)
  cmd.map{|s| s =~ /\s/ ? s.inspect : s}.join(' ')
end

class MountError < StandardError
end

def mount(fs, from, to, options = nil)
  if !File.exist?(to)
    begin
      if fs == 'nullfs' && File.file?(from)
        FileUtils.touch(to)
      else
        FileUtils.mkdir_p(to)
        FileUtils.touch(File.join(to, '.mountpoint'))
      end
    rescue Errno::EACCES, Errno::EROFS
      # do nothing
    end
  end
  cmd = ['mount']
  if options
    cmd << '-o' << options
  end
  cmd << '-t' << fs
  cmd << from
  cmd << to
  pwarn format_cmd(cmd)
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
          pwarn "Downloading #{pkg} from #{mirror}..."
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
  `sysctl -nq hw.ncpu`.to_i.clamp(1..16).times do
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

class VersionTuple

  include Comparable

  attr :major
  attr :minor
  attr :patch

  def initialize(major, minor, patch)
    @major = major
    @minor = minor
    @patch = patch
  end

  def self.from_str(str)
    if str =~ /^\d+(?:\.\d+(?:\.\d+|)|)$/
      major, minor, patch = str.split('.').map(&:to_i)
      minor ||= 0
      patch ||= 0
      self.new(major, minor, patch)
    else
      raise "#{str} doesn't look like a version string"
    end
  end

  def to_s
    "#{@major}.#{@minor}.#{@patch}"
  end

  def <=>(other)
    result = @major <=> other.major
    result = @minor <=> other.minor if result == 0
    result = @patch <=> other.patch if result == 0
    result
  end
end

def max_symbol_version(library_path, prefix)
  if File.exist?(library_path)
    IO.popen(['readelf', '-s', library_path]) do |io|
      io.read.scan(/@#{prefix}_([\d.]+)(?:\s|$)/).map{|str| VersionTuple.from_str(str[0])}.max
    end
  else
    raise "#{library_path} doesn't exist"
  end
end
