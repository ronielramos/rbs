#ifndef RBS__RUBY_OBJS_H
#define RBS__RUBY_OBJS_H

#include "ruby.h"
#include "lexer.h"
#include "parser.h"

extern VALUE rbsparser_Keywords;

extern VALUE RBSParser;
extern VALUE rbsparser_Keywords;

extern ID id_RBS;
extern VALUE sym_class;
extern VALUE sym_alias;
extern VALUE sym_interface;

extern VALUE RBS;

extern VALUE RBS_AST;
extern VALUE RBS_AST_Comment;
extern VALUE RBS_AST_Annotation;

extern VALUE RBS_AST_Declarations;

/**
 * RBS::AST::Declarations::ModuleTypeParams
 * */
extern VALUE RBS_AST_Declarations_ModuleTypeParams;
/**
 * RBS::AST::Declarations::ModuleTypeParams::TypeParam
 * */
extern VALUE RBS_AST_Declarations_ModuleTypeParams_TypeParam;

extern VALUE RBS_AST_Declarations_Alias;
extern VALUE RBS_AST_Declarations_Constant;
extern VALUE RBS_AST_Declarations_Global;
extern VALUE RBS_AST_Declarations_Interface;
extern VALUE RBS_AST_Declarations_Module;
extern VALUE RBS_AST_Declarations_Module_Self;
extern VALUE RBS_AST_Declarations_Class;
extern VALUE RBS_AST_Declarations_Class_Super;

extern VALUE RBS_AST_Members;
extern VALUE RBS_AST_Members_Alias;
extern VALUE RBS_AST_Members_AttrAccessor;
extern VALUE RBS_AST_Members_AttrReader;
extern VALUE RBS_AST_Members_AttrWriter;
extern VALUE RBS_AST_Members_ClassInstanceVariable;
extern VALUE RBS_AST_Members_ClassVariable;
extern VALUE RBS_AST_Members_Extend;
extern VALUE RBS_AST_Members_Include;
extern VALUE RBS_AST_Members_InstanceVariable;
extern VALUE RBS_AST_Members_MethodDefinition;
extern VALUE RBS_AST_Members_Prepend;
extern VALUE RBS_AST_Members_Private;
extern VALUE RBS_AST_Members_Public;

extern VALUE RBS_Location;
extern VALUE RBS_Namespace;
extern VALUE RBS_TypeName;

extern VALUE RBS_Types_Alias;
extern VALUE RBS_Types_Bases_Any;
extern VALUE RBS_Types_Bases_Bool;
extern VALUE RBS_Types_Bases_Bottom;
extern VALUE RBS_Types_Bases_Class;
extern VALUE RBS_Types_Bases_Instance;
extern VALUE RBS_Types_Bases_Nil;
extern VALUE RBS_Types_Bases_Self;
extern VALUE RBS_Types_Bases_Top;
extern VALUE RBS_Types_Bases_Void;
extern VALUE RBS_Types_Bases;
extern VALUE RBS_Types_Block;
extern VALUE RBS_Types_ClassInstance;
extern VALUE RBS_Types_ClassSingleton;
extern VALUE RBS_Types_Function_Param;
extern VALUE RBS_Types_Function;
extern VALUE RBS_Types_Interface;
extern VALUE RBS_Types_Intersection;
extern VALUE RBS_Types_Literal;
extern VALUE RBS_Types_Optional;
extern VALUE RBS_Types_Proc;
extern VALUE RBS_Types_Record;
extern VALUE RBS_Types_Tuple;
extern VALUE RBS_Types_Union;
extern VALUE RBS_Types_Variable;
extern VALUE RBS_Types;
extern VALUE RBS_MethodType;

