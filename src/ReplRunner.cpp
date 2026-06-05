#include "ReplRunner.h"
#include "Lexer.h"
#include "Parser.h"
#include "Resolver.h"
#include "Interpreter.h"
#include <iostream>
#include <string>
#include <vector>

int ReplRunner::run() {
    // Interpreter를 루프 밖에서 생성 → globalEnv가 세션 전체에서 유지됨
    // 매 입력마다 새 Interpreter를 만들면 변수가 사라진다.
    Interpreter interpreter;

    // FabFunction::body는 FuncStmt::body에 대한 비소유 포인터이므로
    // 함수가 선언된 AST(stmts)가 소멸하면 댕글링 포인터가 된다.
    // REPL 세션이 끝날 때까지 모든 AST를 살려두기 위해 여기서 보관한다.
    std::vector<std::vector<StmtPtr>> sessionStmts;

    std::cout << "Fab REPL  (type 'exit' or 'quit' to quit)\n";

    std::string line;
    while (true) {
        std::cout << "> " << std::flush;

        // EOF (Ctrl+Z on Windows, Ctrl+D on Unix)
        if (!std::getline(std::cin, line)) break;

        // 공백만 있는 줄은 건너뜀
        if (line.find_first_not_of(" \t\r\n") == std::string::npos) continue;

        // 종료 명령
        if (line == "exit" || line == "quit") break;

        try {
            Lexer lexer(line);
            auto  tokens = lexer.scanTokens();

            Parser parser(std::move(tokens));
            auto   stmts = parser.parse();

            // 매 입력마다 새 Resolver로 정적 검사
            // 전역 스코프(scopes.empty())에서는 중복 선언 검사를 건너뛰므로
            // REPL에서 var x 재선언이 허용된다.
            Resolver resolver;
            resolver.resolve(stmts);

            interpreter.interpret(stmts);

            // 함수 선언 AST를 살려두기 위해 이동 보관
            sessionStmts.push_back(std::move(stmts));

        } catch (const LexError& e) {
            // 오류를 출력하고 루프를 계속 — REPL은 오류로 종료되지 않는다.
            std::cerr << e.what() << "\n";
        } catch (const ParseError& e) {
            std::cerr << e.what() << "\n";
        } catch (const StaticError& e) {
            std::cerr << e.what() << "\n";
        } catch (const RuntimeError& e) {
            std::cerr << "[line " << e.token.line << "] Runtime Error: " << e.what() << "\n";
        }
    }

    std::cout << "\n";
    return 0;
}
