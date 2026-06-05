#pragma once

// Ch.5: 모든 실행 모드(파일/REPL/디버그)의 공통 인터페이스
// InterpreterFactory가 이 타입을 반환하므로 main.cpp는 모드를 몰라도 된다.
// GoF Factory Method 패턴의 Product 역할
class RunnerBase {
public:
    // 모드 실행. 프로세스 종료 코드를 반환한다.
    virtual int run() = 0;
    virtual ~RunnerBase() = default;
};
