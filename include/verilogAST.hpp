#include <map>
#include <string>
#include <utility>  // std::pair
#include <variant>
#include <vector>

namespace verilogAST {

class Node {
 public:
  virtual std::string toString() = 0;
};

class Expression : public Node {
 public:
  virtual std::string toString() = 0;
};

enum Radix { BINARY, OCTAL, HEX, DECIMAL };

class NumericLiteral : public Expression {
  /// For now, we model values as strings because it depends on their radix
  // (alternatively, we could store an unsigned integer representation and
  //  convert it during code generation)
  //
  // TODO Maybe add special toString logic for the default case? E.g. if we're
  // generating a 32 bit unsigned decimal literal (commonly used for indexing
  // into ports) then we don't need to generate the "32'd" prefix
  std::string value;
  unsigned int size;  // default 32
  bool _signed;       // default false
  Radix radix;        // default decimal

 public:
  NumericLiteral(std::string value, unsigned int size, bool _signed,
                 Radix radix)
      : value(value), size(size), _signed(_signed), radix(radix){};

  NumericLiteral(std::string value, unsigned int size, bool _signed)
      : value(value), size(size), _signed(_signed), radix(Radix::DECIMAL){};

  NumericLiteral(std::string value, unsigned int size)
      : value(value), size(size), _signed(false), radix(Radix::DECIMAL){};

  NumericLiteral(std::string value)
      : value(value), size(32), _signed(false), radix(Radix::DECIMAL){};
  std::string toString() override;
};

// TODO also need a string literal, as strings can be used as parameter values

class Identifier : public Expression {
  std::string value;

 public:
  Identifier(std::string value) : value(value){};

  std::string toString() override;
};

class String : public Expression {
  std::string value;

 public:
  String(std::string value) : value(value){};

  std::string toString() override;
};

class Index : public Expression {
  Identifier *id;
  Expression *index;

 public:
  Index(Identifier *id, Expression *index) : id(id), index(index){};
  std::string toString() override;
};

class Slice : public Expression {
  Identifier *id;
  Expression *high_index;
  Expression *low_index;

 public:
  Slice(Identifier *id, Expression *high_index, Expression *low_index)
      : id(id), high_index(high_index), low_index(low_index){};
  std::string toString() override;
};

namespace BinOp {
enum BinOp {
  // TODO Bitwise Logical ops like &, |,
  LSHIFT,
  RSHIFT,
  AND,
  OR,
  EQ,
  NEQ,
  ADD,
  SUB,
  MUL,
  DIV,
  POW,
  MOD,
  ALSHIFT,
  ARSHIFT
};
}

class BinaryOp : public Expression {
  Expression *left;
  BinOp::BinOp op;
  Expression *right;

 public:
  BinaryOp(Expression *left, BinOp::BinOp op, Expression *right)
      : left(left), op(op), right(right){};
  std::string toString() override;
};

namespace UnOp {
enum UnOp {
  NOT,
  INVERT,
  AND,
  NAND,
  OR,
  NOR,
  XOR,
  NXOR,
  XNOR,  // TODO ~^ vs ^~?
  PLUS,
  MINUS
};
}

class UnaryOp : public Expression {
  Expression *operand;

  UnOp::UnOp op;

 public:
  UnaryOp(Expression *operand, UnOp::UnOp op) : operand(operand), op(op){};
  std::string toString();
};

class TernaryOp : public Expression {
  Expression *cond;
  Expression *true_value;
  Expression *false_value;

 public:
  TernaryOp(Expression *cond, Expression *true_value, Expression *false_value)
      : cond(cond), true_value(true_value), false_value(false_value){};
  std::string toString();
};

class NegEdge : public Expression {
  Expression *value;

 public:
  NegEdge(Expression *value) : value(value){};
  std::string toString();
};

class PosEdge : public Expression {
  Expression *value;

 public:
  PosEdge(Expression *value) : value(value){};
  std::string toString();
};

enum Direction { INPUT, OUTPUT, INOUT };

// TODO: Unify with declarations?
enum PortType { WIRE, REG };

class AbstractPort : public Node {};

class Vector : public Node {
  Identifier *id;
  Expression *msb;
  Expression *lsb;

 public:
  Vector(Identifier *id, Expression *msb, Expression *lsb)
      : id(id), msb(msb), lsb(lsb){};
  std::string toString() override;
};

class Port : public AbstractPort {
  // Required
  // `<name>` or `<name>[n]` or `name[n:m]`
  std::variant<Identifier *, Vector *> value;

  // technically the following are optional (e.g. port direction/data type
  // can be declared in the body of the definition), but for now let's force
  // users to declare ports in a single, unified way for
  // simplicity/maintenance
  Direction direction;
  PortType data_type;

 public:
  Port(std::variant<Identifier *, Vector *> value, Direction direction,
       PortType data_type)
      : value(value), direction(direction), data_type(data_type){};
  std::string toString();
};

class StringPort : public AbstractPort {
  std::string value;

 public:
  StringPort(std::string value) : value(value){};
  std::string toString() { return value; };
};

class Statement : public Node {};

class SingleLineComment : public Statement {
  std::string value;

 public:
  SingleLineComment(std::string value) : value(value){};
  std::string toString() { return "// " + value; };
};

class BlockComment : public Statement {
  std::string value;

