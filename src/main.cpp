#include "InterpreterFactory.h"
#include <iostream>

// Ch.5: main은 모드 선택과 실행만 담당한다.
// 파일/REPL/디버그 모드 로직은 각 Runner 클래스에 위임.
// 모드 감지 규칙은 InterpreterFactory::create() 참조.
int main(int argc, char* argv[]) {
    auto runner = InterpreterFactory::create(argc, argv);
    if (!runner) return 64;  // 잘못된 인수 — 사용법 안내 후 종료
    return runner->run();
}
