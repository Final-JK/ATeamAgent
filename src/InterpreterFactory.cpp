#include "InterpreterFactory.h"
#include "FileRunner.h"
#include "ReplRunner.h"
#include "DebugRunner.h"
#include <iostream>
#include <string>

std::unique_ptr<RunnerBase> InterpreterFactory::create(int argc, char* argv[]) {
    // 인수 없음 → REPL(프롬프트) 모드
    if (argc == 1) {
        return std::make_unique<ReplRunner>();
    }

    // "--debug <파일>" → 디버그 모드
    if (argc == 3 && std::string(argv[1]) == "--debug") {
        return std::make_unique<DebugRunner>(argv[2]);
    }

    // "<파일>" → 파일 모드
    if (argc == 2) {
        // "--debug"만 단독으로 쓰면 사용법 안내
        if (std::string(argv[1]) == "--debug") {
            std::cerr << "Usage: interpreter --debug <script>\n";
            return nullptr;
        }
        return std::make_unique<FileRunner>(argv[1]);
    }

    // 그 외 인수 조합은 사용법 안내
    std::cerr << "Usage:\n"
              << "  interpreter                  (REPL mode)\n"
              << "  interpreter <script>         (file mode)\n"
              << "  interpreter --debug <script> (debug mode)\n";
    return nullptr;
}
