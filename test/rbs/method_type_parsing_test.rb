require "test_helper"

class RBS::MethodTypeParsingTest < Test::Unit::TestCase
  Parser = RBS::Parser
  Buffer = RBS::Buffer
  Types = RBS::Types
  TypeName = RBS::TypeName
  Namespace = RBS::Namespace
  Location = RBS::Location

  def parse_type(string)
    buffer = Buffer.new(content: string.encode(Encoding::UTF_8), name: "sample.rbs")
    RBS::Parser._parse_type(buffer, 1, 0)
  end

  def test_tokenizer
#     puts parse_type(<<EOF)
# Array[
#   # LINE Comment here!
#   String # comment here.
#   ,
#   :foo,
#   :"日本語"
# ]
# EOF
    parse_type("self").tap do |type|
      assert_instance_of Types::Bases::Self, type
      assert_equal "self", type.location.source
    end
    puts parse_type("::Object::Array")
    puts parse_type("::Object::_Each")
    puts parse_type("::Object::Types::t")
    puts parse_type("singleton(::Object)")
    puts parse_type("t & s | u & a")
    puts parse_type("t | s | u & a")
    puts parse_type("t & s | (u | a)")
    puts parse_type("[a, B]")
    puts parse_type("[a,]")
    puts parse_type("[]")
    puts parse_type("Array[String]")
    puts parse_type("Array[String?]?")
    puts parse_type("^() -> void")
    puts parse_type("^(String) -> void")
    puts parse_type("^(String s) -> void")
    puts parse_type("[123, +12_23, -1234_]")
    puts parse_type("true | false")
    puts parse_type("['foo', 'foo\"bar', 'ba\\'z']")
    puts parse_type('["foo", "foo\"bar", "ba\\tz"]')
    puts parse_type('"ba\\tz"').literal
    puts parse_type(<<'TYPE')
[
  :foo,
  :$bar,
  :@bar,
  :@barbaz
  :__bar,
  :"日本語",
  :|,
  :&,
  :/,
  :%,
  :~,
  :`,
  :^,
  :==,
  :=~,
  :===,
  :<=>,
  :<<,
  :<=,
  :<,
  :>>,
  :>=,
  :>,
  :+@,
  :+,
  :-@,
  :-,
  :*,
  :**,
  :[]=,
  :[],
  :!=,
  :!~,
]
TYPE
  end

  def test_method_type
    Parser.parse_method_type("()->void").yield_self do |type|
      assert_equal "() -> void", type.to_s
    end

    Parser.parse_method_type("(foo?: Integer, bar!: String)->void")
    Parser.parse_method_type("(?foo?: Integer, ?bar!: String)->void")
  end

  def test_method_param
    Parser.parse_method_type("(untyped _, top __, Object _2, String _abc_123)->void").yield_self do |type|
      assert_equal "(untyped _, top __, Object _2, String _abc_123) -> void", type.to_s
    end

    Parser.parse_method_type("(untyped _)->void").yield_self do |type|
      assert_equal "(untyped _) -> void", type.to_s
    end
end

  def test_method_type_eof_re
    Parser.parse_method_type("()->void~ Integer", eof_re: /~/).yield_self do |type|
      assert_equal "() -> void", type.to_s
    end
  end

  def test_method_type_eof_re_error
    # `eof_re` has higher priority than other token.
    # Specifying type token may result in a SyntaxError
    error = assert_raises Parser::SyntaxError do
      Parser.parse_method_type("()-> { foo: bar } }", eof_re: /}/).yield_self do |type|
        assert_equal "() -> void", type.to_s
      end
    end

    assert_equal "}", error.error_value
  end

  def test_method_parameter_location
    Parser.parse_method_type("(untyped a, ?Integer b, *String c, Symbol d) -> void").tap do |type|
      type.type.required_positionals[0].tap do |param|
        assert_instance_of Location::WithChildren, param.location
        assert_equal "a", param.location[:name].source
      end

      type.type.optional_positionals[0].tap do |param|
        assert_instance_of Location::WithChildren, param.location
        assert_equal "b", param.location[:name].source
      end

      type.type.rest_positionals.tap do |param|
        assert_instance_of Location::WithChildren, param.location
        assert_equal "c", param.location[:name].source
      end

      type.type.trailing_positionals[0].tap do |param|
        assert_instance_of Location::WithChildren, param.location
        assert_equal "d", param.location[:name].source
      end
    end

    Parser.parse_method_type("(a: untyped a, ?b: Integer b, **String c) -> void").tap do |type|
      type.type.required_keywords[:a].tap do |param|
        assert_instance_of Location::WithChildren, param.location
        assert_equal "a", param.location[:name].source
      end

      type.type.optional_keywords[:b].tap do |param|
        assert_instance_of Location::WithChildren, param.location
        assert_equal "b", param.location[:name].source
      end

      type.type.rest_keywords.tap do |param|
        assert_instance_of Location::WithChildren, param.location
        assert_equal "c", param.location[:name].source
      end
    end
  end
end
