require "test_helper"

class RBS::ParserTest < Test::Unit::TestCase
  def buffer(source)
    RBS::Buffer.new(content: source, name: "test.rbs")
  end

  def test_interface
    RBS::Parser.parse_signature(buffer(<<-RBS)).tap do |decls|
interface _Foo[unchecked in A]
  def foo: () -> A
         | { () -> void } -> void
end
    RBS
      decls[0].tap do |decl|
        pp decl
      end
    end
  end

  def test_interface_def_singleton_error
    assert_raises do
      RBS::Parser.parse_signature(buffer(<<-RBS)).tap do |decls|
        interface _Foo
          def self?.foo: () -> A
        end
            RBS
        decls[0].tap do |decl|
          pp decl
        end
      end
    end
  end

  def test_interface_mixin
    RBS::Parser.parse_signature(buffer(<<-RBS)).tap do |decls|
interface _Foo[unchecked in A]
  include Array[A]
  extend Object
  prepend _Foo[String]
end
    RBS
      decls[0].tap do |decl|
        pp decl.members
      end
    end
  end
end
