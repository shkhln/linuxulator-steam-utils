#!/usr/bin/env ruby

DT_INIT       = 12
DT_FINI       = 13
DT_INIT_ARRAY = 25
DT_FINI_ARRAY = 26

def to_number(slice)
  case slice.length
    when 8 then slice.unpack('Q<')[0]
    when 4 then slice.unpack('L<')[0]
    else
      raise
  end
end

def to_slice(number, wordsize)
  case wordsize
    when 8 then [number].pack('Q<')
    when 4 then [number].pack('L<')
    else
      raise
  end
end

def read_dynamic_section(obj, section_offset, wordsize)
  entries = []

  i = 0
  while true
    pos = section_offset + wordsize * 2 * i
    tag = to_number(obj[ pos            ...(pos + wordsize    )])
    val = to_number(obj[(pos + wordsize)...(pos + wordsize * 2)])

    entries << {offset: pos, tag: tag, val: val}

    break if tag == 0

    i += 1
  end

  entries
end

def wwrite(obj, offset, number, wordsize)
  pos = offset
  for char in to_slice(number, wordsize).chars
    obj[pos] = char
    pos += 1
  end
end

def patch_init(path, out_path)

  headers = `readelf --headers --wide "#{path}"`

  headers =~ /Class:\s+(ELF32|ELF64)/
  wordsize = $1 == 'ELF64' ? 8 : 4

  headers =~ /.dynamic\s+DYNAMIC\s+\w+\s(\w+)/
  dynamic_section_offset = $1.to_i(16)

  obj = IO.binread(path)

  for entry in read_dynamic_section(obj, dynamic_section_offset, wordsize)
    case entry[:tag]
      when DT_INIT, DT_INIT_ARRAY #, DT_FINI, DT_FINI_ARRAY
        wwrite(obj, entry[:offset], 0x42420000 + entry[:tag], wordsize)
    end
  end

  IO.binwrite(out_path, obj)
end

patch_init(ARGV[0], ARGV[1])
