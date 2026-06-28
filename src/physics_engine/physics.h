#pragma once
#include "../Constants.h"
#include <Eigen/Dense>
#include <cmath>
using namespace Eigen;
//State(For RK4 integration)
struct State{
    Vector4d X;
    Vector4d U;
    
    State(){
        X = Vector4d::Zero();
        U = Vector4d::Zero();
    }
    
    State(Vector4d pos, Vector4d vel){
        X = pos;
        U = vel;
    }
    // This defines what S+S' means and is needed cuz RK4 needs K1+K2+K3+K4 and this needs to be scaled by dt/6 
    State operator+(const State& other) const { return State(X + other.X, U + other.U); }
    State operator*(double scalar) const { return State(X * scalar, U * scalar); }
    friend State operator*(double scalar, const State& s) { return s * scalar; }

};

// Matter
struct Particle{
    State state;
    float mass;
    Vector3f color;

    Vector4d& pos(){return state.X;};
    Vector4d& vel(){return state.U;};
};
// Light
struct Light{
    State state;

    Vector4d& pos(){return state.X;};
    Vector4d& vel(){return state.U;};
};


Vector4d find_acceleration(State&);
State Integrator(const State& s);
