#!/usr/bin/env ruby
# encoding: UTF-8

require 'fileutils'
require_relative '../../bin/.utils'

I386_PKG_ROOT = ENV['LSU_i386_PKG_ROOT'] || ENV['WINE_i386_ROOT'] || File.join(ENV['HOME'], '.i386-wine-pkg')

KNOWN_VERSIONS = {
  '7.0' => {appId: 1887720},
  '8.0' => {appId: 2348590}
}

def safe_system(*args)
  raise "Command failed: #{args.join(' ').inspect}" if not system(*args)
end

def set_up_file(path)
  if not File.exist?(path)
    yield path
    if not File.exist?(path)
      raise "Failed to create/download #{path}"
    end
  end
end

def set_up_files(*paths)
  if paths.any?{|p| !File.exist?(p)}
    yield paths
    for p in paths
      raise "Failed to create/download #{p}" if not File.exist?(p)
    end
  end
end

wine64_bin = '/usr/local/wine-proton/bin/wine64'
wine32_bin = File.join(I386_PKG_ROOT, 'usr/local/wine-proton/bin/wine')

if not File.exist?(wine64_bin)
  STDERR.puts "\e[7m"
  STDERR.puts "#{wine64_bin} doesn't exist!"
  STDERR.puts "Install emulators/wine-proton first."
  STDERR.puts "\e[27m"
  exit(1)
end

if not File.exist?(wine32_bin)
  STDERR.puts "\e[7m"
  STDERR.puts "#{wine32_bin} doesn't exist!"
  STDERR.puts "\e[27m"
  exit(1)
end

wine64_version = `#{wine64_bin} --version`.chomp.delete_prefix("wine-")
if not KNOWN_VERSIONS[wine64_version]
  STDERR.puts "\e[7m"
  STDERR.puts "Found #{wine64_bin} version #{wine64_version}, expected#{KNOWN_VERSIONS.keys.size > 1 ? ':' : ''} #{KNOWN_VERSIONS.keys.join(', ')}."
  STDERR.puts "\e[27m"
  exit(1)
end

wine32_version = `#{wine32_bin} --version`.chomp.delete_prefix("wine-")
if wine64_version != wine32_version
  STDERR.puts "\e[7m"
  STDERR.puts "#{wine64_bin} (#{wine64_version}) and #{wine32_bin} (#{wine32_version}) versions must match each other."
  STDERR.puts "\e[27m"
  exit(1)
end

# we expect a PE Wine build
raise if not File.exist?('/usr/local/wine-proton/lib/wine/x86_64-windows')
raise if not File.exist?(File.join(I386_PKG_ROOT, 'usr/local/wine-proton/lib/wine/i386-windows'))

PROTON_VERSION = wine64_version

PROTON_DIR = find_steamapp_dir("Proton #{PROTON_VERSION}")
if not PROTON_DIR
  STDERR.puts "\e[7m"
  STDERR.puts "Can't find the Proton #{PROTON_VERSION} directory!"
  STDERR.puts "Try `steam \"steam://install/#{KNOWN_VERSIONS[PROTON_VERSION][:appId]}\"` if it's not installed."
  STDERR.puts "\e[27m"
  exit(1)
end

