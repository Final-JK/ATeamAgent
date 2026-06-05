#pragma once
#include "RunnerBase.h"
#include <string>

// Ch.5: 디버그 모드 Runner — 구현은 별도 커밋에서 추가
// 소스 파일을 Stmt 단위로 멈추며 실행 상태를 점검하는 모드
class DebugRunner : public RunnerBase {
public:
    explicit DebugRunner(std::string path);
    int run() override;

private:
    std::string filePath;
};
