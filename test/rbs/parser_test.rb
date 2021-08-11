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
    assert_raises do
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

  def test_module_decl_vars
    RBS::Parser.parse_signature(buffer(<<-RBS)).tap do |decls|
module Foo[X] : String, _Array[Symbol]
  @foo: Integer

  self.@bar: String

  @@baz: X
end
    RBS
      decls[0].tap do |decl|
        assert_instance_of RBS::AST::Declarations::Module, decl
      end
    end
  end

  def test_module_decl_attributes
    RBS::Parser.parse_signature(buffer(<<-RBS)).tap do |decls|
module Foo
  attr_reader string: String
  attr_writer self.name (): Integer
  attr_accessor writer (@Writer): Symbol
end
    RBS
      decls[0].tap do |decl|
        assert_instance_of RBS::AST::Declarations::Module, decl

        decl.members[0].tap do |member|
          assert_instance_of RBS::AST::Members::AttrReader, member
          assert_equal :instance, member.kind
          assert_equal :string, member.name
          assert_equal "String", member.type.to_s

          assert_equal "attr_reader", member.location[:keyword].source
          assert_equal "string", member.location[:name].source
          assert_nil member.location[:kind]
          assert_nil member.location[:ivar]
          assert_nil member.location[:ivar_name]
        end

        decl.members[1].tap do |member|
          assert_instance_of RBS::AST::Members::AttrWriter, member
          assert_equal :singleton, member.kind
          assert_equal :name, member.name
          assert_equal "Integer", member.type.to_s
          assert_equal false, member.ivar_name

          assert_equal "attr_writer", member.location[:keyword].source
          assert_equal "name", member.location[:name].source
          assert_equal "self.", member.location[:kind].source
          assert_equal "()", member.location[:ivar].source
          assert_nil member.location[:ivar_name]
        end

        decl.members[2].tap do |member|
          assert_instance_of RBS::AST::Members::AttrAccessor, member
          assert_equal :instance, member.kind
          assert_equal :writer, member.name
          assert_equal "Symbol", member.type.to_s
          assert_equal :"@Writer", member.ivar_name

          assert_equal "attr_accessor", member.location[:keyword].source
          assert_equal "writer", member.location[:name].source
          assert_nil member.location[:kind]
          assert_equal "(@Writer)", member.location[:ivar].source
          assert_equal "@Writer", member.location[:ivar_name].source
        end
      end
    end
  end

  def test_module_decl_public_private
    RBS::Parser.parse_signature(buffer(<<-RBS)).tap do |decls|
module Foo
  public
  private
end
    RBS
      decls[0].tap do |decl|
        assert_instance_of RBS::AST::Declarations::Module, decl

        assert_instance_of RBS::AST::Members::Public, decl.members[0]
        assert_instance_of RBS::AST::Members::Private, decl.members[1]
      end
    end
  end

  def test_module_decl_nested
    RBS::Parser.parse_signature(buffer(<<-RBS)).tap do |decls|
module Foo
  type foo = bar

  BAZ: Intger
end
    RBS
      decls[0].tap do |decl|
        assert_instance_of RBS::AST::Declarations::Module, decl
      end
    end
  end

  def test_module_type_var_decl
    RBS::Parser.parse_signature(buffer(<<-RBS)).tap do |decls|
module Foo[A]
  type t = A

  FOO: A
end
    RBS
      decls[0].tap do |decl|
        assert_instance_of RBS::AST::Declarations::Module, decl

        decl.members[0].tap do |member|
          assert_instance_of RBS::AST::Declarations::Alias, member
          assert_instance_of RBS::Types::ClassInstance, member.type
        end

        decl.members[1].tap do |member|
          assert_instance_of RBS::AST::Declarations::Constant, member
          assert_instance_of RBS::Types::ClassInstance, member.type
        end
      end
    end
  end

  def test_module_type_var_ivar
    RBS::Parser.parse_signature(buffer(<<-RBS)).tap do |decls|
module Foo[A]
  @x: A
  @@x: A
  self.@x: A
end
    RBS
      decls[0].tap do |decl|
        assert_instance_of RBS::AST::Declarations::Module, decl

        decl.members[0].tap do |member|
          assert_instance_of RBS::AST::Members::InstanceVariable, member
          assert_instance_of RBS::Types::Variable, member.type
        end

        decl.members[1].tap do |member|
          assert_instance_of RBS::AST::Members::ClassVariable, member
          assert_instance_of RBS::Types::ClassInstance, member.type
        end

        decl.members[2].tap do |member|
          assert_instance_of RBS::AST::Members::ClassInstanceVariable, member
          assert_instance_of RBS::Types::ClassInstance, member.type
        end
      end
    end
  end

  def test_module_type_var_attr
    RBS::Parser.parse_signature(buffer(<<-RBS)).tap do |decls|
module Foo[A]
  attr_reader foo: A
  attr_writer self.bar: A
end
    RBS
      decls[0].tap do |decl|
        assert_instance_of RBS::AST::Declarations::Module, decl

        decl.members[0].tap do |member|
          assert_instance_of RBS::AST::Members::AttrReader, member
          assert_instance_of RBS::Types::Variable, member.type
        end

        decl.members[1].tap do |member|
          assert_instance_of RBS::AST::Members::AttrWriter, member
          assert_instance_of RBS::Types::ClassInstance, member.type
        end
      end
    end
  end

  def test_module_type_var_method
    RBS::Parser.parse_signature(buffer(<<-RBS)).tap do |decls|
module Foo[A]
  def foo: () -> A

  def self.bar: () -> A

  def self?.baz: () -> A
end
    RBS
      decls[0].tap do |decl|
        assert_instance_of RBS::AST::Declarations::Module, decl

        decl.members[0].tap do |member|
          assert_instance_of RBS::AST::Members::MethodDefinition, member
          assert_instance_of RBS::Types::Variable, member.types[0].type.return_type
        end

        decl.members[1].tap do |member|
          assert_instance_of RBS::AST::Members::MethodDefinition, member
          assert_instance_of RBS::Types::ClassInstance, member.types[0].type.return_type
        end

        decl.members[2].tap do |member|
          assert_instance_of RBS::AST::Members::MethodDefinition, member
          assert_instance_of RBS::Types::ClassInstance, member.types[0].type.return_type
        end
      end
    end
  end

  def test_module_type_var_mixin
    RBS::Parser.parse_signature(buffer(<<-RBS)).tap do |decls|
module Foo[A]
  include X[A]

  extend X[A]

  prepend X[A]
end
    RBS
      decls[0].tap do |decl|
        assert_instance_of RBS::AST::Declarations::Module, decl

        decl.members[0].tap do |member|
          assert_instance_of RBS::AST::Members::Include, member
          assert_instance_of RBS::Types::Variable, member.args[0]
        end

        decl.members[1].tap do |member|
          assert_instance_of RBS::AST::Members::Extend, member
          assert_instance_of RBS::Types::ClassInstance, member.args[0]
        end

        decl.members[2].tap do |member|
          assert_instance_of RBS::AST::Members::Prepend, member
          assert_instance_of RBS::Types::Variable, member.args[0]
        end
      end
    end
  end
end
