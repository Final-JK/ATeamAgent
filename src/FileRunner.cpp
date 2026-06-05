#include "FileRunner.h"
#include "Lexer.h"
#include "Parser.h"
#include "Resolver.h"
#include "Interpreter.h"
#include <fstream>
#include <sstream>
#include <iostream>

FileRunner::FileRunner(std::string path)
    : filePath(std::move(path)) {}

int FileRunner::run() {
    // 파일 열기 — 존재하지 않으면 명확한 오류 메시지 출력
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << filePath << "\n";
        return 66;
    }

    std::ostringstream ss;
    ss << file.rdbuf();
    std::string source = ss.str();

    try {
        Lexer lexer(source);
        auto  tokens = lexer.scanTokens();

        Parser parser(std::move(tokens));
        auto   stmts = parser.parse();

        Resolver resolver;
        resolver.resolve(stmts);

        Interpreter interpreter;
        interpreter.interpret(stmts);

    } catch (const LexError& e) {
        std::cerr << e.what() << "\n";
        return 65;
    } catch (const ParseError& e) {
        std::cerr << e.what() << "\n";
        return 65;
    } catch (const StaticError& e) {
        std::cerr << e.what() << "\n";
        return 65;
    } catch (const RuntimeError& e) {
        // 런타임 오류는 줄 번호와 함께 출력
        std::cerr << "[line " << e.token.line << "] Runtime Error: " << e.what() << "\n";
        return 70;
    }

    return 0;
}
