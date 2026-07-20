#pragma once
#include <string>

class RenderPass {
public:
    virtual ~RenderPass() = default;

    virtual void execute() = 0;
    virtual const std::string& name() const = 0;
};
