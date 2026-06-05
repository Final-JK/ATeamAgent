#pragma once
#include "RunnerBase.h"
#include <string>

// Ch.5: 파일 모드 Runner
// 기존 main.cpp의 파일 실행 로직을 이 클래스로 이동.
// 소스 파일을 읽어 Lexer→Parser→Resolver→Interpreter 파이프라인을 실행한다.
class FileRunner : public RunnerBase {
public:
    explicit FileRunner(std::string path);
    int run() override;

private:
    std::string filePath;
};
