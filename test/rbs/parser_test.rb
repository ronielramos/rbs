require "test_helper"

class RBS::ParserTest < Test::Unit::TestCase
  def buffer(source)
    RBS::Buffer.new(content: source, name: "test.rbs")
  end

  def test_interface
    RBS::Parser.parse_signature(buffer(<<-RBS)).tap do |decls|
interface _Foo[unchecked in A]
end
    RBS
      decls[0].tap do |decl|
        pp decl
      end
    end
  end
end
