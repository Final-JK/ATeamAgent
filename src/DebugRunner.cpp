#include "DebugRunner.h"
#include "Lexer.h"
#include "Parser.h"
#include "Resolver.h"
#include "Interpreter.h"
#include "Stmt.h"
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>
#include <algorithm>

DebugRunner::DebugRunner(std::string path)
    : filePath(std::move(path)) {}

// ── 내부 유틸 ─────────────────────────────────────────────────────────────────

// Stmt에서 줄 번호를 추출한다.
// Token 필드가 있는 타입만 줄 번호 반환, 나머지는 0.
static int stmtLine(const Stmt& stmt) {
    if (auto* s = dynamic_cast<const VarStmt*>    (&stmt)) return s->name.line;
    if (auto* s = dynamic_cast<const FuncStmt*>   (&stmt)) return s->name.line;
    if (auto* s = dynamic_cast<const ReturnStmt*> (&stmt)) return s->keyword.line;
    // PrintStmt, ExprStmt, BlockStmt, IfStmt, ForStmt은 Token 필드 없음 → 0
    return 0;
}

// Stmt 타입명을 반환한다 (디버그 출력용)
static std::string stmtTypeName(const Stmt& stmt) {
    if (dynamic_cast<const PrintStmt*>  (&stmt)) return "PrintStmt";
    if (dynamic_cast<const VarStmt*>    (&stmt)) return "VarStmt";
    if (dynamic_cast<const ExprStmt*>   (&stmt)) return "ExprStmt";
    if (dynamic_cast<const BlockStmt*>  (&stmt)) return "BlockStmt";
    if (dynamic_cast<const IfStmt*>     (&stmt)) return "IfStmt";
    if (dynamic_cast<const ForStmt*>    (&stmt)) return "ForStmt";
    if (dynamic_cast<const FuncStmt*>   (&stmt)) return "FuncStmt";
    if (dynamic_cast<const ReturnStmt*> (&stmt)) return "ReturnStmt";
    return "Stmt";
}

// Value를 문자열로 변환 (디버그 출력용 — Interpreter::stringify 외부 복제)
static std::string valueToStr(const Value& v) {
    if (std::holds_alternative<std::nullptr_t>(v)) return "nil";
    if (std::holds_alternative<bool>(v))           return std::get<bool>(v) ? "true" : "false";
    if (std::holds_alternative<double>(v)) {
        double d = std::get<double>(v);
        if (d == static_cast<long long>(d)) return std::to_string(static_cast<long long>(d));
        std::ostringstream oss; oss << d; return oss.str();
    }
    if (std::holds_alternative<std::string>(v)) return std::get<std::string>(v);
    if (std::holds_alternative<std::shared_ptr<FabFunction>>(v))
        return "<function " + std::get<std::shared_ptr<FabFunction>>(v)->name + ">";
    if (std::holds_alternative<std::shared_ptr<FabArray>>(v))
        return "<array[" + std::to_string(std::get<std::shared_ptr<FabArray>>(v)->elements.size()) + "]>";
    return "?";
}

// ── 디버그 명령 처리 ──────────────────────────────────────────────────────────

// 정지 시점마다 watch 변수를 자동 출력한다.
static void printWatches(
    const std::vector<std::string>& watchList,
    const std::shared_ptr<Environment>& env)
{
    if (watchList.empty()) return;
    for (const auto& varName : watchList) {
        // 가짜 토큰으로 get() 호출 (위치 정보 불필요, line=0)
        Token tok(TokenType::IDENTIFIER, varName, nullptr, 0);
        try {
            Value v = env->get(tok);
            std::cout << "  watch " << varName << " = " << valueToStr(v) << "\n";
        } catch (...) {
            std::cout << "  watch " << varName << " = <undefined>\n";
        }
    }
}

// 현재 스코프 변수 전체 출력 (inspect)
static void printInspect(const std::shared_ptr<Environment>& env) {
    auto& vals = env->getValues();
    if (vals.empty()) { std::cout << "  (no variables in current scope)\n"; return; }
    for (const auto& [name, val] : vals)
        std::cout << "  " << name << " = " << valueToStr(val) << "\n";
}

// ── DebugRunner::run ──────────────────────────────────────────────────────────

