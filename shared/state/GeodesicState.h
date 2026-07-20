#pragma once
#include <Eigen/Dense>

using namespace Eigen;

struct State {
    Vector4d X;
    Vector4d U;
    
    State() {
        X = Vector4d::Zero();
        U = Vector4d::Zero();
    }
    
    State(Vector4d pos, Vector4d vel) {
        X = pos;
        U = vel;
    }
    
    State operator+(const State& other) const { return State(X + other.X, U + other.U); }
    State operator*(double scalar) const { return State(X * scalar, U * scalar); }
    friend State operator*(double scalar, const State& s) { return s * scalar; }
};

struct Particle {
    State state;
    float mass;
    Vector3f color;

    Vector4d& pos() { return state.X; }
    Vector4d& vel() { return state.U; }
};

struct Light {
    State state;

    Vector4d& pos() { return state.X; }
    Vector4d& vel() { return state.U; }
};