def set_up()

  init_tmp_dir()

  $setup_steps = []
  $setup_state = nil
  def set_setup_state(state)
    if state != $setup_state
      case state
        when :proton
          puts "Found Proton #{PROTON_VERSION} at #{PROTON_DIR}"
          puts "Copying files from Proton #{PROTON_VERSION}..."
        when :steamrt
          puts "Found Steam Linux Runtime at #{STEAMRT_DIR}"
          puts "Copying files from Steam Runtime..."
        when :symlinks
          puts "Creating symlinks..."
        when :manifest
          puts "Registering emulators/wine-proton as a compatibility tool..."
        when :done
          if $setup_steps.empty?
            puts "Nothing to do"
          else
            puts "Done"
          end
      end
      if not $setup_steps.include?(state)
        $setup_steps << state
      else
        raise "#{$setup_steps.inspect} + #{state.inspect}"
      end
      $setup_state = state
    end
  end

  Dir.chdir(LSU_TMPDIR_PATH) do

    set_up_file('FreeBSD_Proton') do
      FileUtils.mkdir_p('FreeBSD_Proton')
    end

    Dir.chdir('FreeBSD_Proton') do

      set_up_file("proton_#{PROTON_VERSION}") do |target|
        FileUtils.mkdir_p(target)
      end

      Dir.chdir("proton_#{PROTON_VERSION}") do

        for file in %w(filelock.py user_settings.sample.py LICENSE LICENSE.OFL proton_3.7_tracked_files version)
          set_up_file(file) do
            set_setup_state(:proton)
            FileUtils.cp(File.join(PROTON_DIR, file), '.')
          end
        end

        set_up_file('proton') do |target|
          set_setup_state(:proton)
          FileUtils.cp(File.join(PROTON_DIR, 'proton'), "#{target}.bak")

          str = File.read("#{target}.bak")

          # hijacking stderr/stdout is a bit rude
          raise if not str.gsub!(', stderr=self.log_file, stdout=self.log_file',      '')

          raise if not str.gsub!(/if (self|g_proton)\.need_tarball_extraction\(\)/,   'if False')

          # Proton doesn't know anything about LD_ vs LD_32_ distinction
          raise if not str.gsub!('ld_path_var = "LD_LIBRARY_PATH"',                   'ld_path_var = "__LD_LIBRARY_PATH"')

          raise if not str.gsub!('self.wine_bin = self.bin_dir + "wine"',             'self.wine_bin = "wine"')
          raise if not str.gsub!('self.wine64_bin = self.bin_dir + "wine64"',         'self.wine64_bin = "wine64"')
          raise if not str.gsub!('self.wineserver_bin = self.bin_dir + "wineserver"', 'self.wineserver_bin = "wineserver"')

          raise if not str.gsub!('= find_nvidia_wine_dll_dir()',                      '= None')

          File.write(target, str)
          File.chmod(0700, target)
        end

        set_up_file('dist') do
          set_setup_state(:proton)
          FileUtils.mkdir_p('dist.tmp')
          safe_system('tar', '-C', 'dist.tmp', '-xf', File.join(PROTON_DIR, 'proton_dist.tar'),
            'lib*/libopenxr_loader.so.*',
            'lib*/libsteam_api.so',
            'lib*/vkd3d',
            'lib*/wine/*/*steam*',
            'lib*/wine/*/vrclient*',
            'lib*/wine/*/wineopenxr.dll.so',
            'lib*/wine/dxvk',
            'lib*/wine/nvapi',
            'lib*/wine/vkd3d-proton',
            'share/default_pfx',
            'share/fonts',
            'share/wine/fonts',
            'share/wine/wine.inf')
          FileUtils.mv('dist.tmp', 'dist')
        end

        Dir.chdir('dist/lib/wine/i386-windows') do
          for file in Dir[File.join(I386_PKG_ROOT, 'usr/local/wine-proton/lib/wine/i386-windows/*.{cpl,dll,drv,exe}')]
            if not File.exist?(File.basename(file))
              set_setup_state(:symlinks)
              safe_system('ln', '-s', file)
            end
          end
        end

        Dir.chdir('dist/lib64/wine/x86_64-windows') do
          for file in Dir['/usr/local/wine-proton/lib/wine/x86_64-windows/*.{cpl,dll,drv,exe}']
            if not File.exist?(File.basename(file))
              set_setup_state(:symlinks)
              safe_system('ln', '-s', file)
            end
          end
        end

        set_up_file('dist/lib/gstreamer-1.0') do |target|
          set_setup_state(:symlinks)
          safe_system('ln', '-sf', '-h', File.join(I386_PKG_ROOT, 'usr/local/lib/gstreamer-1.0'), target)
        end

        set_up_file('dist/lib64/gstreamer-1.0') do |target|
          set_setup_state(:symlinks)
          safe_system('ln', '-sf', '-h', '/usr/local/lib/gstreamer-1.0', target)
        end
      end

    end # FreeBSD_Proton
  end # compatibilitytools.d

  set_setup_state(:done)
end

def run(args)
    ENV['LD_32_LIBRARY_PATH'] = File.join(LSU_TMPDIR_PATH, "FreeBSD_Proton/proton_#{PROTON_VERSION}/dist/lib")
    ENV['LD_LIBRARY_PATH']    = File.join(LSU_TMPDIR_PATH, "FreeBSD_Proton/proton_#{PROTON_VERSION}/dist/lib64")

    ENV['DISABLE_VK_LAYER_VALVE_steam_overlay_1'] = '1' # avoids ubuntu12_32/steamoverlayvulkanlayer.so crash
    ENV['DXVK_HUD']  ||= 'frametimes,version,devinfo,fps'
    ENV['WINEDEBUG'] ||= 'warn+module,warn+seh'

    cmd = [
      ENV['LSU_SHIM_WRAPPER'] || 'with-glibc-shim',
      File.expand_path('../../bin/lsu-wine-env', __dir__),
      File.join(LSU_TMPDIR_PATH, "FreeBSD_Proton/proton_#{PROTON_VERSION}/proton"),
      *args
    ]
    print_cmd(cmd)
    exec(*cmd)
end

case ARGV[0]
  when 'waitforexitandrun'
    set_up()
    run(ARGV[0..-1])
  else
    puts "Unknown command #{ARGV[0]}"
    exit(1)
end
