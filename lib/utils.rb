require 'json'

def find_steamapp_dir(name)
  library_folders = [File.join(ENV['HOME'], '.steam/steam')]

  vdf = File.read(File.join(ENV['HOME'], '.steam/steam/steamapps/libraryfolders.vdf'))
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
