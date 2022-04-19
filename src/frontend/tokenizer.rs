use std::rc::Rc;

use crate::frontend::token::Token;

pub struct Tokenizer {
    src: Vec<u8>,
    cur: usize,
    line: u16,
}

impl Tokenizer {
    pub(crate) fn new(src: Vec<u8>) -> Tokenizer {
        Tokenizer {
            src,
            cur: 0,
            line: 1,
        }
    }

    pub fn tokenize(&mut self) -> (Vec<Token>, Vec<u16>) {
        let mut tokens = Vec::with_capacity(self.src.len() / 2);
        let mut lines = Vec::with_capacity(tokens.len());
        while self.cur < self.src.len() {
            let byte = self.now();
            match byte {
                b'\n' => {
                    self.line += 1;
                }
                b'(' => {
                    tokens.push(Token::LParen);
                    lines.push(self.line);
                }
                b')' => {
                    tokens.push(Token::RParen);
                    lines.push(self.line);
                }
                b'+' => {
                    tokens.push(Token::And);
                    lines.push(self.line);
                }
                b'-' => {
                    tokens.push(Token::Sub);
                    lines.push(self.line);
                }
                b'*' => {
                    tokens.push(Token::Mul);
                    lines.push(self.line);
                }
                b';' => {
                    tokens.push(Token::Semi);
                    lines.push(self.line);
                }
                b'=' => {
                    tokens.push(Token::Eq);
                    lines.push(self.line);
                }
                b'#' => {
                    self.skip_comment_line();
                }
                b'"' => {
                    tokens.push(self.parse_str());
                    lines.push(self.line)
                }
                byte if Self::is_valid_header(byte) => {
                    tokens.push(self.parse_identifier());
                    lines.push(self.line);
                    continue;
                }
                byte if byte <= b' ' => {}
                _ => println!("{}", byte as char),
            }
            self.cur += 1
        }
        lines.push(self.line + 1);
        (tokens, lines)
    }

    fn skip_comment_line(&mut self) {
        while self.cur < self.src.len() {
            let byte = self.now();
            if byte == b'\n' {
                break;
            } else {
                self.cur += 1;
            }
        }
        self.line += 1;
    }

    fn parse_str(&mut self) -> Token {
        self.cur += 1;
        let mut str = Vec::new();
        while self.cur < self.src.len() {
            let byte = self.now();
            if byte == b'"' {
                break;
            } else {
                str.push(byte);
                self.cur += 1;
            }
        }
        Token::Str(Rc::new(String::from_utf8(str).unwrap()))
    }

    fn is_valid_letter(byte: u8) -> bool {
        (byte >= b'a' && byte <= b'z')
            || (byte >= b'A' && byte <= b'Z')
            || (byte >= b'0' && byte <= b'9')
            || (byte == b'_')
    }

    fn is_valid_header(byte: u8) -> bool {
        (byte >= b'a' && byte <= b'z') || (byte >= b'A' && byte <= b'Z') || (byte == b'_')
    }

    fn parse_identifier(&mut self) -> Token {
        let mut str = vec![self.now()];
        self.cur += 1;
        while self.cur < self.src.len() {
            let byte = self.now();
            if Self::is_valid_letter(byte) {
                str.push(byte);
            } else {
                break;
            }
            self.cur += 1;
        }
        let id = String::from_utf8(str).unwrap();
        match id.as_str() {
            "let" => Token::Let,
            "fun" => Token::Func,
            _ => Token::Id(Rc::new(id)),
        }
    }

    fn now(&mut self) -> u8 {
        return *self.src.get(self.cur).unwrap();
    }
}
