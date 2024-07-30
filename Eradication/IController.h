
#pragma once

class IController
{
public:
    virtual bool Execute() = 0;
    virtual ~IController() {}
};