VALUE rbs_base_type(VALUE klass, VALUE location);
VALUE rbs_namespace(VALUE path, VALUE absolute);
VALUE rbs_type_name(VALUE namespace, VALUE name);
VALUE rbs_class_instance(VALUE typename, VALUE type_args, VALUE location);
VALUE rbs_class_singleton(VALUE typename, VALUE location);
VALUE rbs_alias(VALUE typename, VALUE location);
VALUE rbs_interface(VALUE typename, VALUE type_args, VALUE location);
VALUE rbs_union(VALUE types, VALUE location);
VALUE rbs_intersection(VALUE types, VALUE location);
VALUE rbs_tuple(VALUE types, VALUE location);
VALUE rbs_optional(VALUE type, VALUE location);
VALUE rbs_function(VALUE required_positional_params, VALUE optional_positional_params, VALUE rest_positional_params, VALUE trailing_positional_params, VALUE required_keywords, VALUE optional_keywords, VALUE rest_keywords, VALUE return_type);
VALUE rbs_function_param(VALUE type, VALUE name, VALUE location);
VALUE rbs_block(VALUE type, VALUE required);
VALUE rbs_proc(VALUE function, VALUE block, VALUE location);
VALUE rbs_literal(VALUE literal, VALUE location);
VALUE rbs_record(VALUE fields, VALUE location);
VALUE rbs_variable(VALUE name, VALUE location);

/**
 * Returns RBS::Location object with start/end positions.
 *
 * @param start_pos
 * @param end_pos
 * @return New RSS::Location object.
 * */
VALUE rbs_location_pp(VALUE buffer, const position *start_pos, const position *end_pos);

/**
 * Returns RBS::Location object with position range.
 *
 * @param buffer
 * @param range
 * @return New RBS::Location object.
 * */
VALUE rbs_location_rg(VALUE buffer, const range *range);

/**
 * Returns RBS::Location object from a token.
 *
 * @param buffer
 * @param tok
 * @return New RBS::Location object.
 * */
VALUE rbs_location_tok(VALUE buffer, const token *tok);

/**
 * Returns RBS::Location object of `current_token` of a parser state.
 *
 * @param state
 * @return New RBS::Location object.
 * */
VALUE rbs_location_current_token(parserstate *state);

VALUE rbs_method_type(VALUE type_params, VALUE type, VALUE block, VALUE location);

VALUE rbs_ast_comment(VALUE string, VALUE location);
VALUE rbs_ast_annotation(VALUE string, VALUE location);

/**
 * RBS::AST::Declarations::ModuleTypeParams.new()
 * */
VALUE rbs_ast_decl_module_type_params();

/**
 * RBS::AST::Declarations::ModuleTypeParams::TypeParam.new(name: name, variance: variance, skip_validation: skip_validation);
 * */
VALUE rbs_ast_decl_module_type_params_param(VALUE name, VALUE variance, VALUE skip_validation, VALUE location);

VALUE rbs_ast_decl_constant(VALUE name, VALUE type, VALUE location, VALUE comment);
VALUE rbs_ast_decl_global(VALUE name, VALUE type, VALUE location, VALUE comment);
VALUE rbs_ast_decl_alias(VALUE name, VALUE type, VALUE annotations, VALUE location, VALUE comment);
VALUE rbs_ast_decl_interface(VALUE name, VALUE type_params, VALUE members, VALUE annotations, VALUE location, VALUE comment);
VALUE rbs_ast_decl_module(VALUE name, VALUE type_params, VALUE self_types, VALUE members, VALUE annotations, VALUE location, VALUE comment);
VALUE rbs_ast_decl_module_self(VALUE name, VALUE args, VALUE location);
VALUE rbs_ast_decl_class_super(VALUE name, VALUE args, VALUE location);
VALUE rbs_ast_decl_class(VALUE name, VALUE type_params, VALUE super_class, VALUE members, VALUE annotations, VALUE location, VALUE comment);

VALUE rbs_ast_members_method_definition(VALUE name, VALUE kind, VALUE types, VALUE annotations, VALUE location, VALUE comment, VALUE overload);
VALUE rbs_ast_members_variable(VALUE klass, VALUE name, VALUE type, VALUE location, VALUE comment);
VALUE rbs_ast_members_mixin(VALUE klass, VALUE name, VALUE args, VALUE annotations, VALUE location, VALUE comment);
VALUE rbs_ast_members_attribute(VALUE klass, VALUE name, VALUE type, VALUE ivar_name, VALUE kind, VALUE annotations, VALUE location, VALUE comment);
VALUE rbs_ast_members_visibility(VALUE klass, VALUE location);
VALUE rbs_ast_members_alias(VALUE new_name, VALUE old_name, VALUE kind, VALUE annotations, VALUE location, VALUE comment);

void pp(VALUE object);

#endif
