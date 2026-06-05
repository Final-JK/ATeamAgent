#include "DebugRunner.h"
#include <iostream>

DebugRunner::DebugRunner(std::string path)
    : filePath(std::move(path)) {}

// TODO(Ch.5): 디버그 모드 전체 구현 — 이후 커밋에서 추가
int DebugRunner::run() {
    std::cerr << "Debug mode: not yet implemented.\n";
    return 1;
}
