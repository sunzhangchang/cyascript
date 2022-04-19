use std::error::Error;
use std::fmt::{Debug, Display, Formatter};
use std::rc::Rc;

use crate::frontend::token::Token;
use crate::frontend::tokenizer::Tokenizer;

mod frontend;

pub struct ParseError {
    pub line: u16,
    pub reason: String,
}

impl ParseError {
    pub fn new(line: u16, reason: String) -> ParseError {
        ParseError { line, reason }
    }

    pub fn msg(mut self, msg: &'static str) -> ParseError {
        self.reason.push_str(msg);
        ParseError {
            line: self.line,
            reason: self.reason,
        }
    }
}

impl Debug for ParseError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "Parse error line {} : {}", self.line, self.reason)
    }
}

impl Display for ParseError {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "Parse error line {} : {}", self.line, self.reason)
    }
}

impl Error for ParseError {}

pub struct SingleType {
    pub name: Rc<String>,
    pub generic: Option<Vec<ParsedType>>,
}

impl Debug for SingleType {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}<{:?}>", self.name, self.generic)
    }
}

pub struct TypeTuple {
    pub vec: Vec<ParsedType>,
}

impl Debug for TypeTuple {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "{:?}", self.vec)
    }
}

pub enum ParsedType {
    Single(SingleType),
    Tuple(TypeTuple),
    MySelf,
}

impl Debug for ParsedType {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        match self {
            ParsedType::Single(tp) => write!(f, "{:?}", tp),
            ParsedType::Tuple(tp) => write!(f, "{:?}", tp),
            ParsedType::MySelf => {
                write!(f, "Self")
            }
        }
    }
}

#[derive(Debug, Clone)]
pub enum Var {
    Name(Rc<String>),

    LocalInt(u16, u8),
    LocalNum(u16, u8),
    LocalChar(u16, u8),
    LocalBool(u16, u8),
    LocalRef(u16),

    StaticInt(u16),
    StaticNum(u16),
    StaticChar(u16),
    StaticBool(u16),
    StaticRef(u16),

    Class(u16),
    Enum(u16),
    Interface(u16),
    BuiltinType(u16),

    DirectFn(u16),
}

#[derive(Copy, Clone)]
#[repr(u8)]
pub enum BasicType {
    Int,
    Num,
    Char,
    Bool,
    Ref,
}

impl Debug for BasicType {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(
            f,
            "{}",
            match self {
                BasicType::Int => "int",
                BasicType::Num => "num",
                BasicType::Char => "char",
                BasicType::Bool => "bool",
                BasicType::Ref => "Ref",
            }
        )
    }
}

#[derive(Debug)]
pub enum VarId {
    Index(u16, u8),
    Name(Rc<String>),
    DoubleIndex(u16, u16),
}

// 16bytes   rustc did optimization here
#[derive(Clone, PartialEq)]
pub enum DataType {
    Int,
    Num,
    Char,
    Bool,
    // Ref(RefType),
}

impl Display for DataType {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        let s: String;
        // 用于保证format!返回的String对象不会被立即释放
        // to sure that the returned String obj won't be drop immediately
        write!(
            f,
            "{}",
            match self {
                DataType::Int => "int",
                DataType::Num => "num",
                DataType::Char => "char",
                DataType::Bool => "bool",
                // DataType::Ref(ref_type) => {
                //     s = format!("{}", ref_type);
                //     s.as_str()
                // }
            }
        )
    }
}

impl Debug for DataType {
    fn fmt(&self, f: &mut Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self)
    }
}

#[derive(Debug)]
pub enum ExprType {
    Parsed(ParsedType),
    Analyzed(DataType),
}

#[derive(Debug)]
pub struct Construction {
    pub class_type: ExprType,
    pub fields: Vec<(VarId, BasicType, Expression)>,
}

// 16byte
#[derive(Debug)]
pub enum Expression {
    None,
    // 字面常量 literal constant value
    Int(i64),
    Num(f64),
    Char(char),
    Bool(bool),
    Str(Rc<String>),

    Var(Box<Var>),
    Tuple(Box<Vec<Expression>>),
    Array(Box<(Vec<Expression>, BasicType, bool)>),
    // is array to queue
    Construct(Box<Construction>),

    // 二元操作 binary operation
    // BinaryOp(Box<BinOpVec>),

    // 一元操作 unary operation
    Cast(Box<(Expression, ParsedType, DataType)>),
    NegOp(Box<Expression>),
    NotOp(Box<Expression>),
    // 条件控制 condition control
    // IfElse(Box<IfElse>),

