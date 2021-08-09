require "test_helper"

class RBS::ParserTest < Test::Unit::TestCase
  def buffer(source)
    RBS::Buffer.new(content: source, name: "test.rbs")
  end

  def test_interface
    RBS::Parser.parse_signature(buffer(<<-RBS)).tap do |decls|
interface _Foo[unchecked in A]
  def bar: [A] () -> A

  def foo: () -> A
         | { () -> void } -> void
end
    RBS
      decls[0].tap do |decl|
        decl.members[0].tap do |member|
          assert_equal :bar, member.name
          assert_instance_of RBS::Types::Variable, member.types[0].type.return_type
        end

        decl.members[1].tap do |member|
          assert_equal :foo, member.name
          assert_instance_of RBS::Types::Variable, member.types[0].type.return_type
        end
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

  def test_interface_alias
    RBS::Parser.parse_signature(buffer(<<-RBS)).tap do |decls|
interface _Foo[unchecked in A]
  alias hello world
end
    RBS
      decls[0].tap do |decl|
        decl.members[0].tap do |member|
          assert_instance_of RBS::AST::Members::Alias, member
          assert_equal :instance, member.kind
          assert_equal :hello, member.new_name
          assert_equal :world, member.old_name
          assert_equal "alias hello world", member.location.source
        end
      end
    end
  end

  def test_module_decl
    RBS::Parser.parse_signature(buffer(<<-RBS)).tap do |decls|
module Foo[X] : String, _Array[Symbol]
end
    RBS
      decls[0].tap do |decl|
        assert_instance_of RBS::AST::Declarations::Module, decl
        assert_equal TypeName("Foo"), decl.name

        assert_equal "module", decl.location[:keyword].source
        assert_equal "Foo", decl.location[:name].source
        assert_equal "[X]", decl.location[:type_params].source
        assert_equal ":", decl.location[:colon].source
        assert_equal "String, _Array[Symbol]", decl.location[:self_types].source
        assert_equal "end", decl.location[:end].source
      end
    end
  end

  def test_module_decl_def
    RBS::Parser.parse_signature(buffer(<<-RBS)).tap do |decls|
module Foo[X] : String, _Array[Symbol]
  def foo: () -> void

  def self.bar: () -> void

  def self?.baz: () -> void
end
    RBS
      decls[0].tap do |decl|
        assert_instance_of RBS::AST::Declarations::Module, decl
      end
    end
  end
end
