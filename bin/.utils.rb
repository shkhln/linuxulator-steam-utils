require 'json'

LSU_TMPDIR_PATH = File.join(ENV['HOME'], '.steam/tmp')
STEAM_ROOT_PATH = File.join(ENV['HOME'], '.steam/steam')

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

def mount(fs, from, to, options = nil)
  if fs == 'nullfs' && File.file?(from)
    FileUtils.touch(to)
  else
    FileUtils.mkdir_p(to)
  end
  cmd = ['mount']
  if options
    cmd << '-o' << options
  end
  cmd << '-t' << fs
  cmd << from
  cmd << to
  print_cmd(cmd)
  system(*cmd) || raise
  File.realpath(to)
end

def try_mount(fs, from, to, options = nil)
  mount(fs, from, to, options) rescue nil
end

def download_debs(dpkgs, dist_path)
  queue = Queue.new
  dpkgs.each do |pkg, meta|
    queue << [pkg, meta] if !File.exist?(File.join(dist_path, pkg))
  end
  queue.close

  return if queue.empty?

  FileUtils.mkdir_p(dist_path)

  if !File.exist?(File.join(dist_path, 'mirrors.txt'))
    system('fetch', '-o', File.join(dist_path, 'mirrors.txt'), 'http://mirrors.ubuntu.com/mirrors.txt')
  end

  mirrors = begin
    File.readlines(File.join(dist_path, 'mirrors.txt'), chomp: true)
  rescue
    ['https://mirrors.kernel.org/ubuntu', 'https://nl.archive.ubuntu.com/ubuntu']
  end
  mirrors.shuffle! # ?

  threads = []
  4.times do
    threads <<
      Thread.new do
        begin
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
            raise if !downloaded
            FileUtils.mv(File.join(dist_path, "#{pkg}.tmp"), File.join(dist_path, pkg))
          end
        rescue
          next false
        end
        true
      end # thread
  end

  results = threads.map do |thread|
    thread.join.value
  end

  raise if !results.reduce{|r, a| r && a}
end

def extract_debs(dpkgs, dist_path, target_path)
  queue = Queue.new
  dpkgs.each do |e|
    queue << e
  end
  queue.close

  threads = []
  [`sysctl -nq hw.ncpu`.to_i, 16].min.times do
    threads <<
      Thread.new do
        begin
          while e = queue.deq
            pkg, meta = e
            sha256 = IO.popen(['sha256', '-q', File.join(dist_path, pkg)]){|io| io.read.chomp}
            if sha256 != meta[:sha256]
              STDERR.puts "Wrong checksum #{sha256} for #{pkg}, expected #{meta[:sha256]}."
              raise
            end
            system('sh', '-c', 'tar --to-stdout -xf "$0" data.tar.zst | tar --cd "$1" -x',
              '-', File.join(dist_path, pkg), target_path) || raise
          end
        rescue
          next false
        end
        true
      end # thread
  end

  results = threads.map do |thread|
    thread.join.value
  end

  raise if !results.reduce{|r, a| r && a}
end
