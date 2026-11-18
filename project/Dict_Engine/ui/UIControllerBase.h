#pragma once
class UIControllerBase
{
public:
    virtual ~UIControllerBase() = default;

    virtual void Initialize();

    virtual void Finalize();

    virtual void Update(float deltaTime) = 0;
};
