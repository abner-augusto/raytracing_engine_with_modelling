#ifndef RENDER_STATE_H
#define RENDER_STATE_H

enum RenderMode {
    DefaultRender,
    HighResolution,
    LowResolution,
    Disabled
};

class RenderState {
public:
    RenderState()
        : current_mode(DefaultRender),
        previous_mode(Disabled)
    {
    }

    RenderMode get_mode() const {
        return current_mode;
    }

    RenderMode get_previous_mode() const {
        return previous_mode;
    }

    void set_mode(RenderMode mode) {
        previous_mode = current_mode;
        current_mode = mode;
    }

    bool is_mode(RenderMode mode) const {
        return current_mode == mode;
    }

private:
    RenderMode current_mode;
    RenderMode previous_mode;
};

#endif // RENDER_STATE_H
