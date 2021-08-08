module RBS
  class Location
    def inspect
      "#<#{self.class}:#{self.__id__} @buffer=#{buffer.name}, @pos=#{start_pos}...#{end_pos}, source='#{source.lines.first}', start_line=#{start_line}, start_column=#{start_column}>"
    end

    def name
      buffer.name
    end

    def start_line
      start_loc[0]
    end

    def start_column
      start_loc[1]
    end

    def end_line
      end_loc[0]
    end

    def end_column
      end_loc[1]
    end

    def start_loc
      @start_loc ||= begin
        _start_loc || buffer.pos_to_loc(start_pos)
      end
    end

    def end_loc
      @end_loc ||= begin
        _end_loc || buffer.pos_to_loc(end_pos)
      end
    end

    def range
      @range ||= start_pos...end_pos
    end

    def source
      @source ||= buffer.content[range] or raise
    end

    def to_s
      "#{name || "-"}:#{start_line}:#{start_column}...#{end_line}:#{end_column}"
    end

    def ==(other)
      other.is_a?(Location) &&
        other.buffer == buffer &&
        other.start_pos == start_pos &&
        other.end_pos == end_pos
    end

    def to_json(state = _ = nil)
      {
        start: {
          line: start_line,
          column: start_column
        },
        end: {
          line: end_line,
          column: end_column
        },
        buffer: {
          name: name&.to_s
        }
      }.to_json(state)
    end

    def self.to_string(location, default: "*:*:*...*:*")
      location&.to_s || default
    end

    def add_required_child(name, range)
      _add_required_child(name, range.begin, range.end)
    end

    def add_optional_child(name, range)
      if range
        _add_optional_child(name, range.begin, range.end)
      else
        _add_optional_no_child(name);
      end
    end
  end
end
