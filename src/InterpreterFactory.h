#pragma once
#include "RunnerBase.h"
#include <memory>

// Ch.5: GoF Factory Method 패턴 — ConcreteCreator 역할
// argc/argv를 분석해 적절한 Runner를 생성한다.
//
// 모드 감지 규칙:
//   argc == 1                        → REPL 모드
//   argc == 2                        → 파일 모드
//   argc == 3 && argv[1]=="--debug"  → 디버그 모드
class InterpreterFactory {
public:
    // Runner 생성 및 반환. 잘못된 인수면 stderr에 사용법 출력 후 nullptr 반환.
    static std::unique_ptr<RunnerBase> create(int argc, char* argv[]);
};