int DebugRunner::run() {
    // 소스 파일 로드
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Cannot open file: " << filePath << "\n";
        return 66;
    }
    std::ostringstream ss; ss << file.rdbuf();
    std::string source = ss.str();

    // 파이프라인 실행 준비
    std::vector<StmtPtr> stmts;
    try {
        Lexer    lexer(source);
        auto     tokens = lexer.scanTokens();
        Parser   parser(std::move(tokens));
        stmts = parser.parse();
        Resolver resolver;
        resolver.resolve(stmts);
    } catch (const LexError& e)   { std::cerr << e.what() << "\n"; return 65; }
      catch (const ParseError& e) { std::cerr << e.what() << "\n"; return 65; }
      catch (const StaticError& e){ std::cerr << e.what() << "\n"; return 65; }

    // ── 디버그 상태 ──────────────────────────────────────────────────────────
    // 현재 stepping 중인지 여부 (true=정지, false=연속 실행)
    bool stepping = true;
    // 브레이크포인트 줄 번호 집합
    std::unordered_set<int> breakpoints;
    // 감시 변수 목록
    std::vector<std::string> watchList;

    Interpreter interpreter;

    // Stmt 실행 직전 훅: stepping·breakpoint 제어를 담당한다.
    interpreter.stmtHook = [&](const Stmt& stmt) {
        int line = stmtLine(stmt);

        // RUNNING 모드에서 브레이크포인트 도달 시 stepping으로 전환
        if (!stepping && line > 0 && breakpoints.count(line)) {
            stepping = true;
            std::cout << "[debug] Breakpoint hit at line " << line << "\n";
        }

        if (!stepping) return; // RUNNING 모드 — 그냥 통과

        // STEPPING 모드: 정지하고 사용자 명령을 기다린다.
        std::cout << "[debug] Stopped at line " << line
                  << " (" << stmtTypeName(stmt) << ")\n";

        // 정지 시점마다 감시 변수 자동 출력
        printWatches(watchList, interpreter.getCurrentEnv());

        // 명령 입력 루프
        std::string cmd;
        while (true) {
            std::cout << "(debug) " << std::flush;
            if (!std::getline(std::cin, cmd)) { stepping = false; return; }

            // 앞뒤 공백 제거
            auto first = cmd.find_first_not_of(" \t");
            if (first == std::string::npos) continue;
            cmd = cmd.substr(first);

            // ── 명령 파싱 ─────────────────────────────────────────────────
            if (cmd == "step" || cmd == "s") {
                // 현재 Stmt 실행 후 다음 Stmt에서 다시 정지
                stepping = true;
                return;

            } else if (cmd == "next" || cmd == "n") {
                // step과 동일 (블록 진입 방지는 현재 구조상 step과 같음)
                stepping = true;
                return;

            } else if (cmd == "continue" || cmd == "c") {
                // 다음 브레이크포인트까지 실행
                stepping = false;
                return;

            } else if (cmd.substr(0, 6) == "break " || cmd.substr(0, 2) == "b ") {
                // break <줄번호> — 브레이크포인트 설정
                std::istringstream iss(cmd);
                std::string tok; int bline;
                iss >> tok >> bline;
                breakpoints.insert(bline);
                std::cout << "Breakpoint set at line " << bline << "\n";

            } else if (cmd == "breakpoints" || cmd == "bp") {
                // 현재 브레이크포인트 목록
                if (breakpoints.empty()) {
                    std::cout << "No breakpoints set.\n";
                } else {
                    std::cout << "Breakpoints: ";
                    for (int bl : breakpoints) std::cout << bl << " ";
                    std::cout << "\n";
                }

            } else if (cmd.substr(0, 7) == "remove " || cmd.substr(0, 2) == "r ") {
                // remove <줄번호> — 브레이크포인트 해제
                std::istringstream iss(cmd);
                std::string tok; int bline;
                iss >> tok >> bline;
                breakpoints.erase(bline);
                std::cout << "Breakpoint removed from line " << bline << "\n";

            } else if (cmd.substr(0, 6) == "watch " || cmd.substr(0, 2) == "w ") {
                // watch <변수명> — 감시 목록 추가
                std::istringstream iss(cmd);
                std::string tok, varName;
                iss >> tok >> varName;
                if (!varName.empty() &&
                    std::find(watchList.begin(), watchList.end(), varName) == watchList.end())
                    watchList.push_back(varName);
                std::cout << "Watching '" << varName << "'\n";

            } else if (cmd.substr(0, 8) == "unwatch " || cmd.substr(0, 3) == "uw ") {
                // unwatch <변수명> — 감시 목록에서 제거
                std::istringstream iss(cmd);
                std::string tok, varName;
                iss >> tok >> varName;
                watchList.erase(
                    std::remove(watchList.begin(), watchList.end(), varName),
                    watchList.end());
                std::cout << "Unwatched '" << varName << "'\n";

            } else if (cmd == "watches" || cmd == "ws") {
                // 현재 감시 목록 + 값 출력
                if (watchList.empty()) {
                    std::cout << "No watches set.\n";
                } else {
                    printWatches(watchList, interpreter.getCurrentEnv());
                }

            } else if (cmd == "inspect" || cmd == "i") {
                // 현재 스코프 전체 변수 출력
                printInspect(interpreter.getCurrentEnv());

            } else if (cmd == "help" || cmd == "h") {
                std::cout
                    << "  step/s            -- execute current stmt, stop at next\n"
                    << "  next/n            -- same as step\n"
                    << "  continue/c        -- run until next breakpoint\n"
                    << "  break/b <line>    -- set breakpoint\n"
                    << "  breakpoints/bp    -- list breakpoints\n"
                    << "  remove/r <line>   -- remove breakpoint\n"
                    << "  watch/w <var>     -- add variable to watch list\n"
                    << "  unwatch/uw <var>  -- remove from watch list\n"
                    << "  watches/ws        -- show all watches with values\n"
                    << "  inspect/i         -- show all variables in current scope\n";

            } else {
                std::cout << "Unknown command '" << cmd << "'. Type 'help' for list.\n";
            }
        }
    };

    // 실행
    try {
        interpreter.interpret(stmts);
    } catch (const RuntimeError& e) {
        std::cerr << "[line " << e.token.line << "] Runtime Error: " << e.what() << "\n";
        return 70;
    }

    std::cout << "[debug] Program finished.\n";
    return 0;
}