    // 模式匹配 pattern match
    // Match(RefCount<(Expression, Vec<(Expression, Vec<Statement>)>)>),

    // 链式的成员变量访问和函数调用 field access and function call
    // Chain(Box<(Expression, Vec<Chain>)>),

    // 类函数定义 func-like define
    // Func(Box<FuncExpr>),
}

pub type Line = u16;

// 16byte
#[derive(Debug)]
pub enum Statement {
    Let(Box<(Var, Option<ParsedType>, Expression, Line)>),
    // Static(Box<(Var, Option<ParsedType>, Expression)>),
    // PubStatic(Box<(Var, Option<ParsedType>, Expression)>),
    // LeftValueOp(Box<(LeftValue, LeftValueOp)>),
    Expr(Expression, u16),
    Discard(Expression, u16),
    // 循环控制 loop control
    // While(Box<WhileLoop>),
    // For(Box<ForLoop>),

    // Continue(u16),
    // Break(u16),
    // Return(Expression, u16),
    // IfResult(Expression, u16),
}

pub struct ParsedFunc {
    pub params: Vec<(Rc<String>, ParsedType)>,
    pub body: Vec<Statement>,
    pub return_type: Option<ParsedType>,
}

// pub struct ParsedClass {
//     pub name: Rc<String>,
//     pub parent: Option<Rc<String>>,
//     pub impl_interfaces: Vec<Rc<String>>,
//     pub fields: Vec<(bool, ParsedType, Rc<String>)>,
//     pub funcs: Vec<(bool, Rc<String>, ParsedFunc)>,
// }

struct Parser {
    tokens: Vec<Token>,
    cur: usize,
    // classes: Vec<(ParsedClass, bool)>,
    // interfaces: Vec<(ParsedInterface, bool)>,
    // enums: Vec<(ParsedEnum, bool)>,
    funcs: Vec<(Rc<String>, ParsedFunc, bool)>,
    // imports: Vec<ParsedFile>,
    // importer: RefCount<Importer>,
    // path: String,
    pub lines: Vec<u16>,
}

pub struct ParsedFile {
    // pub imports: Vec<ParsedFile>,
    // pub classes: Vec<(ParsedClass, bool)>,
    // pub interfaces: Vec<(ParsedInterface, bool)>,
    // pub enums: Vec<(ParsedEnum, bool)>,
    pub funcs: Vec<(Rc<String>, ParsedFunc, bool)>,
    pub statements: Vec<Statement>,
    // pub path: String,
    pub index: u16,
}

impl Parser {
    pub fn parse(mut self) -> Result<ParsedFile, ParseError> {
        let vec = self.statements()?;
        Result::Ok(ParsedFile {
            // imports: self.imports,
            // classes: self.classes,
            // interfaces: self.interfaces,
            funcs: self.funcs,
            // enums: self.enums,
            statements: vec,
            // path: self.path,
            index: 0,
        })
    }

    fn statements(&mut self) -> Result<Vec<Statement>, ParseError> {
        let mut statements = vec![];
        while self.has_next() {
            let token = self.next();
            let statement = match token {
                Token::Let => {
                    let line = self.line();
                    let name = self.identifier()?;
                    let mut parsed_type: Option<ParsedType> = None;
                    if self.test(Token::Eq) {
                        self.cur += 1;
                    } else if self.test(Token::Colon) {
                        self.cur += 1;
                        parsed_type = Some(self.parse_type()?);
                        self.assert_next(Token::Eq)
                            .map_err(|e| e.msg("expect a '=' after 'let' and variable name"))?;
                    } else {
                        parsed_type = Some(self.parse_type()?);
                        self.assert_next(Token::Eq).map_err(|e| {
                            e.msg("expect a '=' after variable type in 'let' statement")
                        })?;
                    }
                    let expr = self.expr()?;
                    if self.has_next() && self.test(Token::Semi) {
                        self.cur += 1;
                    }
                    Statement::Let(Box::new((Var::Name(name), parsed_type, expr, line)))
                }
                Token::Semi => {
                    continue;
                }
                _ => {}
            };
        }
        Ok(statements)
    }

