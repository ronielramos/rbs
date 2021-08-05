module RBS
  class Parser
    def self.parse_type(buffer, line: 1, column: 0)
      _parse_type(buffer, line, column)
    end

    def self.parse_method_type(buffer, line: 1, column: 0)
      _parse_method_type(buffer, line, column)
    end

    def self.parse_signature(buffer, line: 1, column: 0)
      _parse_signature(buffer, line, column)
    end
  end
end