 public:
  BlockComment(std::string value) : value(value){};
  std::string toString() { return "/*\n" + value + "\n*/"; };
};

class BehavioralStatement : public Statement {};
class StructuralStatement : public Statement {};

typedef std::vector<std::pair<Identifier *, Expression *>> Parameters;

class ModuleInstantiation : public StructuralStatement {
  std::string module_name;

  // parameter,value
  Parameters parameters;

  std::string instance_name;

  // map from instance port names to connection expression
  // NOTE: anonymous style of module connections is not supported
  std::map<std::string, std::variant<Identifier *, Index *, Slice *>>
      connections;

 public:
  // TODO Need to make sure that the instance parameters are a subset of the
  // module parameters
  ModuleInstantiation(
      std::string module_name, Parameters parameters, std::string instance_name,
      std::map<std::string, std::variant<Identifier *, Index *, Slice *>>
          connections)
      : module_name(module_name),
        parameters(parameters),
        instance_name(instance_name),
        connections(connections){};
  std::string toString();
};

class Declaration : public Node {
 protected:
  std::string decl;
  std::variant<Identifier *, Vector *> value;

  Declaration(std::variant<Identifier *, Vector *> value,
              std::string decl)
      : decl(decl), value(value){};

 public:
  std::string toString();
};

class Wire : public Declaration {
 public:
  Wire(std::variant<Identifier *, Vector *> value)
      : Declaration(value, "wire"){};
};

class Reg : public Declaration {
 public:
  Reg(std::variant<Identifier *, Vector *> value)
      : Declaration(value, "reg"){};
};

class Assign : public Node {
 protected:
  std::variant<Identifier *, Index *, Slice *> target;
  Expression *value;
  std::string prefix;
  std::string symbol;

  Assign(std::variant<Identifier *, Index *, Slice *> target, Expression *value,
         std::string prefix)
      : target(target), value(value), prefix(prefix), symbol("="){};
  Assign(std::variant<Identifier *, Index *, Slice *> target, Expression *value,
         std::string prefix, std::string symbol)
      : target(target), value(value), prefix(prefix), symbol(symbol){};

 public:
  std::string toString();
};

class ContinuousAssign : public StructuralStatement, public Assign {
 public:
  ContinuousAssign(std::variant<Identifier *, Index *, Slice *> target,
                   Expression *value)
      : Assign(target, value, "assign "){};
  // Multiple inheritance forces us to have to explicitly state this?
  std::string toString() { return Assign::toString(); };
};

class BehavioralAssign : public BehavioralStatement {};

class BlockingAssign : public BehavioralAssign, public Assign {
 public:
  BlockingAssign(std::variant<Identifier *, Index *, Slice *> target,
                 Expression *value)
      : Assign(target, value, ""){};
  // Multiple inheritance forces us to have to explicitly state this?
  std::string toString() { return Assign::toString(); };
};

class NonBlockingAssign : public BehavioralAssign, public Assign {
 public:
  NonBlockingAssign(std::variant<Identifier *, Index *, Slice *> target,
                    Expression *value)
      : Assign(target, value, "", "<="){};
  // Multiple inheritance forces us to have to explicitly state this?
  std::string toString() { return Assign::toString(); };
};

class Star : Node {
 public:
  std::string toString() { return "*"; };
};

class Always : public StructuralStatement {
  std::vector<std::variant<Identifier *, PosEdge *, NegEdge *, Star *>>
      sensitivity_list;
  std::vector<std::variant<BehavioralStatement *, Declaration *>> body;

 public:
  Always(std::vector<std::variant<Identifier *, PosEdge *, NegEdge *, Star *>>
             sensitivity_list,
         std::vector<std::variant<BehavioralStatement *, Declaration *>> body)
      : sensitivity_list(sensitivity_list), body(body) {
    if (sensitivity_list.empty()) {
      throw std::runtime_error(
          "vAST::Always expects non-empty sensitivity list");
    }
  };
  std::string toString();
};

class AbstractModule : public Node {};

class Module : public AbstractModule {
 protected:
  std::string name;
  std::vector<AbstractPort *> ports;
  std::vector<std::variant<StructuralStatement *, Declaration *>> body;
  Parameters parameters;
  std::string emitModuleHeader();
  // Protected initializer that is used by the StringBodyModule subclass which
  // overrides the `body` field (but reuses the other fields)
  Module(std::string name, std::vector<AbstractPort *> ports,
         Parameters parameters)
      : name(name), ports(ports), parameters(parameters){};

 public:
  Module(std::string name, std::vector<AbstractPort *> ports,
         std::vector<std::variant<StructuralStatement *, Declaration *>> body,
         Parameters parameters)
      : name(name), ports(ports), body(body), parameters(parameters){};

  std::string toString();
};

class StringBodyModule : public Module {
  std::string body;

 public:
  StringBodyModule(std::string name, std::vector<AbstractPort *> ports,
                   std::string body, Parameters parameters)
      : Module(name, ports, parameters), body(body){};
  std::string toString();
};

class StringModule : public AbstractModule {
  std::string definition;

 public:
  StringModule(std::string definition) : definition(definition){};
  std::string toString() { return definition; };
};

class File : public Node {
  std::vector<AbstractModule *> modules;

 public:
  File(std::vector<AbstractModule *> modules) : modules(modules){};
  std::string toString();
};

}  // namespace verilogAST
