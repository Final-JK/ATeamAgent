#pragma once
#include "RunnerBase.h"

// Ch.5: REPL(프롬프트) 모드 Runner — 구현은 별도 커밋에서 추가
// 사용자가 소스를 한 줄씩 직접 입력하는 대화형 실행 모드
class ReplRunner : public RunnerBase {
public:
    int run() override;
};