    fn expr(&mut self) -> Result<Expression, ParseError> {
        let expr = self.medium_expr()?;
        // let mut op_vec: Option<Vec<(BinOp, Expression)>> = Option::None;
        while self.has_next() {
            // let bin_op_expr: Option<(BinOp, Expression)>;
            match self.next() {
                // Token::Plus => bin_op_expr = Some((BinOp::Plus, self.medium_expr()?)),
                // Token::Sub => bin_op_expr = Some((BinOp::Sub, self.medium_expr()?)),
                // Token::And => bin_op_expr = Some((BinOp::And, self.medium_expr()?)),
                // Token::Or => bin_op_expr = Some((BinOp::Or, self.medium_expr()?)),
                _ => {
                    self.cur -= 1;
                    break;
                }
            }
            // match bin_op_expr {
            //     Some(op_tuple) => match &mut op_vec {
            //         None => {
            //             let mut vec = Vec::new();
            //             vec.push(op_tuple);
            //             op_vec = Some(vec);
            //         }
            //         Some(vec) => vec.push(op_tuple),
            //     },
            //     None => break,
            // }
        }
        match op_vec {
            Some(vec) => Result::Ok(Expression::BinaryOp(Box::new(BinOpVec { left: expr, vec }))),
            None => Result::Ok(expr),
        }
    }

    #[inline]
    fn has_next(&self) -> bool {
        self.cur + 1 < self.tokens.len()
    }

    #[inline]
    fn now(&self) -> &Token {
        self.tokens.get(self.cur).unwrap()
    }

    fn next(&mut self) -> &Token {
        self.cur += 1;
        self.tokens.get(self.cur - 1).unwrap()
    }

    fn test(&mut self, tk: Token) -> bool {
        tk.eq(self.now())
    }

    #[inline]
    fn line(&self) -> u16 {
        match self.lines.get(self.cur) {
            None => panic!("cur : {} lines len : {}", self.cur, self.lines.len()),
            Some(line) => *line,
        }
    }

    #[inline]
    fn identifier(&mut self) -> Result<Rc<String>, ParseError> {
        self.cur += 1;
        let token = self.tokens.get(self.cur - 1).unwrap();

        if let Token::Id(name) = token {
            Result::Ok(name.clone())
        } else {
            Result::Err(ParseError::new(
                self.line(),
                format!("expect identifier in fact found token {:?}", token),
            ))
        }
    }

    #[inline]
    fn assert_next(&mut self, token: Token) -> Result<(), ParseError> {
        let curr = self.tokens.get(self.cur).unwrap();
        if token.eq(curr) {
            self.cur += 1;
            Result::Ok(())
        } else {
            Result::Err(ParseError::new(
                self.line(),
                format!("[assert_next] expect token {:?} in fact {:?}", token, curr),
            ))
        }
    }
    fn parse_type(&mut self) -> Result<ParsedType, ParseError> {
        if self.test(Token::LParen) {
            self.cur += 1;
            let mut vec = Vec::new();
            while self.has_next() {
                match self.next() {
                    Token::Comma => continue,
                    Token::RParen => break,
                    _ => {
                        self.cur -= 1;
                        vec.push(self.parse_type()?)
                    }
                }
            }
            return Result::Ok(ParsedType::Tuple(TypeTuple { vec }));
        }
        let type_name = self
            .identifier()
            .map_err(|err| err.msg("expect a type identifier"))?;
        let mut generic: Option<Vec<ParsedType>> = Option::None;
        if self.has_next() && self.test(Token::Lt) {
            let mut vec: Vec<ParsedType> = Vec::new();
            self.cur += 1;
            while self.has_next() {
                vec.push(self.parse_type()?);
                if self.test(Token::Gt) {
                    self.cur += 1;
                    break;
                }
                if self.test(Token::Comma) {
                    self.cur += 1;
                }
            }
            generic = Some(vec)
        }
        Result::Ok(ParsedType::Single(SingleType {
            name: type_name,
            generic,
        }))
    }

    pub fn new(
        tokens: Vec<Token>,
        lines: Vec<u16>,
        // importer: RefCount<Importer>,
        // path: String,
    ) -> Parser {
        return Parser {
            tokens,
            lines,
            cur: 0,
            // classes: Vec::with_capacity(0),
            // interfaces: Vec::with_capacity(0),
            // enums: Vec::with_capacity(0),
            funcs: Vec::new(),
            // imports: Vec::new(),
            // importer,
            // path,
        };
    }
}

fn main() {
    // println!("Hello, world!");
    let code = Vec::from(
        r#"
    println("hello world!")
    "#
        .as_bytes(),
    );

    let (tokens, lines) = Tokenizer::new(code).tokenize();
    // println!("{:?}", &lines);
    let parser = Parser::new(tokens, lines);
    let a = parser.parse().unwrap();
    println!("{:?}", a.statements);
}
